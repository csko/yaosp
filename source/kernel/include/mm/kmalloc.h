#ifndef _MM_KMALLOC_H_
#define _MM_KMALLOC_H_

#include <types.h>

#define KMALLOC_BLOCK_SIZE          32
#define KMALLOC_ROOT_SIZE          128

#define KMALLOC_BLOCK_MAGIC 0xCAFEBABE
#define KMALLOC_CHUNK_MAGIC 0xDEADBEEF

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

void* kmalloc( uint32_t size );
void kfree( void* p );

int init_kmalloc( void );

#endif // _MM_KMALLOC_H_
