/* Wait queue implementation
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

#include <waitqueue.h>
#include <macros.h>
#include <scheduler.h>

int waitqueue_add_node( waitqueue_t* queue, waitnode_t* node ) {
    waitnode_t* current;

    if ( ( queue->nodes == NULL ) || ( node->wakeup_time < queue->nodes->wakeup_time ) ) {
        queue->nodes = node;
        node->next = NULL;

        return 0;
    }

    current = queue->nodes;

    while ( current->next != NULL ) {
        if ( node->wakeup_time < current->wakeup_time ) {
            current->prev->next = node;
            node->prev = current->prev;
            node->next = current;
            current->prev = node;

            return 0;
        }

        node = node->next;
    }

    node->prev = current;
    node->next = NULL;

    current->next = node;

    return 0;
}

int waitqueue_wake_up( waitqueue_t* queue, uint64_t now ) {
    waitnode_t* node;

    node = queue->nodes;

    while ( ( node != NULL ) && ( node->wakeup_time <= now ) ) {
        thread_t* thread;

        thread = get_thread_by_id( node->thread );

        if ( thread != NULL ) {
            /* If the thread doesn't have time to run add
               it to the list of expired threads, otherwise
               add it to the ready list */

            if ( thread->quantum == 0 ) {
                add_thread_to_expired( thread );
            } else {
                add_thread_to_ready( thread );
            }
        }

        node = node->next;
    }

    queue->nodes = node;

    if ( queue->nodes != NULL ) {
        queue->nodes->prev = NULL;
    }

    return 0;
}

int init_waitqueue( waitqueue_t* queue ) {
    queue->nodes = NULL;

    return 0;
}
