/* Kernel debug filesystem
 *
 * Copyright (c) 2009 Zoltan Kovacs
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

#include <macros.h>
#include <errno.h>
#include <mm/kmalloc.h>
#include <vfs/kdebugfs.h>
#include <vfs/filesystem.h>
#include <vfs/vfs.h>
#include <lib/string.h>

static lock_id inode_lock;
static hashtable_t inode_table;
static ino_t next_inode_number = 0;

static kdbgfs_node_t* root_node = NULL;

static kdbgfs_node_t* kdebugfs_do_create_node( const char* name, size_t max_buffer_size ) {
    kdbgfs_node_t* node;

    node = ( kdbgfs_node_t* )kmalloc( sizeof( kdbgfs_node_t ) + max_buffer_size );

    if ( node == NULL ) {
        goto error1;
    }

    node->name = strdup( name );

    if ( node->name == NULL ) {
        goto error2;
    }

    node->buffer = ( char* )( node + 1 );
    node->buffer_size = 0;
    node->max_buffer_size = max_buffer_size;

    mutex_lock( inode_lock, LOCK_IGNORE_SIGNAL );

    do {
        node->inode_number = next_inode_number++;

        if ( next_inode_number < 0 ) {
            next_inode_number = 0;
        }
    } while ( hashtable_get( &inode_table, ( const void* )&node->inode_number ) != NULL );

    hashtable_add( &inode_table, ( hashitem_t* )node );

    mutex_unlock( inode_lock );

    return node;

 error2:
    kfree( node );

 error1:
    return NULL;
}

static int kdebugfs_mount( const char* device, uint32_t flags, void** fs_cookie, ino_t* root_inode_number ) {
    root_node = kdebugfs_do_create_node( "", 0 );

    if ( root_node == NULL ) {
        return -ENOMEM;
    }

    *root_inode_number = root_node->inode_number;

    return 0;
}

static int kdebugfs_read_inode( void* fs_cookie, ino_t inode_number, void** _node ) {
    kdbgfs_node_t* inode;

    mutex_lock( inode_lock, LOCK_IGNORE_SIGNAL );
    inode = ( kdbgfs_node_t* )hashtable_get( &inode_table, ( const void* )&inode_number );
    mutex_unlock( inode_lock );

    if ( inode == NULL ) {
        return -EINVAL;
    }

    *_node = ( void* )inode;

    return 0;
}

static int kdebugfs_write_inode( void* fs_cookie, void* node ) {
    return 0;
}

typedef struct lookup_data {
    const char* name;
    int name_length;
    ino_t* inode_number;
} lookup_data_t;

static int kdebugfs_lookup_helper( hashitem_t* item, void* _data ) {
    kdbgfs_node_t* node;
    lookup_data_t* data;

    node = ( kdbgfs_node_t* )item;
    data = ( lookup_data_t* )_data;

    if ( ( strlen( node->name ) == data->name_length ) &&
         ( strcmp( node->name, data->name ) == 0 ) ) {
        *( data->inode_number ) = node->inode_number;

        return -1;
    }

    return 0;
}

static int kdebugfs_lookup_inode( void* fs_cookie, void* _parent, const char* name, int name_length, ino_t* inode_number ) {
    int error;
    lookup_data_t data;
    kdbgfs_node_t* parent;

    parent = ( kdbgfs_node_t* )_parent;

    if ( ( name_length == 2 ) &&
         ( strncmp( name, "..", 2 ) == 0 ) ) {
        if ( parent == root_node ) {
            return -EINVAL;
        } else {
            *inode_number = root_node->inode_number;
            return 0;
        }
    }

    data.name = name;
    data.name_length = name_length;
    data.inode_number = inode_number;

    mutex_lock( inode_lock, LOCK_IGNORE_SIGNAL );

    if ( hashtable_iterate( &inode_table, kdebugfs_lookup_helper, &data ) == 0 ) {
        error = -ENOENT;
    } else {
        error = 0;
    }

    mutex_unlock( inode_lock );

    return error;
}

static int kdebugfs_open( void* fs_cookie, void* _node, int mode, void** file_cookie ) {
    kdbgfs_node_t* node;

    node = ( kdbgfs_node_t* )_node;

    if ( node == root_node ) {
        kdbgfs_dir_cookie_t* cookie;

        cookie = ( kdbgfs_dir_cookie_t* )kmalloc( sizeof( kdbgfs_dir_cookie_t ) );

        if ( cookie == NULL ) {
            return -ENOMEM;
        }

        cookie->position = 0;

        *file_cookie = ( void* )cookie;
    }

    return 0;
}

static int kdebugfs_close( void* fs_cookie, void* _node, void* file_cookie ) {
    kdbgfs_node_t* node;

    node = ( kdbgfs_node_t* )_node;

    if ( node == root_node ) {
        kfree( file_cookie );
    }

    return 0;
}

static int kdebugfs_read( void* fs_cookie, void* _node, void* file_cookie, void* buffer, off_t pos, size_t size ) {
    kdbgfs_node_t* node;

    if ( size == 0 ) {
        return 0;
    }

    node = ( kdbgfs_node_t* )_node;

    if ( node == root_node ) {
        return -EISDIR;
    }

    if ( pos >= node->buffer_size ) {
        return 0;
    }

    if ( size > ( node->buffer_size - pos ) ) {
        size = node->buffer_size - pos;
    }

    mutex_lock( inode_lock, LOCK_IGNORE_SIGNAL );
    memcpy( buffer, node->buffer + pos, size );
    mutex_unlock( inode_lock );

    return size;
}

static int kdebugfs_read_stat( void* fs_cookie, void* _node, struct stat* stat ) {
    kdbgfs_node_t* node;

    node = ( kdbgfs_node_t* )_node;

    mutex_lock( inode_lock, LOCK_IGNORE_SIGNAL );

    stat->st_ino = node->inode_number;
    stat->st_mode = 0777;
    stat->st_size = node->buffer_size;

    if ( node == root_node ) {
        stat->st_mode |= S_IFDIR;
    }

    mutex_unlock( inode_lock );

    return 0;
}

typedef struct dir_iter_data {
    int current;
    int position;
    struct dirent* entry;
} dir_iter_data_t;

static int kdebugfs_read_dir_helper( hashitem_t* item, void* _data ) {
    kdbgfs_node_t* node;
    dir_iter_data_t* data;

    node = ( kdbgfs_node_t* )item;
    data = ( dir_iter_data_t* )_data;

    if ( node == root_node ) {
        return 0;
    }

    if ( data->current == data->position ) {
        data->entry->inode_number = node->inode_number;

        strncpy( data->entry->name, node->name, NAME_MAX );
        data->entry->name[ NAME_MAX ] = 0;

        return -1;
    }

    data->current++;

    return 0;
}

static int kdebugfs_read_directory( void* fs_cookie, void* _node, void* file_cookie, struct dirent* entry ) {
    int ret;
    kdbgfs_node_t* node;
    kdbgfs_dir_cookie_t* cookie;

    node = ( kdbgfs_node_t* )_node;

    if ( node != root_node ) {
        return -ENOTDIR;
    }

    cookie = ( kdbgfs_dir_cookie_t* )file_cookie;

    mutex_lock( inode_lock, LOCK_IGNORE_SIGNAL );

    if ( cookie->position < ( hashtable_get_item_count( &inode_table ) - 1 ) ) {
        dir_iter_data_t data;

        data.current = 0;
        data.position = cookie->position++;
        data.entry = entry;

        hashtable_iterate( &inode_table, kdebugfs_read_dir_helper, &data );

        ret = 1;
    } else {
        ret = 0;
    }

    mutex_unlock( inode_lock );

    return ret;
}

static int kdebugfs_rewind_directory( void* fs_cookie, void* _node, void* file_cookie ) {
    kdbgfs_dir_cookie_t* cookie;

    cookie = ( kdbgfs_dir_cookie_t* )file_cookie;
    cookie->position = 0;

    return 0;
}

static filesystem_calls_t kdebugfs_calls = {
    .probe = NULL,
    .mount = kdebugfs_mount,
    .unmount = NULL,
    .read_inode = kdebugfs_read_inode,
    .write_inode = kdebugfs_write_inode,
    .lookup_inode = kdebugfs_lookup_inode,
    .open = kdebugfs_open,
    .close = kdebugfs_close,
    .free_cookie = NULL,
    .read = kdebugfs_read,
    .write = NULL,
    .ioctl = NULL,
    .read_stat = kdebugfs_read_stat,
    .write_stat = NULL,
    .read_directory = kdebugfs_read_directory,
    .rewind_directory = kdebugfs_rewind_directory,
    .create = NULL,
    .unlink = NULL,
    .mkdir = NULL,
    .rmdir = NULL,
    .isatty = NULL,
    .symlink = NULL,
    .readlink = NULL,
    .set_flags = NULL,
    .add_select_request = NULL,
    .remove_select_request = NULL
};

kdbgfs_node_t* kdebugfs_create_node( const char* name, size_t max_buffer_size ) {
    /* Limit the maximum buffer size ... */

    if ( max_buffer_size > 64 * 1024 ) {
        max_buffer_size = 64 * 1024;
    }

    /* todo: check if name already exists. */

    return kdebugfs_do_create_node( name, max_buffer_size );
}

