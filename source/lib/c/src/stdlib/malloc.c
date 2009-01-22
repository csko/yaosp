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

#include <stdlib.h>
#include <sys/types.h>
#include <errno.h>
#include <assert.h>
#define ASSERT assert
#include <sys/param.h>
#include <yaosp/debug.h>
#include <yaosp/region.h>
#include <yaosp/semaphore.h>

#define PAGE_SHIFT 12
#define PAGE_SIZE  ( 1UL << PAGE_SHIFT )
#define PAGE_MASK  ( ~( PAGE_SIZE - 1 ) )
#define PAGE_ALIGN( addr )  ( ( (addr) + PAGE_SIZE - 1 ) & PAGE_MASK )

#define KMALLOC_BLOCK_MAGIC 0xCAFEBABE
#define KMALLOC_CHUNK_MAGIC 0xDEADBEEF

#define KMALLOC_ROOT_SIZE ( 32 * 1024 )
#define KMALLOC_BLOCK_SIZE ( 128 * 1024 )

enum kmalloc_chunk_type {
  CHUNK_FREE = 1,
  CHUNK_ALLOCATED
};

struct kmalloc_chunk;

typedef struct kmalloc_block {
    uint32_t magic;
    uint32_t pages;
    uint32_t biggest_free;
    struct kmalloc_block* next;
    struct kmalloc_chunk* first_chunk;
} kmalloc_block_t;

typedef struct kmalloc_chunk {
    uint32_t magic;
    uint32_t type;
    uint32_t size;
    struct kmalloc_block* block;
    struct kmalloc_chunk* prev;
    struct kmalloc_chunk* next;
} kmalloc_chunk_t;

static semaphore_id lock = -1;
static kmalloc_block_t* root = NULL;

static kmalloc_block_t* __kmalloc_create_block( uint32_t pages ) {
    region_id region;
    kmalloc_block_t* block;
    kmalloc_chunk_t* chunk;

    region = create_region( "malloc region", pages * 4096, REGION_READ | REGION_WRITE, ALLOC_PAGES, ( void** )&block );

    if ( region < 0 ) {
        return NULL;
    }

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

    if ( chunk == NULL ) {
        return NULL;
    }

    chunk->type = CHUNK_ALLOCATED;
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

void validate_malloc( char* file, int line ) {
    kmalloc_block_t* block;

    dbprintf( "%s() Enter %s:%d\n", __FUNCTION__, file, line );

    lock_semaphore( lock, 1, INFINITE_TIMEOUT );

    block = root;

    while ( block != NULL ) {
        dbprintf( "%s() block=%x\n", __FUNCTION__, block );

        assert( block->magic == KMALLOC_BLOCK_MAGIC );
        assert( block->biggest_free >= 0 );

        block = block->next;
    }

    unlock_semaphore( lock, 1 );

    dbprintf( "%s() Exit %s:%d\n", __FUNCTION__, file, line );
}

void* malloc( uint32_t size ) {
    void* p;
    uint32_t min_size;
    kmalloc_block_t* block;

    dbprintf( "malloc() size=%u\n", size );
    assert( size > 0 );

    lock_semaphore( lock, 1, INFINITE_TIMEOUT );

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

    if ( block == NULL ) {
        unlock_semaphore( lock, 1 );

        return NULL;
    }

    /* link the new block to the list */

    block->next = root;
    root = block;

    /* allocate the required memory from the new block */

block_found:
    p = __kmalloc_from_block( block, size );

    unlock_semaphore( lock, 1 );

    return p;
}

void free( void* p ) {
    kmalloc_chunk_t* chunk;

    if ( p == NULL ) {
        return;
    }

    chunk = ( kmalloc_chunk_t* )( ( uint8_t* )p - sizeof( kmalloc_chunk_t ) );

    lock_semaphore( lock, 1, INFINITE_TIMEOUT );

    if ( chunk->magic != KMALLOC_CHUNK_MAGIC ) {
        dbprintf( "kfree(): Tried to free an invalid memory region! (%x)\n", p );
        _exit( -1 );
    }

    if ( chunk->type != CHUNK_ALLOCATED ) {
        dbprintf( "kfree(): Tried to free a non-allocated memory region! (%x)\n", p );
        _exit( -1 );
    }

    /* make the current chunk free */

    chunk->type = CHUNK_FREE;

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

    unlock_semaphore( lock, 1 );
}

void* realloc( void* ptr, size_t size ) {
    size_t old_size;
    void* new_block;
    kmalloc_chunk_t* chunk;

    if ( ptr != NULL ) {
        chunk = ( kmalloc_chunk_t* )( ( uint8_t* )ptr - sizeof( kmalloc_chunk_t ) );

        lock_semaphore( lock, 1, INFINITE_TIMEOUT );

        if ( chunk->magic != KMALLOC_CHUNK_MAGIC ) {
            dbprintf( "kfree(): Tried to free an invalid memory region! (%x)\n", ptr );
            _exit( -1 );
        }

        if ( chunk->type != CHUNK_ALLOCATED ) {
            dbprintf( "kfree(): Tried to free a non-allocated memory region! (%x)\n", ptr );
            _exit( -1 );
        }

        old_size = chunk->size;

        unlock_semaphore( lock, 1 );
    }

    new_block = malloc( size );

    if ( new_block == NULL ) {
        return NULL;
    }

    if ( ptr != NULL ) {
        size_t to_copy = MIN( size, old_size );

        memcpy( new_block, ptr, to_copy );

        free( ptr );
    }

    return new_block;
}

int init_malloc( void ) {
    lock = create_semaphore( "malloc lock", SEMAPHORE_BINARY, 0, 1 );

    if ( lock < 0 ) {
        return lock;
    }

    root = __kmalloc_create_block( KMALLOC_ROOT_SIZE / PAGE_SIZE );

    if ( root == NULL ) {
        return -ENOMEM;
    }

    return 0;
}
