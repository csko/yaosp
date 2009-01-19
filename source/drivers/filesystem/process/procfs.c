/* Process filesystem implementation
 *
 * Copyright (c) 2009 Zoltan Kovacs, Kornel Csernai
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

#include <errno.h>
#include <semaphore.h>
#include <console.h>
#include <scheduler.h>
#include <process.h>
#include <macros.h>
#include <sysinfo.h>
#include <mm/kmalloc.h>
#include <vfs/filesystem.h>
#include <vfs/vfs.h>
#include <lib/hashtable.h>
#include <lib/string.h>

#include "procfs.h"

static hashtable_t procfs_inode_table;
static ino_t procfs_inode_counter = 0;
static semaphore_id procfs_lock;
static procfs_node_t* procfs_root_node;

static procfs_node_t* procfs_create_node( procfs_node_t* parent, char* name, bool is_directory ) {
    procfs_node_t* node;

    node = ( procfs_node_t* )kmalloc( sizeof( procfs_node_t ) );

    if ( node == NULL ) {
        return NULL;
    }

    memset( node, 0, sizeof( procfs_node_t ) );

    node->name = strdup( name );

    if ( node->name == NULL ) {
        kfree( node );
        return NULL;
    }

    node->is_directory = is_directory;

    do {
        node->inode_number = procfs_inode_counter++;

        if ( procfs_inode_counter < 0 ) {
            procfs_inode_counter = 0;
        }
    } while ( hashtable_get( &procfs_inode_table, ( const void* )&node->inode_number ) != NULL );

    hashtable_add( &procfs_inode_table, ( hashitem_t* )node );

    node->parent = parent;

    if ( parent != NULL ) {
        node->next_sibling = parent->first_child;
        parent->first_child = node;
    }

    return node;
}

static procfs_node_t* procfs_create_node_with_data(
    procfs_node_t* parent,
    char* name,
    bool is_directory,
    char* data,
    size_t size
) {
    procfs_node_t* node;

    node = procfs_create_node( parent, name, is_directory );

    if ( node == NULL ) {
        return NULL;
    }

    node->data = ( char* )kmalloc( size );

    if ( node->data == NULL ) {
        /* TODO: cleanup! */
        return NULL;
    }

    memcpy( node->data, data, size );
    node->data_size = size;

    return node;
}

static void procfs_delete_node( procfs_node_t* node, bool delete_children ) {
    if ( delete_children ) {
        procfs_node_t* tmp;

        while ( node->first_child != NULL ) {
            tmp = node->first_child;
            node->first_child = tmp->next_sibling;

            procfs_delete_node( tmp, delete_children );
        }
    }

    hashtable_remove( &procfs_inode_table, ( const void* )&node->inode_number );

    kfree( node->data );
    kfree( node->name );
    kfree( node );
}

static int procfs_thread_callback( thread_t* thread, void* data ) {
    char tmp[ 16 ];
    procfs_node_t* thr_node;
    procfs_thr_iter_data_t* thr_data;

    thr_data = ( procfs_thr_iter_data_t* )data;

    if ( thread->process->id != thr_data->proc_id ) {
        return 0;
    }

    snprintf( tmp, sizeof( tmp ), "%d", thread->id );

    thr_node = procfs_create_node( thr_data->proc_node, tmp, true );
    thr_node->name_node = procfs_create_node_with_data( thr_node, "name", false, thread->name, strlen( thread->name ) );

    return 0;
}

static int procfs_proc_callback( process_t* process, void* data ) {
    char tmp[ 16 ];
    procfs_node_t* proc_node;
    bool* scan_threads;

    scan_threads = ( bool* )data;

    snprintf( tmp, sizeof( tmp ), "%d", process->id );

    proc_node = procfs_create_node( procfs_root_node, tmp, true );
    proc_node->name_node = procfs_create_node_with_data( proc_node, "name", false, process->name, strlen( process->name ) );

    if ( *scan_threads ) {
        procfs_thr_iter_data_t thr_data;

        thr_data.proc_id = process->id;
        thr_data.proc_node = proc_node;

        thread_table_iterate( procfs_thread_callback, ( void* )&thr_data );
    }

    return 0;
}

