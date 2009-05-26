/* Inode related functions
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

#include <macros.h>
#include <errno.h>
#include <console.h>
#include <mm/kmalloc.h>
#include <vfs/inode.h>
#include <vfs/vfs.h>
#include <lib/string.h>

inode_t* get_inode( mount_point_t* mount_point, ino_t inode_number ) {
    int error;
    inode_t* inode;
    inode_cache_t* cache;
    void* tmp_fs_node;

    cache = &mount_point->inode_cache;

    LOCK( cache->lock );

    /* Get the inode from the hashtable */

    inode = ( inode_t* )hashtable_get( &cache->inode_table, ( const void* )&inode_number );

    /* If it is found, increment the reference counter and
       return it to the caller */

    if ( inode != NULL ) {
        atomic_inc( &inode->ref_count );

        UNLOCK( cache->lock );

        return inode;
    }

    /* The inode is not found so we have to load it. First
       allocate a new inode structure. We get it from the
       free inode list if it isn't empty, otherwise we use
       kmalloc to create it. */

    if ( cache->free_inode_count > 0 ) {
        ASSERT( cache->free_inodes != NULL );

        inode = cache->free_inodes;
        cache->free_inodes = inode->next_free;
        cache->free_inode_count--;

        ASSERT( cache->free_inode_count >= 0 );
    } else {
        inode = ( inode_t* )kmalloc( sizeof( inode_t ) );

        if ( inode == NULL ) {
            UNLOCK( cache->lock );

            return NULL;
        }

        memset( inode, 0, sizeof( inode_t ) );
    }

    inode->inode_number = inode_number;
    inode->mount_point = mount_point;
    inode->mount = NULL;
    atomic_set( &inode->ref_count, 1 );

    /* Read the inode from the filesystem */

    error = mount_point->fs_calls->read_inode(
        mount_point->fs_data,
        inode_number,
        ( void** )&inode->fs_node
    );

    /* In the case of an error we free the inode and
       return NULL */

    if ( error < 0 ) {
        if ( cache->free_inode_count < cache->max_free_inode_count ) {
            inode->next_free = cache->free_inodes;
            cache->free_inodes = inode;
            cache->free_inode_count++;
            inode = NULL;
        }

        UNLOCK( cache->lock );

        kfree( inode );

        return NULL;
    }

    /* Insert to the inode table */

    error = hashtable_add( &cache->inode_table, ( void* )inode );

    if ( error < 0 ) {
        tmp_fs_node = inode->fs_node;

        if ( cache->free_inode_count < cache->max_free_inode_count ) {
            inode->next_free = cache->free_inodes;
            cache->free_inodes = inode;
            cache->free_inode_count++;
            inode = NULL;
        }

        if ( mount_point->fs_calls->write_inode != NULL ) {
            mount_point->fs_calls->write_inode( mount_point->fs_data, tmp_fs_node );
        }

        UNLOCK( cache->lock );

        kfree( inode );

        return NULL;
    }

    UNLOCK( cache->lock );

    return inode;
}

int put_inode( inode_t* inode ) {
    void* tmp_fs_node;
    inode_cache_t* cache;
    mount_point_t* mount_point;

    mount_point = inode->mount_point;
    cache = &mount_point->inode_cache;

    LOCK( cache->lock );

    ASSERT( atomic_get( &inode->ref_count ) > 0 );

    if ( atomic_dec_and_test( &inode->ref_count ) ) {
        tmp_fs_node = inode->fs_node;

        /* Remove the inode from the cache table */

        hashtable_remove( &cache->inode_table, ( const void* )&inode->inode_number );

        /* Add the inode to the free list if it's not full */

        if ( cache->free_inode_count < cache->max_free_inode_count ) {
            inode->next_free = cache->free_inodes;
            cache->free_inodes = inode;
            cache->free_inode_count++;
            inode = NULL;
        }

        /* Free the inode */

        kfree( inode );

        /* Write the inode */

        if ( mount_point->fs_calls->write_inode != NULL ) {
            mount_point->fs_calls->write_inode( mount_point->fs_data, tmp_fs_node );
        }
    }

    UNLOCK( cache->lock );

    return 0;
}

