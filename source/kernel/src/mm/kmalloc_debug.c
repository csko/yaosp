/* Memory allocator debugger
 *
 * Copyright (c) 2010 Zoltan Kovacs
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

#include <mm/kmalloc.h>

#ifdef ENABLE_KMALLOC_DEBUG

#include <debug.h>
#include <console.h>

static kmalloc_debug_output_t* output = NULL;

static int kmalloc_debug_trace( ptr_t ip, ptr_t symbol_address, const char* symbol_name, void* data ) {
    kmalloc_debug_output_t* out;

    out = ( kmalloc_debug_output_t* )data;

    if ( symbol_name != NULL ) {
        out->malloc_trace( ip, symbol_name );
    } else {
        out->malloc_trace( 0, "<unknown>" );
    }

    return 0;
}

void kmalloc_debug( uint32_t size, void* p ) {
    if ( output != NULL ) {
        output->malloc( size, p );
        debug_print_stack_trace_cb( kmalloc_debug_trace, output );
    }
}

void kfree_debug( void* p ) {
    if ( output != NULL ) {
        output->free( p );
    }
}

void kmalloc_set_debug_output( kmalloc_debug_output_t* out ) {
    output = out;

    if ( output != NULL ) {
        output->init();
    }
}

#endif /* ENABLE_KMALLOC_DEBUG */

#ifdef ENABLE_KMALLOC_BARRIERS

#include <console.h>
#include <lib/string.h>

void* kmalloc_create_barriers(void* p, size_t size) {
    uint8_t* data = (uint8_t*)p;
    uint8_t* before = data;
    uint8_t* after = data + size - KMALLOC_BARRIER_SIZE;

    memset(before, 0xbb, KMALLOC_BARRIER_SIZE);
    memset(after, 0xcc, KMALLOC_BARRIER_SIZE);

    return (data + KMALLOC_BARRIER_SIZE);
}

void kmalloc_validate_barriers(void* p) {
    int i;
    uint8_t* data = (uint8_t*)p;
    data -= KMALLOC_BARRIER_SIZE;

    kmalloc_chunk_t* chunk = (kmalloc_chunk_t*)((uint8_t*)data - sizeof(kmalloc_chunk_t));
    uint8_t* before = data;
    uint8_t* after = data + chunk->size - KMALLOC_BARRIER_SIZE;

    uint8_t* failed_ptr;
    uint8_t expected_data;

    for (i = 0; i < KMALLOC_BARRIER_SIZE; i++) {
        if (before[i] != 0xbb) {
            failed_ptr = before + i;
            expected_data = 0xbb;
            goto failed;
        }

        if (after[i] != 0xcc) {
            failed_ptr = after + i;
            expected_data = 0xcc;
            goto failed;
        }
    }

    return;

failed:
    dprintf_unlocked("kmalloc barrier validation failed at %x\n", failed_ptr);
    dprintf_unlocked("    expected: %02x found: %02x\n", expected_data, *failed_ptr);
}

#endif /* ENABLE_KMALLOC_BARRIERS */
