/* Scheduler
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

#ifndef _SCHEDULER_H_
#define _SCHEDULER_H_

#include <types.h>
#include <thread.h>

#include <arch/spinlock.h>

extern spinlock_t scheduler_lock;

int add_thread_to_ready( thread_t* thread );

thread_t* do_schedule( void );
void schedule( registers_t* regs );

void sched_preempt( void );

int init_scheduler( void );

#endif // _SCHEDULER_H_
