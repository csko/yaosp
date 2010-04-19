/* Init thread
 *
 * Copyright (c) 2008, 2009, 2010 Zoltan Kovacs
 * Copyright (c) 2009 Kornel Csernai
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <kernel.h>
#include <console.h>
#include <bootmodule.h>
#include <module.h>
#include <loader.h>
#include <config.h>
#include <errno.h>
#include <process.h>
#include <mm/pages.h>
#include <vfs/vfs.h>
#include <network/network.h>
#include <lib/string.h>

#include <arch/fork.h>
#include <arch/loader.h>
#include <arch/smp.h>

thread_id init_thread_id;

__init static void load_bootmodules( void ) {
    int i;
    int error;
    int module_count;
    bootmodule_t* bootmodule;

    module_count = get_bootmodule_count();

    for ( i = 0; i < module_count; i++ ) {
        bootmodule = get_bootmodule_at( i );

        kprintf( INFO, "Loading module: %s\n", bootmodule->name );

        error = load_module( bootmodule->name );

        if ( error < 0 ) {
            kprintf( INFO, "Failed to load module: %s\n", bootmodule->name );
        }
    }

    release_bootmodules();
}

__init static int scan_and_mount_root_filesystem( void ) {
    int dir;
    int error;
    dirent_t entry;
    char path[ 128 ];

    /* Check all storage nodes and try to find out which one is the root */

    dir = open( "/device/storage", O_RDONLY );

    if ( dir < 0 ) {
        panic( "Failed to mount root filesystem: no disk(s) available!\n" );
    }

    error = -ENOENT;

    while ( getdents( dir, &entry, sizeof( dirent_t ) ) == 1 ) {
        if ( ( strcmp( entry.name, "." ) == 0 ) ||
             ( strcmp( entry.name, ".." ) == 0 ) ) {
            continue;
        }

        if ( strlen( entry.name ) < 2 ) {
            continue;
        }

        snprintf( path, sizeof( path ), "/device/storage/%s", entry.name );

        error = mount( path, "/yaosp", "iso9660", MOUNT_RO );

        if ( error >= 0 ) {
            kprintf( INFO, "Root filesystem mounted from %s.\n", path );
            break;
        }
    }

    close( dir );

    return error;
}

__init static void mount_root_filesystem( void ) {
    int error;
    const char* root;

    /* Create the directory where we'll mount the root */

    error = mkdir( "/yaosp", 0777 );

    if ( error < 0 ) {
        panic( "Failed to mount root filesystem: failed to create mount point!\n" );
    }

    /* Try root= kernel parameter */

    error = get_kernel_param_as_string( "root", &root );

    if ( error == 0 ) {
        error = mount( root, "/yaosp", "ext2", MOUNT_NONE );

        if ( error >= 0 ) {
            kprintf( INFO, "Root filesystem mounted from %s.\n", root );
        }
    } else {
        error = scan_and_mount_root_filesystem();
    }

    if ( error < 0 ) {
        panic( "Failed to mount root filesystem!\n" );
    }
}

int init_thread( void* arg ) {
    uint32_t init_page_count;

    DEBUG_LOG( "Init thread started.\n" );

    kernel_running = 1;

#ifdef ENABLE_SMP
    arch_boot_processors();
#endif /* ENABLE_SMP */

    init_vfs();
    load_bootmodules();
    mount_root_filesystem();

#ifdef ENABLE_NETWORK
    init_network();
#endif /* ENABLE_NETWORK */

    /* Free init code */

    init_page_count = ( ( uint32_t )&__kernel_end - ( uint32_t )&__k_init_start ) / PAGE_SIZE;
    kprintf( INFO, "Freeing %u pages containing initialization code.\n", init_page_count );
    free_pages( ( void* )&__k_init_start, init_page_count );

    /* Create a new process and start the init application */

    if ( fork() == 0 ) {
        if ( execve( "/yaosp/system/init", NULL, NULL ) != 0 ) {
            panic( "Failed to execute init process!\n" );
        }
    }

    while ( 1 ) {
        sys_wait4( -1, NULL, 0, NULL );
    }

    return 0;
}

__init int create_init_thread( void ) {
    /* Create the init thread */

    init_thread_id = create_kernel_thread( "init", PRIORITY_NORMAL, init_thread, NULL, 0 );

    if ( init_thread_id < 0 ) {
        return init_thread_id;
    }

    thread_wake_up( init_thread_id );

    return 0;
}
