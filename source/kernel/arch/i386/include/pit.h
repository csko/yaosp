/* Programmable interval timer
 *
 * Copyright (c) 2008, 2009 Zoltan Kovacs
 * Copyright (c) 2009 Kornel Csernai
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

#ifndef _ARCH_PIT_H_
#define _ARCH_PIT_H_

#define PIT_MODE 0x43
#define PIT_CH0  0x40

#define PIT_TICKS_PER_SEC 1193182

int pit_read_timer( void );
void pit_wait_wrap( void );

/**
 * Returns the time in microseconds not counting the leap ones since the Epoch
 *
 * @return The system time in microseconds
 */
uint64_t get_system_time( void );
uint64_t get_boot_time( void );

int init_system_time( void );

int init_pit( void );

#endif // _ARCH_PIT_H_
