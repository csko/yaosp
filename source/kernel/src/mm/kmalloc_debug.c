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
