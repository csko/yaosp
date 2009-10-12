/* Root file system
 *
 * Copyright (c) 2008, 2009 Zoltan Kovacs
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
#include <kernel.h>
#include <macros.h>
#include <time.h>
#include <lock/mutex.h>
#include <mm/kmalloc.h>
#include <vfs/rootfs.h>
#include <vfs/vfs.h>
#include <lib/string.h>
#include <lib/hashtable.h>

static lock_id rootfs_mutex;
static ino_t rootfs_inode_counter = 0;
static hashtable_t rootfs_node_table;

static rootfs_node_t* rootfs_create_node( rootfs_node_t* parent, const char* name, int length, bool is_directory ) {
    int error;
    rootfs_node_t* node;

    /* Create a new node */

    node = ( rootfs_node_t* )kmalloc( sizeof( rootfs_node_t ) );

    if ( node == NULL ) {
        goto error1;
    }

    /* Initialize the node */

    if ( length == -1 ) {
        node->name = strdup( name );
    } else {
        node->name = strndup( name, length );
    }

    if ( node->name == NULL ) {
        goto error2;
    }

    node->is_directory = is_directory;
    node->parent = parent;
    node->prev_sibling = NULL;
    node->next_sibling = NULL;
    node->first_child = NULL;
    node->atime = node->mtime = node->ctime = time( NULL );
    node->link_path = NULL;
    node->link_count = 1;
    node->is_loaded = false;

    /* Link the new node to the parent */

    if ( parent != NULL ) {
        node->next_sibling = parent->first_child;

        if ( parent->first_child != NULL ) {
            parent->first_child->prev_sibling = node;
        }

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
        goto error3;
    }

    return node;

error3:
    kfree( node->name );

error2:
    kfree( node );

error1:
    return NULL;
}

static void rootfs_delete_node( rootfs_node_t* node ) {
    if ( node->link_count > 0 ) {
        if ( node == node->parent->first_child ) {
            node->parent->first_child = node->next_sibling;
        }

        if ( node->prev_sibling != NULL ) {
            node->prev_sibling->next_sibling = node->next_sibling;
        }

        if ( node->next_sibling != NULL ) {
            node->next_sibling->prev_sibling = node->prev_sibling;
        }

        node->link_count--;
    }

    ASSERT( node->link_count == 0 );

    /* Remove from the node table */

    hashtable_remove( &rootfs_node_table, ( const void* )&node->inode_number );

    /* Free the node */

    if ( !node->is_loaded ) {
        kfree( node->name );
        kfree( node );
    }
}

static int rootfs_read_inode( void* fs_cookie, ino_t inode_num, void** _node ) {
    rootfs_node_t* node;

    mutex_lock( rootfs_mutex, LOCK_IGNORE_SIGNAL );

    node = ( rootfs_node_t* )hashtable_get( &rootfs_node_table, ( const void* )&inode_num );

    if ( __likely( node != NULL ) ) {
        ASSERT( !node->is_loaded );

        node->is_loaded = true;
    }

    mutex_unlock( rootfs_mutex );

    if ( node == NULL ) {
        return -ENOENT;
    }

    *_node = ( void* )node;

    return 0;
}

static int rootfs_write_inode( void* fs_cookie, void* _node ) {
    rootfs_node_t* node;

    node = ( rootfs_node_t* )_node;

    mutex_lock( rootfs_mutex, LOCK_IGNORE_SIGNAL );

    ASSERT( node->is_loaded );

    node->is_loaded = false;

    if ( node->link_count == 0 ) {
        rootfs_delete_node( node );
    }

    mutex_unlock( rootfs_mutex );

    return 0;
}

static int rootfs_do_lookup_inode( void* fs_cookie, void* _parent, const char* name, int name_length, ino_t* inode_number ) {
    rootfs_node_t* node;
    rootfs_node_t* parent;

    parent = ( rootfs_node_t* )_parent;
    node = parent->first_child;

    /* First check for ".." */

    if ( ( name_length == 2 ) && ( strncmp( name, "..", 2 ) == 0 ) ) {
        if ( parent->parent == NULL ) {
            return -EINVAL;
        }

        *inode_number = parent->parent->inode_number;

        return 0;
    }

    while ( node != NULL ) {
        if ( ( strlen( node->name ) == name_length ) &&
             ( strncmp( node->name, name, name_length ) == 0 ) ) {
            *inode_number = node->inode_number;

            return 0;
        }

        node = node->next_sibling;
    }

    return -ENOENT;
}

