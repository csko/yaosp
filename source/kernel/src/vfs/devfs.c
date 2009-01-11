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

static devfs_node_t* devfs_root_node = NULL;

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
    node->calls = NULL;

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
    devfs_root_node = devfs_create_node( NULL, "", -1, true );

    if ( devfs_root_node == NULL ) {
        return -ENOMEM;
    }

    *root_inode_num = devfs_root_node->inode_number;

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

    /* First check for ".." */

    if ( ( name_len == 2 ) && ( strncmp( name, "..", 2 ) == 0 ) ) {
        if ( parent->parent == NULL ) {
            return -EINVAL;
        }

        *inode_num = parent->parent->inode_number;

        return 0;
    }

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
    int error;
    devfs_node_t* node;

    node = ( devfs_node_t* )_node;

    if ( node->is_directory ) {
        error = devfs_open_directory( node, mode, file_cookie );
    } else {
        if ( node->calls->open == NULL ) {
            error = 0;
        } else {
            error = node->calls->open( node->cookie, 0, file_cookie );
        }
    }

    return error;
}

static int devfs_close( void* fs_cookie, void* _node, void* file_cookie ) {
    int error;
    devfs_node_t* node;

    node = ( devfs_node_t* )_node;

    if ( node->is_directory ) {
        error = 0;
    } else {
        if ( node->calls->close == NULL ) {
            error = 0;
        } else {
            error = node->calls->close( node->cookie, file_cookie );
        }
    }

    return error;
}

static int devfs_read( void* fs_cookie, void* _node, void* file_cookie, void* buffer, off_t pos, size_t size ) {
    devfs_node_t* node;

    node = ( devfs_node_t* )_node;

    if ( node->is_directory ) {
        return -EISDIR;
    }

    if ( node->calls->read == NULL ) {
        return -ENOSYS;
    } else {
        return node->calls->read( node->cookie, file_cookie, buffer, pos, size );
    }
}

static int devfs_write( void* fs_cookie, void* _node, void* file_cookie, const void* buffer, off_t pos, size_t size ) {
    devfs_node_t* node;

    node = ( devfs_node_t* )_node;

    if ( node->is_directory ) {
        return -EISDIR;
    }

    if ( node->calls->write == NULL ) {
        return -ENOSYS;
    } else {
        return node->calls->write( node->cookie, file_cookie, buffer, pos, size );
    }
}

static int devfs_ioctl( void* fs_cookie, void* _node, void* file_cookie, int command, void* buffer, bool from_kernel ) {
    devfs_node_t* node;

    node = ( devfs_node_t* )_node;

    if ( node->is_directory ) {
        return -EISDIR;
    }

    if ( node->calls->ioctl == NULL ) {
        return -ENOSYS;
    } else {
        return node->calls->ioctl( node->cookie, file_cookie, command, buffer, from_kernel );
    }
}

static int devfs_read_stat( void* fs_cookie, void* _node, struct stat* stat ) {
    devfs_node_t* node;

    node = ( devfs_node_t* )_node;

    stat->st_ino = node->inode_number;
    stat->st_mode = 0;
    stat->st_size = 0;

    if ( node->is_directory ) {
        stat->st_mode |= S_IFDIR;
    }

    return 0;
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

int create_device_node( const char* path, device_calls_t* calls, void* cookie ) {
    int error;
    char* sep;
    int node_length;
    devfs_node_t* parent;
    devfs_node_t* node;
    ino_t dummy;

    parent = devfs_root_node;

    /* Go through the directories */

    while ( true ) {
        size_t length;

        sep = strchr( path, '/' );

        if ( sep == NULL ) {
            break;
        }

        length = sep - path;

        node = parent->first_child;

        while ( node != NULL ) {
            if ( ( strlen( node->name ) == length ) &&
                 ( strncmp( node->name, path, length ) == 0 ) ) {
                break;
            }

            node = node->next_sibling;
        }

        if ( node == NULL ) {
            node = devfs_create_node( parent, path, length, true );

            if ( node == NULL ) {
                return -ENOMEM;
            }
        }

        parent = node;
        path = sep + 1;
    }

    /* Check if the node is already exists */

    node_length = strlen( path );

    if ( node_length == 0 ) {
        return -EINVAL;
    }

    error = devfs_lookup_inode( NULL, parent, path, node_length, &dummy );

    if ( error == 0 ) {
        return -EEXIST;
    }

    node = devfs_create_node( parent, path, node_length, false );

    if ( node == NULL ) {
        return -ENOMEM;
    }

    node->calls = calls;
    node->cookie = cookie;

    return 0;
}

static int devfs_mkdir( void* fs_cookie, void* _node, const char* name, int name_len, int permissions ) {
    int error;
    ino_t dummy;
    devfs_node_t* node;
    devfs_node_t* new_node;

    /* Check if this name already exists */

    error = devfs_lookup_inode( fs_cookie, _node, name, name_len, &dummy );

    if ( error == 0 ) {
        return -EEXIST;
    }

    node = ( devfs_node_t* )_node;

    if ( !node->is_directory ) {
        return -EINVAL;
    }

    /* Create the new node */

    new_node = devfs_create_node(
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

static int devfs_add_select_request( void* fs_cookie, void* _node, void* file_cookie, select_request_t* request ) {
    int error;
    devfs_node_t* node;

    node = ( devfs_node_t* )_node;

    if ( node->is_directory ) {
        return -EISDIR;
    }

    if ( node->calls->add_select_request == NULL ) {
        switch ( ( int )request->type ) {
            case SELECT_READ :
            case SELECT_WRITE :
                request->ready = true;
                UNLOCK( request->sync );
                break;
        }

        error = 0;
    } else {
        error = node->calls->add_select_request( node->cookie, file_cookie, request );
    }

    return error;
}

static int devfs_remove_select_request( void* fs_cookie, void* _node, void* file_cookie, select_request_t* request ) {
    int error;
    devfs_node_t* node;

    node = ( devfs_node_t* )_node;

    if ( node->is_directory ) {
        return -EISDIR;
    }

    if ( node->calls->remove_select_request == NULL ) {
        error = 0;
    } else {
        error = node->calls->remove_select_request( node->cookie, file_cookie, request );
    }

    return error;
}

static filesystem_calls_t devfs_calls = {
    .probe = NULL,
    .mount = devfs_mount,
    .unmount = NULL,
    .read_inode = devfs_read_inode,
    .write_inode = devfs_write_inode,
    .lookup_inode = devfs_lookup_inode,
    .open = devfs_open,
    .close = devfs_close,
    .free_cookie = NULL,
    .read = devfs_read,
    .write = devfs_write,
    .ioctl = devfs_ioctl,
    .read_stat = devfs_read_stat,
    .read_directory = devfs_read_directory,
    .create = NULL,
    .mkdir = devfs_mkdir,
    .isatty = NULL,
    .add_select_request = devfs_add_select_request,
    .remove_select_request = devfs_remove_select_request
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
