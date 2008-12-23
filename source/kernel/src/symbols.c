/* Kernel symbol table
 *
 * Copyright (c) 2008 Zoltan Kovacs
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

#include <symbols.h>
#include <errno.h>
#include <console.h>
#include <mm/kmalloc.h>
#include <lib/string.h>

#include <arch/atomic.h>
#include <arch/spinlock.h>
#include <arch/interrupt.h>

static kernel_symbol_t symbols[] = {
    /* Console output */
    { "kprintf", ( ptr_t )kprintf },
    { "kvprintf", ( ptr_t )kvprintf },

    /* Memory management */
    { "kmalloc", ( ptr_t )kmalloc },
    { "kfree", ( ptr_t )kfree },

    /* Atomic operations */
    { "atomic_get", ( ptr_t )atomic_get },
    { "atomic_set", ( ptr_t )atomic_set },
    { "atomic_inc", ( ptr_t )atomic_inc },
    { "atomic_dec", ( ptr_t )atomic_dec },
    { "atomic_swap", ( ptr_t )atomic_swap },

    /* Spinlock operations */
    { "spinlock", ( ptr_t )spinlock },
    { "spinunlock", ( ptr_t )spinunlock },
    { "spinlock_disable", ( ptr_t )spinlock_disable },
    { "spinunlock_enable", ( ptr_t )spinunlock_enable },

    /* Interrupt manipulation */
    { "disable_interrupts", ( ptr_t )disable_interrupts },
    { "enable_interrupts", ( ptr_t )enable_interrupts },

    /* List terminator */
    { NULL, 0 }
};

int get_kernel_symbol_address( const char* name, ptr_t* address ) {
    uint32_t i;

    for ( i = 0; symbols[ i ].name != NULL; i++ ) {
        if ( strcmp( symbols[ i ].name, name ) == 0 ) {
            *address = symbols[ i ].address;

            return 0;
        }
    }

    return -EINVAL;
}