static int rootfs_lookup_inode( void* fs_cookie, void* _parent, const char* name, int name_length, ino_t* inode_number ) {
    int error;

    mutex_lock( rootfs_mutex, LOCK_IGNORE_SIGNAL );

    error = rootfs_do_lookup_inode( fs_cookie, _parent, name, name_length, inode_number );

    mutex_unlock( rootfs_mutex );

    return error;
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

static int rootfs_read_stat( void* fs_cookie, void* _node, struct stat* stat ) {
    rootfs_node_t* node;

    node = ( rootfs_node_t* )_node;

    mutex_lock( rootfs_mutex, LOCK_IGNORE_SIGNAL );

    stat->st_ino = node->inode_number;
    stat->st_mode = 0777;
    stat->st_size = 0;
    stat->st_atime = node->atime;
    stat->st_mtime = node->mtime;
    stat->st_ctime = node->ctime;

    if ( node->link_path != NULL ) {
        stat->st_mode |= S_IFLNK;
    } else {
        if ( node->is_directory ) {
            stat->st_mode |= S_IFDIR;
        } else {
            stat->st_mode |= S_IFREG;
        }
    }

    mutex_unlock( rootfs_mutex );

    return 0;
}

static int rootfs_write_stat( void* fs_cookie, void* _node, struct stat* stat, uint32_t mask ) {
    rootfs_node_t* node;

    node = ( rootfs_node_t* )_node;

    mutex_lock( rootfs_mutex, LOCK_IGNORE_SIGNAL );

    if ( mask & WSTAT_ATIME ) {
        node->atime = stat->st_atime;
    }

    if ( mask & WSTAT_MTIME ) {
        node->mtime = stat->st_mtime;
    }

    if ( mask & WSTAT_CTIME ) {
        node->ctime = stat->st_ctime;
    }

    mutex_unlock( rootfs_mutex );

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

    mutex_lock( rootfs_mutex, LOCK_IGNORE_SIGNAL );

    child = node->first_child;
    current = 0;

    while ( child != NULL ) {
        if ( current == cookie->position ) {
            entry->inode_number = child->inode_number;
            strncpy( entry->name, child->name, NAME_MAX );
            entry->name[ NAME_MAX ] = 0;

            mutex_unlock( rootfs_mutex );

            cookie->position++;

            return 1;
        }

        current++;
        child = child->next_sibling;
    }

    node->atime = time( NULL );

    mutex_unlock( rootfs_mutex );

    return 0;
}

static int rootfs_rewind_directory( void* fs_cookie, void* _node, void* file_cookie ) {
    rootfs_dir_cookie_t* cookie;

    cookie = ( rootfs_dir_cookie_t* )file_cookie;

    cookie->position = 0;

    return 0;
}

static int rootfs_mkdir( void* fs_cookie, void* _node, const char* name, int name_length, int permissions ) {
    int error;
    ino_t dummy;
    rootfs_node_t* node;
    rootfs_node_t* new_node;

    mutex_lock( rootfs_mutex, LOCK_IGNORE_SIGNAL );

    /* Check if this name already exists */

    error = rootfs_do_lookup_inode( fs_cookie, _node, name, name_length, &dummy );

    if ( error == 0 ) {
        error = -EEXIST;
        goto out;
    }

    node = ( rootfs_node_t* )_node;

    if ( !node->is_directory ) {
        error = -EINVAL;
        goto out;
    }

    /* Create the new node */

    new_node = rootfs_create_node(
        node,
        name,
        name_length,
        true
    );

    if ( new_node == NULL ) {
        error = -ENOMEM;
        goto out;
    }

    node->mtime = time( NULL );

    error = 0;

out:
    mutex_unlock( rootfs_mutex );

    return error;
}

static int rootfs_rmdir( void* fs_cookie, void* _node, const char* name, int name_length ) {
    int error;
    ino_t inode_number;
    rootfs_node_t* parent;
    rootfs_node_t* node;

    parent = ( rootfs_node_t* )_node;

    mutex_lock( rootfs_mutex, LOCK_IGNORE_SIGNAL );

    error = rootfs_do_lookup_inode( fs_cookie, _node, name, name_length, &inode_number );

    if ( error < 0 ) {
        goto out;
    }

    node = ( rootfs_node_t* )hashtable_get( &rootfs_node_table, ( const void* )&inode_number );

    if ( node == NULL ) {
        error = -ENOENT;
        goto out;
    }

    if ( !node->is_directory ) {
        error = -ENOTDIR;
        goto out;
    }

    if ( node->first_child != NULL ) {
        error = -ENOTEMPTY;
        goto out;
    }

    rootfs_delete_node( node );

    parent->mtime = time( NULL );

out:
    mutex_unlock( rootfs_mutex );

    return error;
}

static int rootfs_symlink( void* fs_cookie, void* _node, const char* name, int name_length, const char* link_path ) {
    int error;
    ino_t dummy;
    rootfs_node_t* node;
    rootfs_node_t* new_node;

    mutex_lock( rootfs_mutex, LOCK_IGNORE_SIGNAL );

    /* Check if this name already exists */

    error = rootfs_do_lookup_inode( fs_cookie, _node, name, name_length, &dummy );

    if ( error == 0 ) {
        error = -EEXIST;
        goto out;
    }

    node = ( rootfs_node_t* )_node;

    /* We can create symbolic links only in directories */

    if ( !node->is_directory ) {
        error = -EINVAL;
        goto out;
    }

    /* Create the new node */

    new_node = rootfs_create_node(
        node,
        name,
        name_length,
        true
    );

    if ( new_node == NULL ) {
        error = -ENOMEM;
        goto out;
    }

    /* Save the link path in the new node */

    new_node->link_path = strdup( link_path );

    if ( new_node->link_path == NULL ) {
        rootfs_delete_node( new_node );
        error = -ENOMEM;
        goto out;
    }

    error = 0;

    node->mtime = time( NULL );

out:
    mutex_unlock( rootfs_mutex );

    return error;
}

static int rootfs_readlink( void* fs_cookie, void* _node, char* buffer, size_t length ) {
    rootfs_node_t* node;

    node = ( rootfs_node_t* )_node;

    mutex_lock( rootfs_mutex, LOCK_IGNORE_SIGNAL );

    if ( node->link_path == NULL ) {
        mutex_unlock( rootfs_mutex );

        return -EINVAL;
    }

    strncpy( buffer, node->link_path, length );
    buffer[ length - 1 ] = 0;

    node->atime = time( NULL );

    mutex_unlock( rootfs_mutex );

    return strlen( buffer );
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
    .ioctl = NULL,
    .read_stat = rootfs_read_stat,
    .write_stat = rootfs_write_stat,
    .read_directory = rootfs_read_directory,
    .rewind_directory = rootfs_rewind_directory,
    .create = NULL,
    .unlink = NULL,
    .mkdir = rootfs_mkdir,
    .rmdir = rootfs_rmdir,
    .isatty = NULL,
    .symlink = rootfs_symlink,
    .readlink = rootfs_readlink,
    .set_flags = NULL,
    .add_select_request = NULL,
    .remove_select_request = NULL
};

static void* rootfs_node_key( hashitem_t* item ) {
    rootfs_node_t* node;

    node = ( rootfs_node_t* )item;

    return ( void* )&node->inode_number;
}

__init int init_root_filesystem( void ) {
    int error;
    rootfs_node_t* root_node;
    mount_point_t* mount_point;

    /* Create rootfs lock */

    rootfs_mutex = mutex_create( "rootfs mutex", MUTEX_NONE );

    if ( rootfs_mutex < 0 ) {
        return rootfs_mutex;
    }

    /* Initialize the rootfs node hashtable */

    error = init_hashtable(
        &rootfs_node_table,
        32,
        rootfs_node_key,
        hash_int64,
        compare_int64
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
        4, /* max free inodes */
        MOUNT_NONE /* rw */
    );

    if ( mount_point == NULL ) {
        destroy_hashtable( &rootfs_node_table );
        rootfs_delete_node( root_node );
        return -ENOMEM;
    }

    mount_point->fs_data = NULL;
    mount_point->root_inode_number = root_node->inode_number;
    mount_point->mount_inode = NULL;

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

    atomic_inc( &kernel_io_context.root_directory->ref_count );

    kernel_io_context.current_directory = kernel_io_context.root_directory;

    return 0;
}
