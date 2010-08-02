/* Memory allocator
 *
 * Copyright (c) 2008, 2009, 2010 Zoltan Kovacs
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

static spinlock_t kmalloc_lock = INIT_SPINLOCK("kmalloc");
static kmalloc_block_t* root = NULL;

static inline int kmalloc_chunk_validate(kmalloc_chunk_t* chunk) {
    return ((chunk->magic & 0xFFFFFFF0) == KMALLOC_CHUNK_MAGIC);
}

static inline void kmalloc_chunk_set_free(kmalloc_chunk_t* chunk, int free) {
    if (free) {
        chunk->magic |= CHUNK_FREE;
    } else {
        chunk->magic &= ~CHUNK_FREE;
    }
}

static inline int kmalloc_chunk_is_free(kmalloc_chunk_t* chunk) {
    return ((chunk->magic & CHUNK_FREE) != 0);
}

static kmalloc_block_t* kmalloc_block_create(uint32_t pages) {
    kmalloc_block_t* block;
    kmalloc_chunk_t* chunk;

    block = (kmalloc_block_t*)alloc_pages(pages, MEM_COMMON);

    if (__unlikely(block == NULL)) {
        return NULL;
    }

    used_pages += pages;

    block->magic = KMALLOC_BLOCK_MAGIC;
    block->pages = pages;
    block->next = NULL;

    chunk = (kmalloc_chunk_t*)(block + 1);

    chunk->magic = KMALLOC_CHUNK_MAGIC;
    kmalloc_chunk_set_free(chunk, 1);
    chunk->block = block;
    chunk->size = (pages * PAGE_SIZE) - sizeof(kmalloc_block_t) - sizeof(kmalloc_chunk_t);
    chunk->prev = NULL;
    chunk->next = NULL;

    block->biggest_free = chunk->size;

    return block;
}

static void* __kmalloc_from_block( kmalloc_block_t* block, uint32_t size ) {
    void* p;
    kmalloc_chunk_t* chunk;

    chunk = (kmalloc_chunk_t*)(block + 1);

    while ((chunk != NULL) &&
           ((!kmalloc_chunk_is_free(chunk)) ||
            (chunk->size < size))) {
        ASSERT(kmalloc_chunk_validate(chunk));
        chunk = chunk->next;
    }

    if ( __unlikely( chunk == NULL ) ) {
        return NULL;
    }

    kmalloc_chunk_set_free(chunk, 0);
    p = ( void* )(chunk + 1);

    if ( chunk->size > ( size + sizeof( kmalloc_chunk_t ) + 4 ) ) {
        uint32_t remaining_size;
        kmalloc_chunk_t* new_chunk;

        remaining_size = chunk->size - size - sizeof( kmalloc_chunk_t );

        chunk->size = size;

        new_chunk = ( kmalloc_chunk_t* )( ( uint8_t* )chunk + sizeof( kmalloc_chunk_t ) + size );

        new_chunk->magic = KMALLOC_CHUNK_MAGIC;
        kmalloc_chunk_set_free(new_chunk, 1);
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

    /* Update the number of allocated bytes. */
    alloc_size += chunk->size;

    /* Recalculate the biggest free chunk in this block. */
    block->biggest_free = 0;

    chunk = (kmalloc_chunk_t*)(block + 1);

    while (chunk != NULL) {
        ASSERT(kmalloc_chunk_validate(chunk));

        if ((kmalloc_chunk_is_free(chunk)) &&
            (chunk->size > block->biggest_free)) {
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

    /* Is this an invalid request? */
    if (__unlikely(size == 0)) {
        kprintf( WARNING, "kmalloc(): Called with 0 size!\n" );
        return NULL;
    }

    /* Ensure the minimum allocation. */
    if (size < sizeof(ptr_t)) {
        size = sizeof(ptr_t);
    }

    spinlock_disable(&kmalloc_lock);

    block = root;

    while (block != NULL) {
        if (block->biggest_free >= size) {
            goto block_found;
        }

        block = block->next;
    }

    /* create a new block */

    min_size = PAGE_ALIGN(size + sizeof(kmalloc_block_t) + sizeof(kmalloc_chunk_t));

    if (min_size < KMALLOC_BLOCK_SIZE) {
        min_size = KMALLOC_BLOCK_SIZE;
    }

    block = kmalloc_block_create(min_size / PAGE_SIZE);

    if (__unlikely(block == NULL)) {
        spinunlock_enable(&kmalloc_lock);
        return NULL;
    }

    /* link the new block to the list */

    block->next = root;
    root = block;

    /* allocate the required memory from the new block */

block_found:
    p = __kmalloc_from_block(block, size);

#ifdef ENABLE_KMALLOC_DEBUG
    kmalloc_debug(size, p);
#endif

    spinunlock_enable(&kmalloc_lock);

    if (__likely(p != NULL)) {
        memset(p, 0xAA, size);
    }

    return p;
}

void* kcalloc( uint32_t nmemb, uint32_t size ) {
    void* p;
    uint32_t s;

    s = nmemb * size;
    p = kmalloc( s );

    if ( p == NULL ) {
        return NULL;
    }

    memset( p, 0, s );

    return p;
}

void kfree( void* p ) {
    kmalloc_chunk_t* chunk;

    if ( __unlikely( p == NULL ) ) {
        return;
    }

    chunk = ( kmalloc_chunk_t* )( ( uint8_t* )p - sizeof( kmalloc_chunk_t ) );

    spinlock_disable( &kmalloc_lock );

    if (__unlikely(!kmalloc_chunk_validate(chunk))) {
        panic( "kfree(): Tried to free an invalid memory region! (%x)\n", p );
    }

    if (__unlikely(kmalloc_chunk_is_free(chunk))) {
        panic( "kfree(): Tried to free a non-allocated memory region! (%x)\n", p );
    }

#ifdef ENABLE_KMALLOC_DEBUG
    kfree_debug( p );
#endif

    alloc_size -= chunk->size;

    /* Make the current chunk free. */
    kmalloc_chunk_set_free(chunk, 1);

    /* Destroy the previous data in this memory chunk. */
    memset(chunk + 1, 0xAA, chunk->size);

    /* Merge with the previous chunk if it is free */
    if ((chunk->prev != NULL) &&
        (kmalloc_chunk_is_free(chunk->prev))) {
        kmalloc_chunk_t* prev_chunk = chunk->prev;

        ASSERT(kmalloc_chunk_validate(chunk->prev));

        chunk->magic = 0;

        prev_chunk->size += chunk->size;
        prev_chunk->size += sizeof(kmalloc_chunk_t);

        prev_chunk->next = chunk->next;
        chunk = prev_chunk;

        if ( chunk->next != NULL ) {
            chunk->next->prev = chunk;
        }
    }

    /* merge with the next chunk if it is free */

    if ((chunk->next != NULL) &&
        (kmalloc_chunk_is_free(chunk->next))) {
        kmalloc_chunk_t* next_chunk = chunk->next;

        ASSERT(kmalloc_chunk_validate(next_chunk));

        next_chunk->magic = 0;

        chunk->size += next_chunk->size;
        chunk->size += sizeof( kmalloc_chunk_t );

        chunk->next = next_chunk->next;

        if ( chunk->next != NULL ) {
            chunk->next->prev = chunk;
        }
    }

    /* update the biggest free chunk size in the current block */

    if (chunk->size > chunk->block->biggest_free) {
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
    root = kmalloc_block_create(KMALLOC_ROOT_SIZE / PAGE_SIZE);

    if ( root == NULL ) {
        return -ENOMEM;
    }

    return 0;
}
