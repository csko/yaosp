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

static int check_array_buffer( array_t* array, int new_item_count ) {
    void** new_items;
    int new_size;

    if ( ( array->item_count + new_item_count ) <= array->max_item_count ) {
        return 0;
    }

    new_size = ( array->item_count + new_item_count + array->realloc_size - 1 ) & ~array->realloc_mask;

    new_items = ( void** )malloc( sizeof( void* ) * new_size );

    if ( new_items == NULL ) {
        return -ENOMEM;
    }

    memcpy( new_items, array->items, sizeof( void* ) * array->max_item_count );

    if ( array->items != NULL ) {
        free( array->items );
    }

    array->items = new_items;
    array->max_item_count = new_size;

    return 0;
}

int array_add_item( array_t* array, void* item ) {
    int error;

    error = check_array_buffer( array, 1 );

    if ( error < 0 ) {
        return error;
    }

    array->items[ array->item_count++ ] = item;

    return 0;
}

int array_add_items( array_t* array, array_t* other ) {
    int i;
    int error;

    error = check_array_buffer( array, other->item_count );

    if ( error < 0 ) {
        return error;
    }

    for ( i = 0; i < other->item_count; i++ ) {
        array->items[ array->item_count++ ] = other->items[ i ];
    }

    return 0;
}

int array_insert_item( array_t* array, int index, void* item ) {
    int error;
    int count;

    if ( ( index < 0 ) || ( index > array->item_count ) ) {
        return -EINVAL;
    }

    error = check_array_buffer( array, 1 );

    if ( error < 0 ) {
        return error;
    }

    count = array->item_count - index;

    if ( count > 0 ) {
        memmove( &array->items[ index + 1 ], &array->items[ index ], count * sizeof( void* ) );
    }

    array->items[ index ] = item;

    array->item_count++;

    return 0;
}

int array_remove_item( array_t* array, void* item ) {
    int i;
    int size;

    size = array->item_count;

    /* Find the first hit */

    for ( i = 0; i < size; i++ ) {
        if ( array->items[ i ] == item ) {
            break;
        }
    }

    if ( i == size ) { /* Item not found */
        return -ENOENT;
    }

    array->item_count--;

    /* Shift elements */

    memmove(
        &array->items[ i ],
        &array->items[ i + 1 ],
        sizeof( void* ) * ( size - ( i + 1 ) )
    );

    return 0;
}

int array_remove_item_from( array_t* array, int index ) {
    if ( ( index < 0 ) ||
         ( index >= array->item_count ) ) {
        return -EINVAL;
    }

    memmove(
        &array->items[ index ],
        &array->items[ index + 1 ],
        sizeof( void* ) * ( array->item_count - ( index + 1 ) )
    );

    array->item_count--;

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

int array_index_of( array_t* array, void* item ) {
    int i;

    for ( i = 0; i < array->item_count; i++ ) {
        if ( array->items[ i ] == item ) {
            return i;
        }
    }

    return -1;
}

int array_make_empty( array_t* array ) {
    int i;

    array->item_count = 0;

    for ( i = 0; i < array->max_item_count; i++ ) {
        array->items[ i ] = NULL;
    }

    return 0;
}

int array_sort( array_t* array, array_item_comparator_t* comparator ) {
    if ( array->item_count < 2 ) {
        return 0;
    }

    qsort( array->items, array->item_count, sizeof( void* ), comparator );

    return 0;
}

int array_set_realloc_size( array_t* array, int realloc_size ) {
    if ( realloc_size <= 0 ) {
        return -EINVAL;
    }

    array->realloc_size = realloc_size;
    array->realloc_mask = realloc_size - 1;

    return 0;
}

int init_array( array_t* array ) {
    array->item_count = 0;
    array->max_item_count = 0;
    array->realloc_size = 8;
    array->realloc_mask = array->realloc_size - 1;
    array->items = NULL;

    return 0;
}

int destroy_array( array_t* array ) {
    if ( array->items != NULL ) {
        free( array->items );
        array->items = NULL;
    }

    return 0;
}
