/* Memory allocator
 *
 * Copyright (c) 2008, 2009 Zoltan Kovacs
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
#include <console.h>
#include <kernel.h>
#include <macros.h>
#include <mm/kmalloc.h>
#include <mm/pages.h>
#include <lib/string.h>

#include <arch/spinlock.h>
#include <arch/mm/config.h>

static uint32_t used_pages = 0;
static uint32_t alloc_size = 0;

static spinlock_t kmalloc_lock = INIT_SPINLOCK( "kmalloc" );
static kmalloc_block_t* root = NULL;

static kmalloc_block_t* __kmalloc_create_block( uint32_t pages ) {
    kmalloc_block_t* block;
    kmalloc_chunk_t* chunk;

    block = ( kmalloc_block_t* )alloc_pages( pages, MEM_COMMON );

    if ( __unlikely( block == NULL ) ) {
        return NULL;
    }

    used_pages += pages;

    block->magic = KMALLOC_BLOCK_MAGIC;
    block->pages = pages;
    block->next = NULL;
    block->first_chunk = ( kmalloc_chunk_t* )( block + 1 );

    chunk = block->first_chunk;

    chunk->magic = KMALLOC_CHUNK_MAGIC;
    chunk->type = CHUNK_FREE;
    chunk->block = block;
    chunk->size = pages * PAGE_SIZE - sizeof( kmalloc_block_t ) - sizeof( kmalloc_chunk_t );
    chunk->prev = NULL;
    chunk->next = NULL;

    block->biggest_free = chunk->size;

    return block;
}

static void* __kmalloc_from_block( kmalloc_block_t* block, uint32_t size ) {
    void* p;
    kmalloc_chunk_t* chunk;

    chunk = block->first_chunk;

    while ( ( chunk != NULL ) &&
            ( ( chunk->type != CHUNK_FREE ) ||
              ( chunk->size < size ) ) ) {
        ASSERT( chunk->magic == KMALLOC_CHUNK_MAGIC );

        chunk = chunk->next;
    }

    if ( __unlikely( chunk == NULL ) ) {
        return NULL;
    }

    chunk->type = CHUNK_ALLOCATED;
    chunk->real_size = size;
    p = ( void* )( chunk + 1 );

    if ( chunk->size > ( size + sizeof( kmalloc_chunk_t ) + 4 ) ) {
        uint32_t remaining_size;
        kmalloc_chunk_t* new_chunk;

        remaining_size = chunk->size - size - sizeof( kmalloc_chunk_t );

        chunk->size = size;

        new_chunk = ( kmalloc_chunk_t* )( ( uint8_t* )chunk + sizeof( kmalloc_chunk_t ) + size );

        new_chunk->magic = KMALLOC_CHUNK_MAGIC;
        new_chunk->type = CHUNK_FREE;
        new_chunk->size = remaining_size;
        new_chunk->block = chunk->block;

        /* link it to the current block */

        new_chunk->prev = chunk;
        new_chunk->next = chunk->next;
        chunk->next = new_chunk;

        if ( new_chunk->next != NULL ) {
            new_chunk->next->prev = new_chunk;
        }
    }

    /* recalculate the biggest free chunk in this block */

    chunk = block->first_chunk;
    block->biggest_free = 0;

    while ( chunk != NULL ) {
        ASSERT( chunk->magic == KMALLOC_CHUNK_MAGIC );

        if ( ( chunk->type == CHUNK_FREE ) &&
             ( chunk->size > block->biggest_free ) ) {
            block->biggest_free = chunk->size;
        }

        chunk = chunk->next;
    }

    return p;
}