int kdebugfs_write_node( kdbgfs_node_t* node, void* data, size_t size ) {
    mutex_lock( inode_lock, LOCK_IGNORE_SIGNAL );

    if ( ( node->buffer_size + size ) <= node->max_buffer_size ) {
        memcpy( node->buffer + node->buffer_size, data, size );
        node->buffer_size += size;
    } else {
        if ( size >= node->max_buffer_size ) {
            size_t start_offset = size - node->max_buffer_size;

            memcpy( node->buffer, ( char* )data + start_offset, node->max_buffer_size );
            node->buffer_size = node->max_buffer_size;
        } else {
            size_t free_size = node->max_buffer_size - node->buffer_size;
            size_t to_move = size - free_size;

            memmove( node->buffer, node->buffer + to_move, node->buffer_size - to_move );
            node->buffer_size -= to_move;

            memcpy( node->buffer + node->buffer_size, data, size );
            node->buffer_size += size;
        }
    }

    mutex_unlock( inode_lock );

    return 0;
}

static void* kdebugfs_inode_key( hashitem_t* item ) {
    kdbgfs_node_t* node;

    node = ( kdbgfs_node_t* )item;

    return ( void* )&node->inode_number;
}

__init int init_kdebugfs( void ) {
    int error;

    error = init_hashtable(
        &inode_table, 32,
        kdebugfs_inode_key, hash_int64,
        compare_int64
    );

    if ( error < 0 ) {
        return error;
    }

    inode_lock = mutex_create( "kdbgfs inode lock", MUTEX_NONE );

    if ( inode_lock < 0 ) {
        return inode_lock;
    }

    error = register_filesystem( "kdebugfs", &kdebugfs_calls );

    if ( error < 0 ) {
        return error;
    }

    return 0;
}