static int procfs_mount( const char* device, uint32_t flags, void** fs_cookie, ino_t* root_inode_num ) {
    bool scan_threads = true;
    procfs_root_node = procfs_create_node( NULL, "", true );

    if ( procfs_root_node == NULL ) {
        return -ENOMEM;
    }

    /* Get the current process & thread list */

    lock_scheduler();

    process_table_iterate( procfs_proc_callback, ( void* )&scan_threads );

    unlock_scheduler();

    *root_inode_num = procfs_root_node->inode_number;

    return 0;
}

static int procfs_read_inode( void* fs_cookie, ino_t inode_num, void** _node ) {
    procfs_node_t* node;

    LOCK( procfs_lock );

    node = ( procfs_node_t* )hashtable_get( &procfs_inode_table, ( const void* )&inode_num );

    UNLOCK( procfs_lock );

    *_node = node;

    if ( node == NULL ) {
        return -ENOINO;
    }

    return 0;
}

static int procfs_write_inode( void* fs_cookie, void* node ) {
    return 0;
}

static int procfs_lookup_inode( void* fs_cookie, void* _parent, const char* name, int name_len, ino_t* inode_num ) {
    procfs_node_t* node;
    procfs_node_t* parent;

    parent = ( procfs_node_t* )_parent;

    LOCK( procfs_lock );

    if ( ( name_len == 2 ) &&
         ( strncmp( name, "..", 2 ) == 0 ) ) {
        *inode_num = parent->parent->inode_number;

        UNLOCK( procfs_lock );

        return 0;
    }

    node = parent->first_child;

    while ( node != NULL ) {
        if ( ( strlen( node->name ) == name_len ) &&
             ( strncmp( node->name, name, name_len ) == 0 ) ) {
            *inode_num = node->inode_number;

            UNLOCK( procfs_lock );

            return 0;
        }

        node = node->next_sibling;
    }

    UNLOCK( procfs_lock );

    return -ENOINO;
}

static int procfs_open_directory( procfs_node_t* node, void** file_cookie ) {
    procfs_dir_cookie_t* cookie;

    cookie = ( procfs_dir_cookie_t* )kmalloc( sizeof( procfs_dir_cookie_t ) );

    if ( cookie == NULL ) {
        return -ENOMEM;
    }

    cookie->position = 0;

    *file_cookie = ( void* )cookie;

    return 0;
}

static int procfs_open( void* fs_cookie, void* _node, int mode, void** file_cookie ) {
    int error;
    procfs_node_t* node;

    node = ( procfs_node_t* )_node;

    if ( node->is_directory ) {
        error = procfs_open_directory( node, file_cookie );
    } else {
        error = 0;
    }

    return error;
}

static int procfs_close( void* fs_cookie, void* node, void* file_cookie ) {
    return 0;
}

static int procfs_free_cookie( void* fs_cookie, void* _node, void* file_cookie ) {
    procfs_node_t* node;

    node = ( procfs_node_t* )_node;

    if ( node->is_directory ) {
        kfree( file_cookie );
    }

    return 0;
}

static int procfs_read( void* fs_cookie, void* _node, void* file_cookie, void* buffer, off_t pos, size_t size ) {
    size_t to_read;
    procfs_node_t* node;

    node = ( procfs_node_t* )_node;

    if ( node->is_directory ) {
        return -EISDIR;
    }

    LOCK( procfs_lock );

    to_read = MIN( node->data_size - pos, size );

    if ( to_read == 0 ) {
        goto out;
    }

    memcpy( buffer, node->data + pos, to_read );

out:
    UNLOCK( procfs_lock );

    return to_read;
}

static int procfs_read_stat( void* fs_cookie, void* _node, struct stat* stat ) {
    procfs_node_t* node;

    node = ( procfs_node_t* )_node;

    memset( stat, 0, sizeof( struct stat ) );

    stat->st_ino = node->inode_number;
    stat->st_size = node->data_size;

    if ( node->is_directory ) {
        stat->st_mode |= S_IFDIR;
    }

    return 0;
}

