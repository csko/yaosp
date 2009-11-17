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
#include <config.h>

#ifdef ENABLE_SMP
#include <arch/atomic.h>
#endif /* ENABLE_SMP */

typedef struct spinlock {
    const char* name;
#ifdef ENABLE_SMP
    atomic_t locked;
#endif /* ENABLE_SMP */
    bool enable_interrupts;
} spinlock_t;

#ifdef ENABLE_SMP
#define INIT_SPINLOCK(name) { name, ATOMIC_INIT(0), false }
#else
#define INIT_SPINLOCK(name) { name, false }
#endif /* ENABLE_SMP */

int init_spinlock( spinlock_t* lock, const char* name );

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

/**
 * Checks is the specified spinlock is locked or not.
 *
 * @param lock The spinlock to check
 * @return True is returned if the specified spinlock is locked
 */
bool spinlock_is_locked( spinlock_t* lock );

#endif /* _ARCH_SPINLOCK_H_ */
