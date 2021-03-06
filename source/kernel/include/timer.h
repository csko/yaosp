/* Timer handling
 *
 * Copyright (c) 2010 Zoltan Kovacs
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

#ifndef _TIMER_H_
#define _TIMER_H_

#include <types.h>
#include <sched/waitqueue.h>

typedef struct timer {
    waitnode_t node;
    timer_callback_t* callback;
    void* data;
} timer_t;

int timer_init( timer_t* timer, timer_callback_t* callback, void* data );
int timer_setup( timer_t* timer, uint64_t expire_time );
int timer_cancel( timer_t* timer );

#endif /* _TIMER_H_ */
