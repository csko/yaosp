/* Block cache
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
#include <vfs/blockcache.h>
#include <vfs/vfs.h>
#include <lib/string.h>

#include <arch/pit.h>

int block_cache_get_block( block_cache_t* cache, uint64_t block_index, void** buffer ) {
    int error;
    uint32_t i;
    uint8_t* data;
    block_t* block;
    uint64_t start_block;
    uint32_t block_count;

    if ( block_index >= cache->block_count ) {
        return -EINVAL;
    }

    mutex_lock( cache->mutex, LOCK_IGNORE_SIGNAL );

    block = ( block_t* )hashtable_get( &cache->block_table, ( const void* )&block_index );

    if ( block != NULL ) {
        block->last_access_time = get_system_time();

        *buffer = ( void* )( block + 1 );

        mutex_unlock( cache->mutex );

        return 0;
    }

    start_block = block_index;
    block_count = MIN( BLOCK_CACHE_READ_BUF_BLOCKS, cache->block_count - start_block );

    ASSERT( block_count > 0 );

    error = pread(
        cache->fd,
        cache->read_buffer,
        block_count * cache->block_size,
        start_block * cache->block_size
    );

    if ( error != block_count * cache->block_size ) {
        error = -EIO;
        goto error1;
    }

    for ( i = 0, data = ( uint8_t* )cache->read_buffer; i < block_count; i++, data += cache->block_size ) {
        block = ( block_t* )kmalloc( sizeof( block_t ) + cache->block_size );

        if ( block == NULL ) {
            error = -ENOMEM;
            goto error1;
        }

        block->block_index = start_block + i;
        block->last_access_time = get_system_time();

        memcpy( ( void* )( block + 1 ), data, cache->block_size );

        if ( block->block_index == block_index ) {
            *buffer = ( void* )( block + 1 );
        }

        error = hashtable_add( &cache->block_table, ( hashitem_t* )block );

        if ( error < 0 ) {
            goto error1;
        }
    }

    mutex_unlock( cache->mutex );

    return 0;

 error1:
    mutex_unlock( cache->mutex );

    return error;
}

int block_cache_put_block( block_cache_t* cache, uint64_t block_index ) {
    return 0;
}

int block_cache_read_blocks( block_cache_t* cache, uint64_t start_block, uint64_t block_count, void* buffer ) {
    if ( block_count == 0 ) {
        return -EINVAL;
    }

    mutex_lock( cache->mutex, LOCK_IGNORE_SIGNAL );

    /* TODO */

    mutex_unlock( cache->mutex );

    return 0;
}

static void* block_table_key( hashitem_t* item ) {
    block_t* block;

    block = ( block_t* )item;

    return ( void* )&block->block_index;
}

block_cache_t* init_block_cache( int fd, uint32_t block_size, uint64_t block_count ) {
    int error;
    block_cache_t* cache;

    if ( ( fd < 0 ) || ( block_size == 0 ) ) {
        goto error1;
    }

    cache = ( block_cache_t* )kmalloc( sizeof( block_cache_t ) + ( block_size * BLOCK_CACHE_READ_BUF_BLOCKS ) );

    if ( cache == NULL ) {
        goto error1;
    }

    cache->mutex = mutex_create( "block cache mutex", MUTEX_NONE );

    if ( cache->mutex < 0 ) {
        goto error2;
    }

    error = init_hashtable(
        &cache->block_table,
        1024,
        block_table_key,
        hash_int64,
        compare_int64
    );

    if ( error < 0 ) {
        goto error3;
    }

    cache->fd = fd;
    cache->block_size = block_size;
    cache->read_buffer = ( void* )( cache + 1 );
    cache->block_count = block_count;

    return cache;

 error3:
    mutex_destroy( cache->mutex );

 error2:
    kfree( cache );

 error1:
    return NULL;
}