static int procfs_read_directory( void* fs_cookie, void* _node, void* file_cookie, struct dirent* entry ) {
    int current;
    procfs_node_t* parent;
    procfs_node_t* node;
    procfs_dir_cookie_t* cookie;

    parent = ( procfs_node_t* )_node;
    cookie = ( procfs_dir_cookie_t* )file_cookie;

    LOCK( procfs_lock );

    current = 0;
    node = parent->first_child;

    while ( node != NULL ) {
        if ( current == cookie->position ) {
            entry->inode_number = node->inode_number;
            strncpy( entry->name, node->name, NAME_MAX );
            entry->name[ NAME_MAX ] = 0;

            UNLOCK( procfs_lock );

            cookie->position++;

            return 1;
        }

        current++;
        node = node->next_sibling;
    }

    UNLOCK( procfs_lock );

    return 0;
}

static filesystem_calls_t procfs_calls = {
    .probe = NULL,
    .mount = procfs_mount,
    .unmount = NULL,
    .read_inode = procfs_read_inode,
    .write_inode = procfs_write_inode,
    .lookup_inode = procfs_lookup_inode,
    .open = procfs_open,
    .close = procfs_close,
    .free_cookie = procfs_free_cookie,
    .read = procfs_read,
    .write = NULL,
    .ioctl = NULL,
    .read_stat = procfs_read_stat,
    .read_directory = procfs_read_directory,
    .create = NULL,
    .mkdir = NULL,
    .isatty = NULL,
    .add_select_request = NULL,
    .remove_select_request = NULL
};

static int procfs_process_created( process_t* process ) {
    bool scan_threads = false;

    LOCK( procfs_lock );

    lock_scheduler();
    procfs_proc_callback( process, ( void* )&scan_threads );
    unlock_scheduler();

    UNLOCK( procfs_lock );

    return 0;
}

static int procfs_process_destroyed( process_id id ) {
    char tmp[ 16 ];
    procfs_node_t* node;
    procfs_node_t* prev;

    snprintf( tmp, sizeof( tmp ), "%d", id );

    LOCK( procfs_lock );

    prev = NULL;
    node = procfs_root_node->first_child;

    while ( node != NULL ) {
        if ( strcmp( node->name, tmp ) == 0 ) {
            if ( prev == NULL ) {
                procfs_root_node->first_child = node->next_sibling;
            } else {
                prev->next_sibling = node->next_sibling;
            }

            break;
        }

        prev = node;
        node = node->next_sibling;
    }

    /* Delete the node if we found it */

    if ( node != NULL ) {
        procfs_delete_node( node, true );
    }

    UNLOCK( procfs_lock );

    return 0;
}

static int procfs_process_renamed( process_t* process ) {
    char tmp[ 16 ];
    procfs_node_t* node;

    snprintf( tmp, sizeof( tmp ), "%d", process->id );

    LOCK( procfs_lock );

    node = procfs_root_node->first_child;

    while ( node != NULL ) {
        if ( strcmp( node->name, tmp ) == 0 ) {
            size_t length;
            char* new_data;
            procfs_node_t* name_node;

            name_node = node->name_node;

            lock_scheduler();

            length = strlen( process->name );

            new_data = ( char* )kmalloc( length );

            if ( new_data != NULL ) {
                kfree( name_node->data );

                name_node->data = new_data;
                name_node->data_size = length;

                memcpy( new_data, process->name, length );
            }

            unlock_scheduler();

            break;
        }

        node = node->next_sibling;
    }

    UNLOCK( procfs_lock );

    return 0;
}

static int procfs_thread_created( thread_t* thread ) {
    char tmp[ 16 ];
    procfs_node_t* node;

    snprintf( tmp, sizeof( tmp ), "%d", thread->process->id );

    LOCK( procfs_lock );

    node = procfs_root_node->first_child;

    while ( node != NULL ) {
        if ( strcmp( node->name, tmp ) == 0 ) {
            break;
        }

        node = node->next_sibling;
    }

    if ( node == NULL ) {
        goto out;
    }

    lock_scheduler();

    procfs_thr_iter_data_t thr_data;

    thr_data.proc_id = thread->process->id;
    thr_data.proc_node = node;

    procfs_thread_callback( thread, ( void* )&thr_data );

    unlock_scheduler();

out:
    UNLOCK( procfs_lock );

    return 0;
}

