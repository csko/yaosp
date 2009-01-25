/* Spinlock implementation
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

#include <arch/spinlock.h>
#include <arch/interrupt.h>

int init_spinlock( spinlock_t* lock, const char* name ) {
    lock->name = name;
    atomic_set( &lock->locked, 0 );
    lock->enable_interrupts = false;

    return 0;
}

void spinlock( spinlock_t* lock ) {
    while ( atomic_swap( &lock->locked, 1 ) == 1 ) {
        __asm__ __volatile__( "pause" );
    }
}

void spinunlock( spinlock_t* lock ) {
    atomic_set( &lock->locked, 0 );
}

void spinlock_disable( spinlock_t* lock ) {
    bool ints;

    ints = disable_interrupts();

    spinlock( lock );

    lock->enable_interrupts = ints;
}

void spinunlock_enable( spinlock_t* lock ) {
    bool ints;

    ints = lock->enable_interrupts;

    spinunlock( lock );

    if ( ints ) {
        enable_interrupts();
    }
}

bool spinlock_is_locked( spinlock_t* lock ) {
    return ( atomic_get( &lock->locked ) == 1 );
}
