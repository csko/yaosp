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

//#define MK_RELEASE_BUILD 1

//#define ENABLE_SMP 1
//#define ENABLE_DEBUGGER 1

/**
 * The maximum number of CPUs supported.
 */
#ifdef ENABLE_SMP
#define MAX_CPU_COUNT 4
#else
#define MAX_CPU_COUNT 1
#endif

/**
 * The maximum number of boot modules supported.
 */
#define MAX_BOOTMODULE_COUNT 16

/**
 * The maximum length of module names.
 */
#define MAX_MODULE_NAME_LENGTH 64

/**
 * The maximum length of process names.
 */
#define MAX_PROCESS_NAME_LENGTH 64

/**
 * The maximum length of thread names.
 */
#define MAX_THREAD_NAME_LENGTH 64

/**
 * The maximum length of processor name.
 */
#define MAX_PROCESSOR_NAME_LENGTH 64

/**
 * The maximum number of available memory type descriptors.
 */
#define MAX_MEMORY_TYPES 4

/**
 * The size of the kernel stack for threads.
 */
#define KERNEL_STACK_SIZE ( 32 * 1024 )

/**
 * The default size of the user stack for threads.
 */
#define USER_STACK_SIZE ( 128 * 1024 )

#define KERNEL_PARAM_BUF_SIZE 4096
#define MAX_KERNEL_PARAMS 64

#endif /* _CONFIG_H_ */
