/* Atomic operations
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

#ifndef _ARCH_ATOMIC_H_
#define _ARCH_ATOMIC_H_

/**
 * This macro is used to initialize an atomic structure at
 * definition time.
 */
#define ATOMIC_INIT(n) { n }

typedef struct atomic {
    volatile int value;
} atomic_t;

/**
 * Atomically gets the value stored in the atomic structure.
 *
 * @param atomic The pointer to the atomic structure
 * @return The value stored in the atomic structure
 */
int atomic_get( atomic_t* atomic );

/**
 * Atomically sets the value of the atomic structure.
 *
 * @param atomic The pointer to the atomic structure
 * @param value The number to set the atomic structure to
 * @return The same as the value parameter
 */
int atomic_set( atomic_t* atomic, int value );

/**
 * Atomically increments the value of the atomic structure.
 *
 * @param atomic The pointer to the atomic structure
 */
void atomic_inc( atomic_t* atomic );

/**
 * Atomically decrements the value of the atomic structure.
 *
 * @param atomic The pointer to the atomic structure
 */
void atomic_dec( atomic_t* atomic );
bool atomic_dec_and_test( atomic_t* atomic );

/**
 * Atomically swaps the value stored in the atomic structure
 * with the value specified as the second parameter.
 *
 * @param atomic The pointer to the atomic structure
 * @param value The value to swap the atomic structure with
 * @return The old value stored in the structure before the swap operation
 */
int atomic_swap( atomic_t* atomic, int value );

/**
 * Atomically tests and clears the nth bit in the dword pointer
 * by the specified address.
 *
 * @param address The address of the dword to test and clear on
 * @param bit The number of the bit to test and clear
 * @return 0 is returned if the specified bit was zero, otherwise
 *         a non-zero value is returned
 */
int atomic_test_and_clear( volatile void* address, int bit );

#endif /* _ARCH_ATOMIC_H_ */
