/* RAM filesystem implementation
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

#include <errno.h>
#include <macros.h>
#include <mm/kmalloc.h>
#include <vfs/filesystem.h>
#include <vfs/vfs.h>
#include <lib/string.h>

#include "ramfs.h"

static ramfs_inode_t* ramfs_create_inode( ramfs_cookie_t* cookie, ramfs_inode_t* parent, const char* name, int name_length, bool is_directory ) {
    ramfs_inode_t* inode;

    inode = ( ramfs_inode_t* )kmalloc( sizeof( ramfs_inode_t ) );

    if ( inode == NULL ) {
        goto error1;
    }

    if ( name_length == -1 ) {
        inode->name = strdup( name );
    } else {
        inode->name = strndup( name, name_length );
    }

    if ( inode->name == NULL ) {
        goto error2;
    }

    inode->data = NULL;
    inode->size = 0;

    inode->link_count = 1;
    inode->is_loaded = false;

    inode->parent = parent;
    inode->first_children = NULL;
    inode->is_directory = is_directory;

    if ( parent != NULL ) {
        inode->next_sibling = parent->first_children;
        inode->prev_sibling = NULL;

        if ( parent->first_children != NULL ) {
            parent->first_children->prev_sibling = inode;
        }

        parent->first_children = inode;
    }

    do {
        inode->inode_number = cookie->inode_id_counter++;

        if ( cookie->inode_id_counter < 0 ) {
            cookie->inode_id_counter = 0;
        }
    } while ( hashtable_get( &cookie->inode_table, ( const void* )&inode->inode_number ) != NULL );

    hashtable_add( &cookie->inode_table, ( hashitem_t* )inode );

    return inode;

error2:
    kfree( inode );

error1:
    return NULL;
}

static void ramfs_delete_inode( ramfs_cookie_t* cookie, ramfs_inode_t* inode ) {
    if ( inode->link_count > 0 ) {
        if ( inode == inode->parent->first_children ) {
            inode->parent->first_children = inode->next_sibling;
        }

        if ( inode->prev_sibling != NULL ) {
            inode->prev_sibling->next_sibling = inode->next_sibling;
        }

        if ( inode->next_sibling != NULL ) {
            inode->next_sibling->prev_sibling = inode->prev_sibling;
        }

        inode->link_count--;
    }

    ASSERT( inode->link_count == 0 );

    if ( !inode->is_loaded ) {
        hashtable_remove( &cookie->inode_table, ( const void* )&inode->inode_number );

        kfree( inode->data );
        kfree( inode->name );
        kfree( inode );
    }
}

static void* ramfs_inode_key( hashitem_t* item ) {
    ramfs_inode_t* inode;

    inode = ( ramfs_inode_t* )item;

    return ( void* )&inode->inode_number;
}

static uint32_t ramfs_inode_hash( const void* key ) {
    return hash_number( ( uint8_t* )key, sizeof( ino_t ) );
}

static bool ramfs_inode_compare( const void* key1, const void* key2 ) {
    ino_t* inode_num_1;
    ino_t* inode_num_2;

    inode_num_1 = ( ino_t* )key1;
    inode_num_2 = ( ino_t* )key2;

    return ( *inode_num_1 == *inode_num_2 );
}

static int ramfs_mount( const char* device, uint32_t flags, void** fs_cookie, ino_t* root_inode_number ) {
    int error;
    ramfs_cookie_t* cookie;

    cookie = ( ramfs_cookie_t* )kmalloc( sizeof( ramfs_cookie_t ) );

    if ( cookie == NULL ) {
        error = -ENOMEM;
        goto error1;
    }

    error = init_hashtable(
        &cookie->inode_table,
        128,
        ramfs_inode_key,
        ramfs_inode_hash,
        ramfs_inode_compare
    );

    if ( error < 0 ) {
        goto error2;
    }

    cookie->inode_id_counter = 0;

    cookie->lock = create_semaphore( "ramfs lock", SEMAPHORE_BINARY, 0, 1 );

    if ( cookie->lock < 0 ) {
        error = cookie->lock;
        goto error3;
    }

    cookie->root_inode = ramfs_create_inode( cookie, NULL, "", 0, true );

    if ( cookie->root_inode == NULL ) {
        error = -ENOMEM;
        goto error4;
    }

    *fs_cookie = ( void* )cookie;
    *root_inode_number = cookie->root_inode->inode_number;

    return 0;

error4:
    delete_semaphore( cookie->lock );

error3:
    destroy_hashtable( &cookie->inode_table );

error2:
    kfree( cookie );

error1:
    return error;
}

static int ramfs_unmount( void* fs_cookie ) {
    return -ENOSYS;
}

static int ramfs_read_inode( void* fs_cookie, ino_t inode_number, void** node ) {
    ramfs_inode_t* inode;
    ramfs_cookie_t* cookie;

    cookie = ( ramfs_cookie_t* )fs_cookie;

    LOCK( cookie->lock );

    inode = ( ramfs_inode_t* )hashtable_get( &cookie->inode_table, ( const void* )&inode_number );

    if ( inode != NULL ) {
        ASSERT( !inode->is_loaded );

        inode->is_loaded = true;
    }

    UNLOCK( cookie->lock );

    if ( inode == NULL ) {
        return -ENOINO;
    }

    *node = ( void* )inode;

    return 0;
}

static int ramfs_write_inode( void* fs_cookie, void* node ) {
    ramfs_cookie_t* cookie;
    ramfs_inode_t* inode;

    cookie = ( ramfs_cookie_t* )fs_cookie;
    inode = ( ramfs_inode_t* )node;

    LOCK( cookie->lock );

    ASSERT( inode->is_loaded );

    inode->is_loaded = 0;

    if ( inode->link_count == 0 ) {
        ramfs_delete_inode( cookie, inode );
    }

    UNLOCK( cookie->lock );

    return 0;
}

static ramfs_inode_t* ramfs_do_lookup_inode( ramfs_inode_t* parent, const char* name, int name_length ) {
    ramfs_inode_t* inode;

    if ( ( name_length == 2 ) &&
         ( strncmp( name, "..", 2 ) == 0 ) ) {
        return parent->parent;
    }

    inode = parent->first_children;

    while ( inode != NULL ) {
        if ( ( strlen( inode->name ) == name_length ) &&
             ( strncmp( inode->name, name, name_length ) == 0 ) ) {
            break;
        }

        inode = inode->next_sibling;
    }

    return inode;
}

static int ramfs_lookup_inode( void* fs_cookie, void* _parent, const char* name, int name_length, ino_t* inode_number ) {
    ramfs_cookie_t* cookie;
    ramfs_inode_t* parent;
    ramfs_inode_t* inode;

    cookie = ( ramfs_cookie_t* )fs_cookie;
    parent = ( ramfs_inode_t* )_parent;

    LOCK( cookie->lock );

    inode = ramfs_do_lookup_inode( parent, name, name_length );

    UNLOCK( cookie->lock );

    if ( inode == NULL ) {
        return -ENOENT;
    }

    *inode_number = inode->inode_number;

    return 0;
}

static int ramfs_open_directory( ramfs_inode_t* inode, void** _cookie ) {
    ramfs_dir_cookie_t* cookie;

    cookie = ( ramfs_dir_cookie_t* )kmalloc( sizeof( ramfs_dir_cookie_t ) );

    if ( cookie == NULL ) {
        return -ENOMEM;
    }

    cookie->position = 0;

    *_cookie = ( void* )cookie;

    return 0;
}

static int ramfs_open_file( ramfs_inode_t* inode, void** _cookie, int flags ) {
    ramfs_file_cookie_t* cookie; 

    cookie = ( ramfs_file_cookie_t* )kmalloc( sizeof( ramfs_file_cookie_t ) );

    if ( cookie == NULL ) {
        return -ENOMEM;
    }

    cookie->open_flags = flags;

    *_cookie = ( void* )cookie;

    return 0;
}

static int ramfs_open( void* fs_cookie, void* node, int mode, void** file_cookie ) {
    ramfs_inode_t* inode;

    inode = ( ramfs_inode_t* )node;

    if ( inode->is_directory ) {
        return ramfs_open_directory( inode, file_cookie );
    } else {
        return ramfs_open_file( inode, file_cookie, mode );
    }
}

static int ramfs_close( void* fs_cookie, void* node, void* file_cookie ) {
    return 0;
}

static int ramfs_free_cookie( void* fs_cookie, void* node, void* file_cookie ) {
    kfree( file_cookie );

    return 0;
}

static int ramfs_read( void* fs_cookie, void* node, void* file_cookie, void* buffer, off_t pos, size_t size ) {
    int result;
    ramfs_cookie_t* cookie;
    ramfs_inode_t* inode;

    cookie = ( ramfs_cookie_t* )fs_cookie;
    inode = ( ramfs_inode_t* )node;

    if ( pos < 0 ) {
        return -EINVAL;
    }

    LOCK( cookie->lock );

    if ( ( inode->data == NULL ) ||
         ( pos >= inode->size ) ) {
        result = 0;
        goto out;
    }

    if ( pos + size > inode->size ) {
        size = inode->size - pos;
    }

    if ( size > 0 ) {
        memcpy( buffer, ( uint8_t* )inode->data + pos, size );
    }

    result = size;

out:
    UNLOCK( cookie->lock );

    return result;
}

static int ramfs_write( void* fs_cookie, void* node, void* _file_cookie, const void* buffer, off_t pos, size_t size ) {
    int result;
    ramfs_cookie_t* cookie;
    ramfs_file_cookie_t* file_cookie;
    ramfs_inode_t* inode;

    cookie = ( ramfs_cookie_t* )fs_cookie;
    file_cookie = ( ramfs_file_cookie_t* )_file_cookie;
    inode = ( ramfs_inode_t* )node;

    if ( pos < 0 ) {
        return -EINVAL;
    }

    if ( size == 0 ) {
        return 0;
    }

    LOCK( cookie->lock );

    if ( file_cookie->open_flags & O_APPEND ) {
        pos = inode->size;
    }

    if ( ( ( pos + size ) > inode->size ) ||
         ( inode->data == NULL ) ) {
        void* new_data;

        new_data = kmalloc( pos + size );

        if ( new_data == NULL ) {
            result = -ENOMEM;
            goto out;
        }

        if ( inode->data != NULL ) {
            memcpy( new_data, inode->data, inode->size );
            kfree( inode->data );
        }

        inode->data = new_data;
        inode->size = pos + size;
    }

    memcpy( ( uint8_t* )inode->data + pos, buffer, size );

out:
    UNLOCK( cookie->lock );

    return result;
}

static int ramfs_read_stat( void* fs_cookie, void* node, struct stat* stat ) {
    ramfs_cookie_t* cookie;
    ramfs_inode_t* inode;

    cookie = ( ramfs_cookie_t* )fs_cookie;
    inode = ( ramfs_inode_t* )node;

    LOCK( cookie->lock );

    stat->st_ino = inode->inode_number;
    stat->st_size = inode->size;

    if ( inode->is_directory ) {
        stat->st_mode |= S_IFDIR;
    } else {
        stat->st_mode |= S_IFREG;
    }

    UNLOCK( cookie->lock );

    return 0;
}

static int ramfs_write_stat( void* fs_cookie, void* node, struct stat* stat, uint32_t mask ) {
    return -ENOSYS;
}

static int ramfs_read_directory( void* fs_cookie, void* node, void* file_cookie, struct dirent* entry ) {
    int result;
    int current;
    ramfs_cookie_t* cookie;
    ramfs_dir_cookie_t* dir_cookie;
    ramfs_inode_t* parent;
    ramfs_inode_t* inode;

    current = 0;

    cookie = ( ramfs_cookie_t* )fs_cookie;
    dir_cookie = ( ramfs_dir_cookie_t* )file_cookie;
    parent = ( ramfs_inode_t* )node;

    ASSERT( parent->is_directory );

    LOCK( cookie->lock );

    inode = parent->first_children;

    while ( inode != NULL ) {
        if ( current == dir_cookie->position ) {
            break;
        }

        current++;
        inode = inode->next_sibling;
    }

    if ( inode != NULL ) {
        entry->inode_number = inode->inode_number;
        strncpy( entry->name, inode->name, NAME_MAX );
        entry->name[ NAME_MAX - 1 ] = 0;

        result = 1;
        dir_cookie->position++;
    } else {
        result = 0;
    }

    UNLOCK( cookie->lock );

    return result;
}

static int ramfs_create( void* fs_cookie, void* node, const char* name, int name_length, int mode, int perms, ino_t* inode_number, void** file_cookie ) {
    int error = 0;
    ramfs_cookie_t* cookie;
    ramfs_inode_t* parent;
    ramfs_inode_t* new_inode;

    cookie = ( ramfs_cookie_t* )fs_cookie;
    parent = ( ramfs_inode_t* )node;

    LOCK( cookie->lock );

    new_inode = ramfs_do_lookup_inode( parent, name, name_length );

    if ( new_inode != NULL ) {
        error = -EEXIST;
        goto out;
    }

    new_inode = ramfs_create_inode( cookie, parent, name, name_length, false );

    if ( new_inode == NULL ) {
        error = -ENOMEM;
        goto out;
    }

    error = ramfs_open_file( new_inode, file_cookie, mode );

    if ( error < 0 ) {
        ramfs_delete_inode( cookie, new_inode );
        goto out;
    }

    *inode_number = new_inode->inode_number;

out:
    UNLOCK( cookie->lock );

    return error;
}

static int ramfs_unlink( void* fs_cookie, void* node, const char* name, int name_length ) {
    int error = 0;
    ramfs_cookie_t* cookie;
    ramfs_inode_t* parent;
    ramfs_inode_t* inode;

    cookie = ( ramfs_cookie_t* )fs_cookie;
    parent = ( ramfs_inode_t* )node;

    LOCK( cookie->lock );

    inode = ramfs_do_lookup_inode( parent, name, name_length );

    if ( inode == NULL ) {
        error = -ENOENT;
        goto out;
    }

    if ( inode->is_directory ) {
        error = -EISDIR;
        goto out;
    }

    ramfs_delete_inode( cookie, inode );

out:
    UNLOCK( cookie->lock );

    return error;
}

static int ramfs_mkdir( void* fs_cookie, void* node, const char* name, int name_length, int permissions ) {
    int error = 0;
    ramfs_cookie_t* cookie;
    ramfs_inode_t* new_inode;
    ramfs_inode_t* parent;

    cookie = ( ramfs_cookie_t* )fs_cookie;
    parent = ( ramfs_inode_t* )node;

    if ( !parent->is_directory ) {
        return -EINVAL;
    }

    LOCK( cookie->lock );

    new_inode = ramfs_do_lookup_inode( parent, name, name_length );

    if ( new_inode != NULL ) {
        error = -EEXIST;
        goto out;
    }

    new_inode = ramfs_create_inode( cookie, parent, name, name_length, true );

    if ( new_inode == NULL ) {
        error = -ENOMEM;
        goto out;
    }

out:
    UNLOCK( cookie->lock );

    return error;
}

static int ramfs_rmdir( void* fs_cookie, void* node, const char* name, int name_length ) {
    int error = 0;
    ramfs_cookie_t* cookie;
    ramfs_inode_t* parent;
    ramfs_inode_t* inode;

    cookie = ( ramfs_cookie_t* )fs_cookie;
    parent = ( ramfs_inode_t* )node;

    LOCK( cookie->lock );

    inode = ramfs_do_lookup_inode( parent, name, name_length );

    if ( inode == NULL ) {
        error = -ENOENT;
        goto out;
    }

    if ( !inode->is_directory ) {
        error = -ENOTDIR;
        goto out;
    }

    if ( inode->first_children != NULL ) {
        error = -ENOTEMPTY;
        goto out;
    }

    ramfs_delete_inode( cookie, inode );

out:
    UNLOCK( cookie->lock );

    return error;
}

static int ramfs_symlink( void* fs_cookie, void* node, const char* name, int name_length, const char* link_path ) {
    return -ENOSYS;
}

static int ramfs_readlink( void* fs_cookie, void* node, char* buffer, size_t length ) {
    return 0;
}

static filesystem_calls_t ramfs_calls = {
    .probe = NULL,
    .mount = ramfs_mount,
    .unmount = ramfs_unmount,
    .read_inode = ramfs_read_inode,
    .write_inode = ramfs_write_inode,
    .lookup_inode = ramfs_lookup_inode,
    .open = ramfs_open,
    .close = ramfs_close,
    .free_cookie = ramfs_free_cookie,
    .read = ramfs_read,
    .write = ramfs_write,
    .ioctl = NULL,
    .read_stat = ramfs_read_stat,
    .write_stat = ramfs_write_stat,
    .read_directory = ramfs_read_directory,
    .create = ramfs_create,
    .unlink = ramfs_unlink,
    .mkdir = ramfs_mkdir,
    .rmdir = ramfs_rmdir,
    .isatty = NULL,
    .symlink = ramfs_symlink,
    .readlink = ramfs_readlink,
    .add_select_request = NULL,
    .remove_select_request = NULL
};

int init_module( void ) {
    int error;

    error = register_filesystem( "ramfs", &ramfs_calls );

    if ( error < 0 ) {
        return error;
    }

    return 0;
}

int destroy_module( void ) {
    return 0;
}
