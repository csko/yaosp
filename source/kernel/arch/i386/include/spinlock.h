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

#ifndef _ARCH_SPINLOCK_H_
#define _ARCH_SPINLOCK_H_

#include <types.h>

#include <arch/atomic.h>

typedef struct spinlock {
    atomic_t locked;
    bool enable_interrupts;
} spinlock_t;

#define INIT_SPINLOCK { ATOMIC_INIT(0), false }

int init_spinlock( spinlock_t* lock );

/**
 * Locks a spinlock.
 *
 * @param lock The spinlock structure to lock on
 */
void spinlock( spinlock_t* lock );
/**
 * Unlocks a spinlock.
 *
 * @param lock The spinlock structure to unlock
 */
void spinunlock( spinlock_t* lock );

/**
 * Disables the interrupts on the current processor and
 * locks the specified spinlock.
 *
 * @param lock The spinlock structure to lock on
 */
void spinlock_disable( spinlock_t* lock );
/**
 * Unlocks the specified spinlock and then enables the
 * interrupts on the current processor if it was disabled
 * previously by the spinlock_disable() call.
 *
 * @param lock The spinlock to unlock
 */
void spinunlock_enable( spinlock_t* lock );

#endif // _ARCH_SPINLOCK_H_
