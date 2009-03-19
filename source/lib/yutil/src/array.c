/* Array implementation
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

#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include <yutil/array.h>

int array_add_item( array_t* array, void* item ) {
    if ( ( array->item_count + 1 ) >= array->max_item_count ) {
        void** new_items;

        new_items = ( void** )malloc( sizeof( void* ) * ( array->max_item_count + array->realloc_size ) );

        if ( new_items == NULL ) {
            return -ENOMEM;
        }

        memcpy( new_items, array->items, sizeof( void* ) * array->max_item_count );

        if ( array->items != NULL ) {
            free( array->items );
        }

        array->items = new_items;
        array->max_item_count += array->realloc_size;
    }

    array->items[ array->item_count++ ] = item;

    return 0;
}

int array_get_size( array_t* array ) {
    return array->item_count;
}

void* array_get_item( array_t* array, int index ) {
    if ( ( index < 0 ) || ( index >= array->item_count ) ) {
        return NULL;
    }

    return array->items[ index ];
}

void array_set_realloc_size( array_t* array, int realloc_size ) {
    array->realloc_size = realloc_size;
}

int init_array( array_t* array ) {
    array->item_count = 0;
    array->max_item_count = 0;
    array->realloc_size = 0;
    array->items = NULL;

    return 0;
}
