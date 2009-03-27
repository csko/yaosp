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

void* sys_sbrk( int increment ) {
    void* pbreak;
    process_t* process;

    pbreak = ( void* )-1;
    process = current_process();

    LOCK( process->lock );

    if ( increment == 0 ) {
        /* Return the current location of the process break */

        if ( process->heap_region < 0 ) {
            memory_context_find_unmapped_region(
                process->memory_context,
                FIRST_USER_ADDRESS,
                LAST_USER_ADDRESS,
                PAGE_SIZE,
                ( ptr_t* )&pbreak
            );
        } else {
            region_info_t region_info;

            if ( get_region_info( process->heap_region, &region_info ) == 0 ) {
                pbreak = ( void* )( region_info.start + region_info.size );
            }
        }
    } else if ( increment < 0 ) {
        /* Decrement the size of the heap */

        increment = -PAGE_ALIGN(-increment);

        if ( process->heap_region >= 0 ) {
            region_info_t region_info;

            if ( get_region_info( process->heap_region, &region_info ) == 0 ) {
                if ( -increment == region_info.size ) {
                    if ( delete_region( process->heap_region ) == 0 ) {
                        pbreak = ( void* )( region_info.start + region_info.size );
                        process->heap_region = -1;
                    }
                } else if ( -increment < region_info.size ) {
                    if ( resize_region( process->heap_region, region_info.size + increment ) == 0 ) {
                        pbreak = ( void* )( region_info.start + region_info.size );
                    }
                }
            }
        }
    } else {
        /* Increment the size of the heap */

        increment = PAGE_ALIGN( increment );

        if ( process->heap_region < 0 ) {
            process->heap_region = create_region(
                "heap",
                increment,
                REGION_READ | REGION_WRITE,
                ALLOC_PAGES,
                &pbreak
            );
        } else {
            region_info_t region_info;

            if ( get_region_info( process->heap_region, &region_info ) == 0 ) {
                if ( resize_region( process->heap_region, region_info.size + increment ) == 0 ) {
                    pbreak = ( void* )( region_info.start + region_info.size );
                }
            }
        }
    }

    UNLOCK( process->lock );

    return pbreak;
}
