/* Device file system
 *
 * Copyright (c) 2008 Zoltan Kovacs
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
#include <mm/kmalloc.h>
#include <vfs/devfs.h>
#include <vfs/filesystem.h>
#include <vfs/vfs.h>
#include <lib/string.h>

static ino_t devfs_inode_counter = 0;
static hashtable_t devfs_node_table;

static devfs_node_t* devfs_create_node( devfs_node_t* parent, const char* name, int length, bool is_directory ) {
    int error;
    devfs_node_t* node;

    /* Create a new node */

    node = ( devfs_node_t* )kmalloc( sizeof( devfs_node_t ) );

    if ( node == NULL ) {
        return NULL;
    }

    /* Initialize the node */

    if ( length == -1 ) {
        node->name = strdup( name );
    } else {
        node->name = strndup( name, length );
    }

    if ( node->name == NULL ) {
        kfree( node );
        return NULL;
    }

    node->is_directory = is_directory;
    node->parent = parent;
    node->next_sibling = NULL;
    node->first_child = NULL;

    if ( parent != NULL ) {
        node->next_sibling = parent->first_child;
        parent->first_child = node;
    }

    /* Insert to the node table */

    do {
        node->inode_number = devfs_inode_counter++;

        if ( devfs_inode_counter < 0 ) {
            devfs_inode_counter = 0;
        }
    } while ( hashtable_get( &devfs_node_table, ( const void* )&node->inode_number ) != NULL );

    error = hashtable_add( &devfs_node_table, ( hashitem_t* )node );

    if ( error < 0 ) {
        kfree( node->name );
        kfree( node );
        return NULL;
    }

    return node;
}

static int devfs_mount( const char* device, uint32_t flags, void** fs_cookie, ino_t* root_inode_num ) {
    devfs_node_t* root_node;

    root_node = devfs_create_node( NULL, "", -1, true );

    if ( root_node == NULL ) {
        return -ENOMEM;
    }

    *root_inode_num = root_node->inode_number;

    return 0;
}

static int devfs_read_inode( void* fs_cookie, ino_t inode_num, void** _node ) {
    devfs_node_t* node;

    node = ( devfs_node_t* )hashtable_get( &devfs_node_table, ( const void* )&inode_num );

    if ( node == NULL ) {
        return -ENOENT;
    }

    *_node = ( void* )node;

    return 0;
}

static int devfs_write_inode( void* fs_cookie, void* node ) {
    return 0;
}

static int devfs_lookup_inode( void* fs_cookie, void* _parent, const char* name, int name_len, ino_t* inode_num ) {
    devfs_node_t* node;
    devfs_node_t* parent;

    parent = ( devfs_node_t* )_parent;
    node = parent->first_child;

    while ( node != NULL ) {
        if ( strncmp( node->name, name, name_len ) == 0 ) {
            *inode_num = node->inode_number;
            return 0;
        }

        node = node->next_sibling;
    }

    return -ENOENT;
}

static int devfs_open_directory( devfs_node_t* node, int mode, void** cookie ) {
    devfs_dir_cookie_t* dir_cookie;

    dir_cookie = ( devfs_dir_cookie_t* )kmalloc( sizeof( devfs_dir_cookie_t ) );

    if ( dir_cookie == NULL ) {
        return -ENOMEM;
    }

    dir_cookie->position = 0;

    *cookie = ( void* )dir_cookie;

    return 0;
}

static int devfs_open( void* fs_cookie, void* _node, int mode, void** file_cookie ) {
    devfs_node_t* node;

    node = ( devfs_node_t* )_node;

    if ( node->is_directory ) {
        return devfs_open_directory( node, mode, file_cookie );
    }

    return -ENOSYS;

}

static int devfs_read_directory( void* fs_cookie, void* _node, void* file_cookie, struct dirent* entry ) {
    int current;
    devfs_node_t* node;
    devfs_node_t* child;
    devfs_dir_cookie_t* cookie;

    node = ( devfs_node_t* )_node;

    if ( !node->is_directory ) {
        return -EINVAL;
    }

    cookie = ( devfs_dir_cookie_t* )file_cookie;

    child = node->first_child;
    current = 0;

    while ( child != NULL ) {
        if ( current == cookie->position ) {
            strncpy( entry->name, child->name, NAME_MAX );
            entry->name[ NAME_MAX ] = 0;

            cookie->position++;

            return 1;
        }

        current++;
        child = child->next_sibling;
    }

    return 0;
}

static filesystem_calls_t devfs_calls = {
    .probe = NULL,
    .mount = devfs_mount,
    .unmount = NULL,
    .read_inode = devfs_read_inode,
    .write_inode = devfs_write_inode,
    .lookup_inode = devfs_lookup_inode,
    .open = devfs_open,
    .close = NULL,
    .free_cookie = NULL,
    .read = NULL,
    .write = NULL,
    .read_directory = devfs_read_directory,
    .mkdir = NULL
};

static void* devfs_node_key( hashitem_t* item ) {
    devfs_node_t* node;

    node = ( devfs_node_t* )item;

    return ( void* )&node->inode_number;
}

static uint32_t devfs_node_hash( const void* key ) {
    return hash_number( ( uint8_t* )key, sizeof( ino_t ) );
}

static bool devfs_node_compare( const void* key1, const void* key2 ) {
    ino_t* inode_num_1;
    ino_t* inode_num_2;

    inode_num_1 = ( ino_t* )key1;
    inode_num_2 = ( ino_t* )key2;

    return ( *inode_num_1 == *inode_num_2 );
}

int init_devfs( void ) {
    int error;

    error = init_hashtable(
        &devfs_node_table,
        64,
        devfs_node_key,
        devfs_node_hash,
        devfs_node_compare
    );

    if ( error < 0 ) {
        return error;
    }

    error = register_filesystem( "devfs", &devfs_calls );

    if ( error < 0 ) {
        return error;
    }

    return 0;
}
