/* Stack implementation
 *
 * Copyright (c) 2009 Zoltan Kovacs
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

#ifndef _YUTIL_STACK_H_
#define _YUTIL_STACK_H_

typedef struct stack {
    void** items;
    int item_count;
    int max_items;
} stack_t;

int stack_push( stack_t* stack, void* item );
int stack_pop( stack_t* stack, void** item );
int stack_peek( stack_t* stack, void** item );

int stack_size( stack_t* stack );

int init_stack( stack_t* stack );
int destroy_stack( stack_t* stack );

#endif /* _YUTIL_STACK_H_ */
