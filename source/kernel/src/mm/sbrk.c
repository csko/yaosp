/* Sbrk implementation
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

#include <process.h>
#include <smp.h>
#include <console.h>

void* sys_sbrk( int increment ) {
    void* pbreak;
    process_t* process;

    pbreak = ( void* )-1;
    process = current_process();

    mutex_lock( process->mutex, LOCK_IGNORE_SIGNAL );

    if ( increment == 0 ) {
        /* Return the current location of the process break */

        if ( process->heap_region == NULL ) {
            memory_context_find_unmapped_region(
                process->memory_context,
                FIRST_USER_ADDRESS,
                LAST_USER_ADDRESS,
                PAGE_SIZE,
                ( ptr_t* )&pbreak
            );
        } else {
            pbreak = ( void* )process->heap_region->address;
        }
    } else if ( increment < 0 ) {
        /* Decrement the size of the heap */

        increment = -PAGE_ALIGN(-increment);

        if ( process->heap_region != NULL ) {
            pbreak = ( void* )( process->heap_region->address + ( uint32_t )process->heap_region->size );

            if ( -increment == process->heap_region->size ) {
                memory_region_put( process->heap_region );
                process->heap_region = NULL;
            } else if ( -increment < process->heap_region->size ) {
                if ( memory_region_resize( process->heap_region, process->heap_region->size + increment ) != 0 ) {
                    pbreak = ( void* )-1;
                }
            }
        }
    } else {
        /* Increment the size of the heap */

        increment = PAGE_ALIGN( increment );

        if ( process->heap_region == NULL ) {
            process->heap_region = memory_region_create(
                "heap", increment,
                REGION_READ | REGION_WRITE
            );

            if ( process->heap_region != NULL ) {
                if ( memory_region_alloc_pages( process->heap_region ) == 0 ) {
                    pbreak = ( void* )process->heap_region->address;
                }
            }
        } else {
            pbreak = ( void* )( process->heap_region->address + ( uint32_t )process->heap_region->size );

            if ( memory_region_resize( process->heap_region, process->heap_region->size + increment ) != 0 ) {
                pbreak = ( void* )-1;
            }
        }
    }

    mutex_unlock( process->mutex );

    return pbreak;
}