static int procfs_thread_destroyed( process_id proc_id, thread_id thr_id ) {
    char tmp[ 16 ];
    procfs_node_t* node;
    procfs_node_t* prev;
    procfs_node_t* parent;

    snprintf( tmp, sizeof( tmp ), "%d", proc_id );

    LOCK( procfs_lock );

    node = procfs_root_node->first_child;

    while ( node != NULL ) {
        if ( strcmp( node->name, tmp ) == 0 ) {
            break;
        }

        node = node->next_sibling;
    }

    if ( node == NULL ) {
        goto out;
    }

    snprintf( tmp, sizeof( tmp ), "%d", thr_id );

    prev = NULL;
    parent = node;
    node = node->first_child;

    while ( node != NULL ) {
        if ( strcmp( node->name, tmp ) == 0 ) {
            if ( prev == NULL ) {
                parent->first_child = node->next_sibling;
            } else {
                prev->next_sibling = node->next_sibling;
            }

            break;
        }

        prev = node;
        node = node->next_sibling;
    }

    if ( node != NULL ) {
        procfs_delete_node( node, true );
    }

out:
    UNLOCK( procfs_lock );

    return 0;
}

static int procfs_thread_renamed( thread_t* thread ) {
    char tmp[ 16 ];
    procfs_node_t* node;

    snprintf( tmp, sizeof( tmp ), "%d", thread->process->id );

    LOCK( procfs_lock );

    node = procfs_root_node->first_child;

    while ( node != NULL ) {
        if ( strcmp( node->name, tmp ) == 0 ) {
            break;
        }

        node = node->next_sibling;
    }

    if ( node == NULL ) {
        goto out;
    }

    snprintf( tmp, sizeof( tmp ), "%d", thread->id );

    node = node->first_child;

    while ( node != NULL ) {
        if ( strcmp( node->name, tmp ) == 0 ) {
            size_t length;
            char* new_data;
            procfs_node_t* name_node;

            name_node = node->name_node;

            lock_scheduler();

            length = strlen( thread->name );

            new_data = ( char* )kmalloc( length );

            if ( new_data != NULL ) {
                kfree( name_node->data );

                name_node->data = new_data;
                name_node->data_size = length;

                memcpy( new_data, thread->name, length );
            }

            unlock_scheduler();

            break;
        }

        node = node->next_sibling;
    }

out:
    UNLOCK( procfs_lock );

    return 0;
}

static process_listener_t procfs_listener = {
    .process_created = procfs_process_created,
    .process_destroyed = procfs_process_destroyed,
    .process_renamed = procfs_process_renamed,
    .thread_created = procfs_thread_created,
    .thread_destroyed = procfs_thread_destroyed,
    .thread_renamed = procfs_thread_renamed
};

static void* procfs_node_key( hashitem_t* item ) {
    procfs_node_t* node;

    node = ( procfs_node_t* )item;

    return ( void* )&node->inode_number;
}

static uint32_t procfs_node_hash( const void* key ) {
    return hash_number( ( uint8_t* )key, sizeof( ino_t ) );
}

static bool procfs_node_compare( const void* key1, const void* key2 ) {
    ino_t* inode_num_1;
    ino_t* inode_num_2;

    inode_num_1 = ( ino_t* )key1;
    inode_num_2 = ( ino_t* )key2;

    return ( *inode_num_1 == *inode_num_2 );
}

int init_module( void ) {
    int error;

    procfs_lock = create_semaphore( "procfs lock", SEMAPHORE_BINARY, 0, 1 );

    if ( procfs_lock < 0 ) {
        return procfs_lock;
    }

    error = init_hashtable(
        &procfs_inode_table,
        256,
        procfs_node_key,
        procfs_node_hash,
        procfs_node_compare
    );

    if ( error < 0 ) {
        return error;
    }

    error = register_filesystem( "procfs", &procfs_calls );

    if ( error < 0 ) {
        return error;
    }

    error = mkdir( "/process", 0 );

    if ( error < 0 ) {
        return error;
    }

    error = mount( "", "/process", "procfs" );

    if ( error < 0 ) {
        return error;
    }

    kprintf( "Process filesystem mounted\n" );

    set_process_listener( &procfs_listener );

    kprintf( "Process listener registered\n" );

    return 0;
}

int destroy_module( void ) {
    return 0;
}