void* kmalloc( uint32_t size ) {
    void* p;
    uint32_t min_size;
    kmalloc_block_t* block;

    if ( __unlikely( size == 0 ) ) {
        kprintf( WARNING, "kmalloc(): Called with 0 size!\n" );
        return NULL;
    }

    spinlock_disable( &kmalloc_lock );

    block = root;

    while ( block != NULL ) {
        if ( block->biggest_free >= size ) {
            goto block_found;
        }

        block = block->next;
    }

    /* create a new block */

    min_size = PAGE_ALIGN( size + sizeof( kmalloc_block_t ) + sizeof( kmalloc_chunk_t ) );

    if ( min_size < KMALLOC_BLOCK_SIZE ) {
        min_size = KMALLOC_BLOCK_SIZE;
    }

    block = __kmalloc_create_block( min_size / PAGE_SIZE );

    if ( __unlikely( block == NULL ) ) {
        spinunlock_enable( &kmalloc_lock );

        return NULL;
    }

    /* link the new block to the list */

    block->next = root;
    root = block;

    /* allocate the required memory from the new block */

block_found:
    p = __kmalloc_from_block( block, size );

    spinunlock_enable( &kmalloc_lock );

    if ( __likely( p != NULL ) ) {
        alloc_size += size;

        memset( p, 0xAA, size );
    }

    return p;
}

void kfree( void* p ) {
    kmalloc_chunk_t* chunk;

    if ( __unlikely( p == NULL ) ) {
        return;
    }

    chunk = ( kmalloc_chunk_t* )( ( uint8_t* )p - sizeof( kmalloc_chunk_t ) );

    spinlock_disable( &kmalloc_lock );

    if ( __unlikely( chunk->magic != KMALLOC_CHUNK_MAGIC ) ) {
        panic( "kfree(): Tried to free an invalid memory region! (%x)\n", p );
    }

    if ( __unlikely( chunk->type != CHUNK_ALLOCATED ) ) {
        panic( "kfree(): Tried to free a non-allocated memory region! (%x)\n", p );
    }

    alloc_size -= chunk->real_size;

    /* make the current chunk free */

    chunk->type = CHUNK_FREE;

    /* destroy the previous data in this memory chunk */

    memset( chunk + 1, 0xAA, chunk->size );

    /* merge with the previous chunk if it is free */

    if ( ( chunk->prev != NULL ) &&
         ( chunk->prev->type == CHUNK_FREE ) ) {
        kmalloc_chunk_t* prev_chunk = chunk->prev;

        ASSERT( chunk->prev->magic == KMALLOC_CHUNK_MAGIC );

        chunk->magic = 0;

        prev_chunk->size += chunk->size;
        prev_chunk->size += sizeof( kmalloc_chunk_t );

        prev_chunk->next = chunk->next;
        chunk = prev_chunk;

        if ( chunk->next != NULL ) {
            chunk->next->prev = chunk;
        }
    }

    /* merge with the next chunk if it is free */

    if ( ( chunk->next != NULL ) &&
         ( chunk->next->type == CHUNK_FREE ) ) {
        kmalloc_chunk_t* next_chunk = chunk->next;

        ASSERT( next_chunk->magic == KMALLOC_CHUNK_MAGIC );

        next_chunk->magic = 0;

        chunk->size += next_chunk->size;
        chunk->size += sizeof( kmalloc_chunk_t );

        chunk->next = next_chunk->next;

        if ( chunk->next != NULL ) {
            chunk->next->prev = chunk;
        }
    }

    /* update the biggest free chunk size in the current block */

    if ( chunk->size > chunk->block->biggest_free ) {
        chunk->block->biggest_free = chunk->size;
    }

    spinunlock_enable( &kmalloc_lock );
}

void kmalloc_get_statistics( uint32_t* _used_pages, uint32_t* _alloc_size ) {
    spinlock_disable( &kmalloc_lock );

    *_used_pages = used_pages;
    *_alloc_size = alloc_size;

    spinunlock_enable( &kmalloc_lock );
}

__init int init_kmalloc( void ) {
    root = __kmalloc_create_block( KMALLOC_ROOT_SIZE / PAGE_SIZE );

    if ( root == NULL ) {
        return -ENOMEM;
    }

    return 0;
}
