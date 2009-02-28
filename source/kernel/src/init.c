/* Init thread
 *
 * Copyright (c) 2008 Zoltan Kovacs
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
#include <mm/pages.h>
#include <vfs/vfs.h>
#include <lib/string.h>

#include <arch/fork.h>
#include <arch/loader.h>
#include <arch/smp.h>

extern int __k_init_start;
extern int __kernel_end;

__init static void load_bootmodules( void ) {
    int i;
    int error;
    int module_count;
    bootmodule_t* bootmodule;

    module_count = get_bootmodule_count();

    for ( i = 0; i < module_count; i++ ) {
        bootmodule = get_bootmodule_at( i );

        kprintf( "Loading module: %s\n", bootmodule->name );

        error = load_module( bootmodule->name );

        if ( error < 0 ) {
            kprintf( "Failed to load module: %s\n", bootmodule->name );
        }
    }
}

__init static void mount_root_filesystem( void ) {
    int dir;
    int error;
    dirent_t entry;
    char path[ 255 ];

    error = mkdir( "/yaosp", 0 );

    if ( error < 0 ) {
        panic( "Failed to mount root filesystem: failed to create mount point!\n" );
    }

    dir = open( "/device/disk", O_RDONLY );

    if ( dir < 0 ) {
        panic( "Failed to mount root filesystem: no disk(s) available!\n" );
    }

    error = -1;

    while ( getdents( dir, &entry, sizeof( dirent_t ) ) == 1 ) {
        if ( ( strcmp( entry.name, "." ) == 0 ) ||
             ( strcmp( entry.name, ".." ) == 0 ) ) {
            continue;
        }

        if ( ( strlen( entry.name ) < 2 ) ||
             ( strncmp( entry.name, "hd", 2 ) != 0 ) ) {
            continue;
        }

        snprintf( path, sizeof( path ), "/device/disk/%s", entry.name );

        error = mount( path, "/yaosp", "iso9660", MOUNT_RO );

        if ( error >= 0 ) {
            break;
        }
    }

    close( dir );

    if ( error < 0 ) {
        panic( "Failed to mount root filesystem!\n" );
    }

    kprintf( "Root filesystem mounted!\n" );
}

int init_thread( void* arg ) {
    uint32_t init_page_count;

    kprintf( "Init thread started!\n" );

#ifdef ENABLE_SMP
    arch_boot_processors();
#endif /* ENABLE_SMP */

    kprintf( "Initializing Virtual File System ... " );
    init_vfs();
    kprintf( "done\n" );

    load_bootmodules();
    mount_root_filesystem();

    /* Free init code */

    init_page_count = ( ( uint32_t )&__kernel_end - ( uint32_t )&__k_init_start ) / PAGE_SIZE;
    kprintf( "Freeing %u pages containing initialization code.\n", init_page_count );
    free_pages( ( void* )&__k_init_start, init_page_count );

    /* Create a new process and start the init application */

    if ( fork() == 0 ) {
        if ( execve( "/yaosp/application/init", NULL, NULL ) != 0 ) {
            panic( "Failed to execute init process!\n" );
        }
    }

    return 0;
}
