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

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <yutil/stack.h>

static int stack_resize( stack_t* stack ) {
    void* new_items;

    new_items = malloc( sizeof( void* ) * ( stack->max_items + 32 ) );

    if ( new_items == NULL ) {
        return -ENOMEM;
    }

    memcpy(
        new_items,
        stack->items,
        sizeof( void* ) * stack->item_count
    );

    free( stack->items );

    stack->items = new_items;
    stack->max_items += 32;

    return 0;
}

int stack_push( stack_t* stack, void* item ) {
    if ( ( stack->item_count + 1 ) >= stack->max_items ) {
        int error;

        error = stack_resize( stack );

        if ( error < 0 ) {
            return error;
        }
    }

    stack->items[ stack->item_count++ ] = item;

    return 0;
}

int stack_pop( stack_t* stack, void** item ) {
    if ( stack->item_count == 0 ) {
        return -ENOENT;
    }

    *item = stack->items[ --stack->item_count ];

    return 0;
}

int stack_peek( stack_t* stack, void** item ) {
    if ( stack->item_count == 0 ) {
        return -ENOENT;
    }

    *item = stack->items[ stack->item_count - 1 ];

    return 0;
}

int stack_size( stack_t* stack ) {
    return stack->item_count;
}

int init_stack( stack_t* stack ) {
    stack->items = NULL;
    stack->item_count = 0;
    stack->max_items = 0;

    return 0;
}

int destroy_stack( stack_t* stack ) {
    free( stack->items );
    stack->items = NULL;

    return 0;
}
