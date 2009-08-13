/* Wait queue implementation
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

#ifndef _WAITQUEUE_H_
#define _WAITQUEUE_H_

#include <types.h>
#include <thread.h>

typedef struct waitnode {
    thread_id thread;
    uint64_t wakeup_time;
    bool in_queue;

    struct waitnode* prev;
    struct waitnode* next;
} waitnode_t;

typedef struct waitqueue {
    waitnode_t* first_node;
    waitnode_t* last_node;
} waitqueue_t;

/**
 * Inserts a new node to the specified waitqueue. The new node
 * will be inserted to the list according to its wakeup time.
 *
 * @param queue The pointer to the waitqueue structure
 * @param node The pointer to the waitnode structure that will
 *             be inserted into the waitqueue
 * @return On success 0 is returned
 */
int waitqueue_add_node( waitqueue_t* queue, waitnode_t* node );

/**
 * Inserts a new node to the specified waitqueue. The new node
 * will be inserted to the end of the list.
 *
 * @param queue The pointer to the waitqueue structure
 * @param node The pointer to the waitnode structure that will
 *             be inserted into the waitqueue
 * @return On success 0 is returned
 */
int waitqueue_add_node_tail( waitqueue_t* queue, waitnode_t* node );

/**
 * Removes a node from the waitqueue.
 *
 * @param queue The pointer to the waitqueue structure
 * @param node The pointer to the node to remove
 * @return On success 0 is returned
 */
int waitqueue_remove_node( waitqueue_t* queue, waitnode_t* node );

/**
 * Wakes up nodes in the queue with wakeup time less or
 * equal to the specified time in now parameter.
 *
 * @param queue The pointer to the waitqueue structure
 * @param now The time to use as a reference for the wakeup
 * @return On success 0 is returned
 */
int waitqueue_wake_up( waitqueue_t* queue, uint64_t now );

/**
 * Wakes up the first count number of nodes from the queue.
 *
 * @param queue The pointer to the waitqueue structure
 * @param count The number of waiters to wake up
 * @return On success 0 is returned
 */
int waitqueue_wake_up_head( waitqueue_t* queue, int count );

/**
 * Wakes up all waiters in the waitqueue.
 *
 * @param queue The pointer to the waitqueue
 * @return On success 0 is returned
 */
int waitqueue_wake_up_all( waitqueue_t* queue );

bool waitqueue_is_empty( waitqueue_t* queue );

/**
 * Initializes a wait queue.
 *
 * @param queue The pointer to the waitqueue structure
 * @return On success 0 is returned
 */
int init_waitqueue( waitqueue_t* queue );

#endif /* _WAITQUEUE_H_ */
