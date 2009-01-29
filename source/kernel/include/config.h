/* Configuration definitions
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

#ifndef _CONFIG_H_
#define _CONFIG_H_

//#define ENABLE_SMP 1

/**
 * The maximum number of CPUs supported.
 */
#ifdef ENABLE_SMP
#define MAX_CPU_COUNT 32
#else
#define MAX_CPU_COUNT 1
#endif

/**
 * The maximum number of boot modules supported.
 */
#define MAX_BOOTMODULE_COUNT 16

/**
 * The size of the kernel stack for threads.
 */
#define KERNEL_STACK_SIZE ( 32 * 1024 )

/**
 * The default size of the user stack for threads.
 */
#define USER_STACK_SIZE ( 32 * 1024 )

#endif // _CONFIG_H_
