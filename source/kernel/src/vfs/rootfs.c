/* Root file system
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

#include <types.h>
#include <errno.h>
#include <scheduler.h>
#include <mm/kmalloc.h>
#include <vfs/rootfs.h>
#include <vfs/vfs.h>
#include <lib/string.h>
#include <lib/hashtable.h>

static ino_t rootfs_inode_counter = 0;
static hashtable_t rootfs_node_table;

static rootfs_node_t* rootfs_create_node( rootfs_node_t* parent, const char* name, int length, bool is_directory ) {
    int error;
    rootfs_node_t* node;

    /* Create a new node */

    node = ( rootfs_node_t* )kmalloc( sizeof( rootfs_node_t ) );

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
        node->inode_number = rootfs_inode_counter++;

        if ( rootfs_inode_counter < 0 ) {
            rootfs_inode_counter = 0;
        }
    } while ( hashtable_get( &rootfs_node_table, ( const void* )&node->inode_number ) != NULL );

    error = hashtable_add( &rootfs_node_table, ( hashitem_t* )node );

    if ( error < 0 ) {
        kfree( node->name );
        kfree( node );
        return NULL;
    }

    return node;
}

static void rootfs_delete_node( rootfs_node_t* node ) {
    /* Remove from the node table */

    hashtable_remove( &rootfs_node_table, ( const void* )&node->inode_number );

    /* Free the node */

    kfree( node->name );
    kfree( node );
}

static int rootfs_read_inode( void* fs_cookie, ino_t inode_num, void** _node ) {
    rootfs_node_t* node;

    node = ( rootfs_node_t* )hashtable_get( &rootfs_node_table, ( const void* )&inode_num );

    if ( node == NULL ) {
        return -ENOENT;
    }

    *_node = ( void* )node;

    return 0;
}

static int rootfs_write_inode( void* fs_cookie, void* node ) {
    return 0;
}

static int rootfs_lookup_inode( void* fs_cookie, void* _parent, const char* name, int name_len, ino_t* inode_num ) {
    rootfs_node_t* node;
    rootfs_node_t* parent;

    parent = ( rootfs_node_t* )_parent;
    node = parent->first_child;

    while ( node != NULL ) {
        if ( ( strlen( node->name ) == name_len ) &&
             ( strncmp( node->name, name, name_len ) == 0 ) ) {
            *inode_num = node->inode_number;
            return 0;
        }

        node = node->next_sibling;
    }

    return -ENOENT;
}

static int rootfs_open_directory( rootfs_node_t* node, int mode, void** cookie ) {
    rootfs_dir_cookie_t* dir_cookie;

    dir_cookie = ( rootfs_dir_cookie_t* )kmalloc( sizeof( rootfs_dir_cookie_t ) );

    if ( dir_cookie == NULL ) {
        return -ENOMEM;
    }

    dir_cookie->position = 0;

    *cookie = ( void* )dir_cookie;

    return 0;
}

static int rootfs_open( void* fs_cookie, void* _node, int mode, void** file_cookie ) {
    rootfs_node_t* node;

    node = ( rootfs_node_t* )_node;

    if ( node->is_directory ) {
        return rootfs_open_directory( node, mode, file_cookie );
    }

    return -ENOSYS;
}

static int rootfs_close( void* fs_cookie, void* node, void* file_cookie ) {
    return 0;
}

static int rootfs_free_cookie( void* fs_cookie, void* node, void* file_cookie ) {
    return 0;
}

static int rootfs_read_directory( void* fs_cookie, void* _node, void* file_cookie, struct dirent* entry ) {
    int current;
    rootfs_node_t* node;
    rootfs_node_t* child;
    rootfs_dir_cookie_t* cookie;

    node = ( rootfs_node_t* )_node;

    if ( !node->is_directory ) {
        return -EINVAL;
    }

    cookie = ( rootfs_dir_cookie_t* )file_cookie;

    child = node->first_child;
    current = 0;

    while ( child != NULL ) {
        if ( current == cookie->position ) {
            entry->inode_number = child->inode_number;

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

static int rootfs_mkdir( void* fs_cookie, void* _node, const char* name, int name_len, int permissions ) {
    int error;
    ino_t dummy;
    rootfs_node_t* node;
    rootfs_node_t* new_node;

    /* Check if this name already exists */

    error = rootfs_lookup_inode( fs_cookie, _node, name, name_len, &dummy );

    if ( error == 0 ) {
        return -EEXIST;
    }

    node = ( rootfs_node_t* )_node;

    if ( !node->is_directory ) {
        return -EINVAL;
    }

    /* Create the new node */

    new_node = rootfs_create_node(
        node,
        name,
        name_len,
        true
    );

    if ( new_node == NULL ) {
        return -ENOMEM;
    }

    return 0;
}

static filesystem_calls_t rootfs_calls = {
    .probe = NULL,
    .mount = NULL,
    .unmount = NULL,
    .read_inode = rootfs_read_inode,
    .write_inode = rootfs_write_inode,
    .lookup_inode = rootfs_lookup_inode,
    .open = rootfs_open,
    .close = rootfs_close,
    .free_cookie = rootfs_free_cookie,
    .read = NULL,
    .write = NULL,
    .read_directory = rootfs_read_directory,
    .mkdir = rootfs_mkdir
};

static void* rootfs_node_key( hashitem_t* item ) {
    rootfs_node_t* node;

    node = ( rootfs_node_t* )item;

    return ( void* )&node->inode_number;
}

static uint32_t rootfs_node_hash( const void* key ) {
    return hash_number( ( uint8_t* )key, sizeof( ino_t ) );
}

static bool rootfs_node_compare( const void* key1, const void* key2 ) {
    ino_t* inode_num_1;
    ino_t* inode_num_2;

    inode_num_1 = ( ino_t* )key1;
    inode_num_2 = ( ino_t* )key2;

    return ( *inode_num_1 == *inode_num_2 );
}

int init_root_filesystem( void ) {
    int error;
    rootfs_node_t* root_node;
    mount_point_t* mount_point;

    /* Initialize the rootfs node hashtable */

    error = init_hashtable(
        &rootfs_node_table,
        32,
        rootfs_node_key,
        rootfs_node_hash,
        rootfs_node_compare
    );

    if ( error < 0 ) {
        return error;
    }

    /* Create the root node */

    root_node = rootfs_create_node( NULL, "", -1, true );

    if ( root_node == NULL ) {
        destroy_hashtable( &rootfs_node_table );
        return -ENOMEM;
    }

    /* Create the root mount point */

    mount_point = create_mount_point(
        &rootfs_calls,
        32, /* initial inode cache size */
        4, /*  current free inodes */
        4 /* max free inodes */
    );

    if ( mount_point == NULL ) {
        destroy_hashtable( &rootfs_node_table );
        rootfs_delete_node( root_node );
        return -ENOMEM;
    }

    mount_point->fs_data = NULL;

    error = insert_mount_point( mount_point );

    if ( error < 0 ) {
        destroy_hashtable( &rootfs_node_table );
        rootfs_delete_node( root_node );
        delete_mount_point( mount_point );
        return error;
    }

    /* Initialize kernel I/O context */

    kernel_io_context.root_directory = get_inode( mount_point, root_node->inode_number );

    if ( kernel_io_context.root_directory == NULL ) {
        /* TODO: remove the mount point */
        destroy_hashtable( &rootfs_node_table );
        rootfs_delete_node( root_node );
        delete_mount_point( mount_point );
        return -EINVAL;
    }

    kernel_io_context.current_directory = kernel_io_context.root_directory;

    return 0;
}
