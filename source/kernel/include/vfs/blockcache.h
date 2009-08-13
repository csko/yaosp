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

#ifndef _VFS_BLOCKCACHE_H_
#define _VFS_BLOCKCACHE_H_

#include <types.h>
#include <lock/mutex.h>
#include <lib/hashtable.h>

#define BLOCK_CACHE_READ_BEFORE      8
#define BLOCK_CACHE_READ_BUF_BLOCKS 32

typedef struct block {
    hashitem_t hash;
    uint64_t block_index;
    uint64_t last_access_time;
} block_t;

typedef struct block_cache {
    int fd;
    lock_id mutex;
    uint32_t block_size;
    uint64_t block_count;
    hashtable_t block_table;
    void* read_buffer;
} block_cache_t;

int block_cache_get_block( block_cache_t* cache, uint64_t block_index, void** buffer );
int block_cache_put_block( block_cache_t* cache, uint64_t block_index );
int block_cache_read_blocks( block_cache_t* cache, uint64_t start_block, uint64_t block_count, void* buffer );

block_cache_t* init_block_cache( int fd, uint32_t block_size, uint64_t block_count );

#endif /* _VFS_BLOCKCACHE_H_ */