int get_vnode( struct mount_point* mount_point, ino_t inode_number, void** data ) {
    inode_t* inode;

    inode = get_inode( mount_point, inode_number );

    if ( inode == NULL ) {
        return -ENOINO;
    }

    *data = inode->fs_node;

    return 0;
}

int put_vnode( struct mount_point* mount_point, ino_t inode_number ) {
    inode_t* inode;

    inode = get_inode( mount_point, inode_number );

    if ( inode == NULL ) {
        return -ENOINO;
    }

    atomic_dec( &inode->ref_count );

    put_inode( inode );

    return 0;
}

static int inode_table_unmount_iterator( hashitem_t* item, void* data ) {
    inode_t* inode;
    mount_point_t* mount_point;

    inode = ( inode_t* )item;
    mount_point = ( mount_point_t* )data;

    /* The filesystem is unmountable only if the root inode is the only one
       in the cache, and it has only 1 reference! */

    if ( ( inode->inode_number != mount_point->root_inode_number ) ||
         ( atomic_get( &inode->ref_count ) > 1 ) ) {
        return -EBUSY;
    }

    return 0;
}

int mount_point_can_unmount( mount_point_t* mount_point ) {
    return hashtable_iterate( &mount_point->inode_cache.inode_table, inode_table_unmount_iterator, ( void* )mount_point );
}

static void* inode_key( hashitem_t* item ) {
    inode_t* inode;

    inode = ( inode_t* )item;

    return ( void* )&inode->inode_number;
}

static uint32_t inode_hash( const void* key ) {
    return hash_number( ( uint8_t* )key, sizeof( ino_t ) );
}

static bool inode_compare( const void* key1, const void* key2 ) {
    ino_t* inode_num_1;
    ino_t* inode_num_2;

    inode_num_1 = ( ino_t* )key1;
    inode_num_2 = ( ino_t* )key2;

    return ( *inode_num_1 == *inode_num_2 );
}

int do_lookup_inode( io_context_t* io_context, inode_t* parent, const char* name, int name_length, bool follow_mount, inode_t** result ) {
    int error;
    inode_t* inode;
    ino_t inode_number;
    bool parent_changed;

    parent_changed = false;

    if ( ( name_length == 2 ) &&
         ( strncmp( name, "..", 2 ) == 0 ) &&
         ( parent->inode_number == parent->mount_point->root_inode_number ) ) {
        bool is_root;

        LOCK( io_context->lock );

        is_root = ( ( parent->inode_number == io_context->root_directory->inode_number ) &&
                    ( parent->mount_point == io_context->root_directory->mount_point ) );

        UNLOCK( io_context->lock );

        if ( is_root ) {
            atomic_inc( &parent->ref_count );

            *result = parent;

            return 0;
        } else {
            parent = parent->mount_point->mount_inode;
            parent_changed = true;

            atomic_inc( &parent->ref_count );
        }
    }

    error = parent->mount_point->fs_calls->lookup_inode(
        parent->mount_point->fs_data,
        parent->fs_node,
        name,
        name_length,
        &inode_number
    );

    if ( error < 0 ) {
        goto out;
    }

    inode = get_inode( parent->mount_point, inode_number );

    if ( inode == NULL ) {
        error = -ENOINO;
    } else {
        if ( ( follow_mount ) && ( inode->mount != NULL ) ) {
            inode_t* tmp;

            tmp = inode;
            inode = inode->mount;
            atomic_inc( &inode->ref_count );

            put_inode( tmp );
        }
    }

    *result = inode;

out:
    if ( parent_changed ) {
        put_inode( parent );
    }

    return error;
}

