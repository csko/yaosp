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

#include <timer.h>
#include <smp.h>
#include <sched/scheduler.h>
#include <lib/string.h>

int timer_init( timer_t* timer, timer_callback_t* callback, void* data ) {
    waitnode_t* node;

    timer->callback = callback;
    timer->data = data;

    node = &timer->node;

    memset( node, 0, sizeof( waitnode_t ) );

    node->type = WAIT_CALLBACK;
    node->u.callback = timer->callback;
    node->u.data = timer->data;

    return 0;
}

int timer_setup( timer_t* timer, uint64_t expire_time ) {
    waitnode_t* node;

    scheduler_lock();

    node = &timer->node;

    /* Remove the timer from the sleep list if it's already on it. */

    if ( node->in_queue ) {
        waitqueue_remove_node( &sleep_queue, node );
    }

    /* Update the timer node. */

    node->wakeup_time = expire_time;

    /* Put the timer to the sleep list. */

    waitqueue_add_node( &sleep_queue, node );

    scheduler_unlock();

    return 0;
}

int timer_cancel( timer_t* timer ) {
    waitnode_t* node;

    scheduler_lock();

    node = &timer->node;

    /* Remove the timer from the sleep list if it's already on it. */

    if ( node->in_queue ) {
        waitqueue_remove_node( &sleep_queue, node );
    }

    scheduler_unlock();

    return 0;
}
