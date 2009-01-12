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

        UNLOCK( cache->lock );

        if ( mount_point->fs_calls->write_inode != NULL ) {
            mount_point->fs_calls->write_inode( mount_point->fs_data, tmp_fs_node );
        }

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

    ASSERT( atomic_get( &inode->ref_count ) > 0 );

    mount_point = inode->mount_point;
    cache = &mount_point->inode_cache;

    if ( atomic_dec_and_test( &inode->ref_count ) ) {
        tmp_fs_node = inode->fs_node;

        LOCK( cache->lock );

        hashtable_remove( &cache->inode_table, ( const void* )&inode->inode_number );

        if ( cache->free_inode_count < cache->max_free_inode_count ) {
            inode->next_free = cache->free_inodes;
            cache->free_inodes = inode;
            cache->free_inode_count++;
            inode = NULL;
        }

        UNLOCK( cache->lock );

        if ( mount_point->fs_calls->write_inode != NULL ) {
            mount_point->fs_calls->write_inode( mount_point->fs_data, tmp_fs_node );
        }

        kfree( inode );
    }

    return 0;
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

int lookup_parent_inode( io_context_t* io_context, const char* path, char** name, int* length, inode_t** _parent ) {
    int error;
    char* sep;
    inode_t* parent;

    LOCK( io_context->lock );

    if ( path[ 0 ] == '/' ) {
        parent = io_context->root_directory;
        path++;
    } else {
        parent = io_context->current_directory;
    }

    if ( parent == NULL ) {
        UNLOCK( io_context->lock );

        return -EINVAL;
    }

    atomic_inc( &parent->ref_count );

    UNLOCK( io_context->lock );

    while ( true ) {
        int name_len;
        ino_t inode_number;
        inode_t* inode;

        sep = strchr( path, '/' );

        if ( sep == NULL ) {
            break;
        }

        name_len = sep - path;

        if ( ( name_len == 0 ) ||
             ( ( name_len == 1 ) && ( path[ 0 ] == '.' ) ) ) {
            goto next;
        }

        error = parent->mount_point->fs_calls->lookup_inode(
            parent->mount_point->fs_data,
            parent->fs_node,
            path,
            name_len,
            &inode_number
        );

        if ( error < 0 ) {    
            put_inode( parent );
            return error;
        }

        inode = get_inode( parent->mount_point, inode_number );

        put_inode( parent );

        if ( inode == NULL ) {
            return -ENOMEM;
        }

        /* Follow possible mount point */

        if ( inode->mount == NULL ) {
            parent = inode;
        } else {
            parent = inode->mount;
            atomic_inc( &parent->ref_count );
            put_inode( inode );
        }

next:
        path = sep + 1;
    }

    *name = ( char* )path;
    *length = strlen( path );
    *_parent = parent;

    return 0;
}

int lookup_inode( io_context_t* io_context, const char* path, inode_t** _inode ) {
    int error;
    char* name;
    int length;
    inode_t* inode;
    ino_t inode_number;
    inode_t* parent;

    error = lookup_parent_inode( io_context, path, &name, &length, &parent );

    if ( error < 0 ) {
        return error;
    }

    error = parent->mount_point->fs_calls->lookup_inode(
        parent->mount_point->fs_data,
        parent->fs_node,
        name,
        length,
        &inode_number
    );

    if ( error < 0 ) {
        put_inode( parent );
        return error;
    }

    inode = get_inode( parent->mount_point, inode_number );

    put_inode( parent );

    if ( inode == NULL ) {
        return -ENOMEM;
    }

    if ( inode->mount == NULL ) {
        *_inode = inode;
    } else {
        atomic_inc( &inode->mount->ref_count );
        *_inode = inode->mount;

        put_inode( inode );
    }

    return 0;
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

    cache->lock = create_semaphore( "inode_cache_lock", SEMAPHORE_BINARY, 0, 1 );

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
