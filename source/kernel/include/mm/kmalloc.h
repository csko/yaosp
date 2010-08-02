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

#ifndef _MM_KMALLOC_H_
#define _MM_KMALLOC_H_

#include <types.h>
#include <config.h>

/**
 * Minimum block size that should be allocated when a new
 * block is created by kmalloc. This value has to be page
 * aligned.
 */
#define KMALLOC_BLOCK_SIZE      131072

/**
 * The size of the root block that is allocated during the
 * kernel initialization. This value has to be page aligned.
 */
#define KMALLOC_ROOT_SIZE       524288

#define KMALLOC_BLOCK_MAGIC 0xCAFEBAB0
#define KMALLOC_CHUNK_MAGIC 0xDEADBEE0

enum {
    CHUNK_FREE = 1
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
    uint32_t size;
    struct kmalloc_block* block;
    struct kmalloc_chunk* prev;
    struct kmalloc_chunk* next;
} kmalloc_chunk_t;

#ifdef ENABLE_KMALLOC_DEBUG

typedef struct {
    int ( *init )( void );
    int ( *malloc )( uint32_t size, void* p );
    int ( *malloc_trace )( ptr_t ip, const char* name );
    int ( *free )( void* p );
} kmalloc_debug_output_t;

void kmalloc_debug( uint32_t size, void* p );
void kfree_debug( void* p );
void kmalloc_set_debug_output( kmalloc_debug_output_t* out );

#endif /* ENABLE_KMALLOC_DEBUG */

/**
 * Allocates a size byte(s) long memory chunk.
 *
 * @param size The size of the memory chunk to allocate
 * @return On success a non-NULL pointer is returned pointing to
 *         the start of the allocated memory chunk
 */
void* kmalloc( uint32_t size ) __attribute__(( malloc ));
void* kcalloc( uint32_t nmemb, uint32_t size );

/**
 * Frees a previously allocated memory region.
 *
 * @param p The pointer to the start of the memory chunk to free
 */
void kfree( void* p );

void kmalloc_get_statistics( uint32_t* used_pages, uint32_t* alloc_size );

/**
 * Initializes the kernel memory allocator. This function is called
 * during the kernel initialization.
 *
 * @return On success 0 is returned
 */
int init_kmalloc( void );

#endif /* _MM_KMALLOC_H_ */