int lookup_parent_inode(
    io_context_t* io_context,
    inode_t* parent,
    const char* path,
    char** name,
    int* length,
    inode_t** _parent
) {
    int error;
    char* sep;

    LOCK( io_context->lock );

    if ( path[ 0 ] == '/' ) {
        parent = io_context->root_directory;
        path++;
    } else {
        if ( parent == NULL ) {
            parent = io_context->current_directory;
        }
    }

    if ( parent == NULL ) {
        UNLOCK( io_context->lock );

        return -EINVAL;
    }

    atomic_inc( &parent->ref_count );

    UNLOCK( io_context->lock );

    while ( true ) {
        int name_length;
        inode_t* inode;

        sep = strchr( path, '/' );

        if ( sep == NULL ) {
            break;
        }

        name_length = sep - path;

        if ( ( name_length == 0 ) ||
             ( ( name_length == 1 ) && ( path[ 0 ] == '.' ) ) ) {
            goto next;
        }

        error = do_lookup_inode( io_context, parent, path, name_length, true, &inode );

        if ( error == 0 ) {
            error = follow_symbolic_link( io_context, &parent, &inode );

            if ( error < 0 ) {
                put_inode( inode );
            }
        }

        put_inode( parent );

        if ( error < 0 ) {
            return error;
        }

        ASSERT( inode != NULL );

        parent = inode;

next:
        path = sep + 1;
    }

    *name = ( char* )path;
    *length = strlen( path );
    *_parent = parent;

    return 0;
}

int lookup_inode( io_context_t* io_context, inode_t* parent, const char* path, inode_t** _inode, bool follow_symlink, bool follow_mount ) {
    int error;
    char* name;
    int length;
    inode_t* inode;
    inode_t* new_parent;

    error = lookup_parent_inode( io_context, parent, path, &name, &length, &new_parent );

    if ( error < 0 ) {
        return error;
    }

    if ( ( length == 0 ) ||
         ( ( length == 1 ) && ( name[ 0 ] == '.' ) ) ) {
        error = 0;
        inode = new_parent;

        atomic_inc( &inode->ref_count );
    } else {
        error = do_lookup_inode( io_context, new_parent, name, length, follow_mount, &inode );
    }

    if ( ( error == 0 ) && ( follow_symlink ) ) {
        error = follow_symbolic_link( io_context, &new_parent, &inode );

        if ( error < 0 ) {
            put_inode( inode );
        }
    }

    put_inode( new_parent );

    if ( error < 0 ) {
        return error;
    }

    ASSERT( inode != NULL );

    *_inode = inode;

    return 0;
}

uint32_t get_inode_cache_size( inode_cache_t* cache ) {
    uint32_t size;

    LOCK( cache->lock );

    size = hashtable_get_item_count( &cache->inode_table );

    UNLOCK( cache->lock );

    return size;
}

int init_inode_cache( inode_cache_t* cache, int current_size, int free_inodes, int max_free_inodes ) {
    int i;
    int error;
    inode_t* inode;

    /* Initialize inode hashtable */

    error = init_hashtable(
        &cache->inode_table,
        current_size,
        inode_key,
        inode_hash,
        inode_compare
    );

    if ( error < 0 ) {
        return error;
    }

    /* Create the inode cache semaphore */

    cache->lock = create_semaphore( "inode cache lock", SEMAPHORE_BINARY, 0, 1 );

    if ( cache->lock < 0 ) {
        destroy_hashtable( &cache->inode_table );
        return cache->lock;
    }

    /* Create the initial free inodes */

    cache->free_inode_count = free_inodes;
    cache->max_free_inode_count = max_free_inodes;
    cache->free_inodes = NULL;

    for ( i = 0; i < free_inodes; i++ ) {
        inode = ( inode_t* )kmalloc( sizeof( inode_t ) );

        if ( inode == NULL ) {
            inode_t* tmp;

            while ( cache->free_inodes != NULL ) {
                tmp = cache->free_inodes;
                cache->free_inodes = tmp->next_free;

                kfree( tmp );
            }

            destroy_hashtable( &cache->inode_table );
            delete_semaphore( cache->lock );

            return -ENOMEM;
        }

        memset( inode, 0, sizeof( inode_t ) );

        atomic_set( &inode->ref_count, 0 );

        inode->next_free = cache->free_inodes;
        cache->free_inodes = inode;
    }

    return 0;
}

void destroy_inode_cache( inode_cache_t* cache ) {
    inode_t* inode;

    /* TODO: destroy loaded inodes? */

    destroy_hashtable( &cache->inode_table );
    delete_semaphore( cache->lock );

    inode = cache->free_inodes;

    while ( cache->free_inodes != NULL ) {
        inode = cache->free_inodes;
        cache->free_inodes = inode->next_free;

        kfree( inode );
    }
}
