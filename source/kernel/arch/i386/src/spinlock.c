/* Spinlock implementation
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

#include <macros.h>

#include <arch/spinlock.h>
#include <arch/interrupt.h>

#include <console.h>

int init_spinlock( spinlock_t* lock, const char* name ) {
    lock->name = name;
    lock->enable_interrupts = false;

#ifdef ENABLE_SMP
    atomic_set( &lock->locked, 0 );
#endif /* ENABLE_SMP */

    return 0;
}

void spinlock( spinlock_t* lock ) {
    ASSERT( is_interrupts_disabled() );

#ifdef ENABLE_SMP
    while ( atomic_swap( &lock->locked, 1 ) == 1 ) {
        __asm__ __volatile__( "pause" );
    }
#endif /* ENABLE_SMP */
}

void spinunlock( spinlock_t* lock ) {
    ASSERT( is_interrupts_disabled() );

#ifdef ENABLE_SMP
    atomic_set( &lock->locked, 0 );
#endif /* ENABLE_SMP */
}

void spinlock_disable( spinlock_t* lock ) {
    bool ints;

    ints = disable_interrupts();

#ifdef ENABLE_SMP
    spinlock( lock );
#endif /* ENABLE_SMP */

    lock->enable_interrupts = ints;
}

void spinunlock_enable( spinlock_t* lock ) {
    bool ints;

    ints = lock->enable_interrupts;

#ifdef ENABLE_SMP
    spinunlock( lock );
#endif /* ENABLE_SMP */

    if ( ints ) {
        enable_interrupts();
    }
}

bool spinlock_is_locked( spinlock_t* lock ) {
#ifdef ENABLE_SMP
    return ( atomic_get( &lock->locked ) == 1 );
#else
    return is_interrupts_disabled();
#endif /* ENABLE_SMP */
}
