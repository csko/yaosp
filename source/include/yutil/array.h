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

#ifndef _YUTIL_ARRAY_H_
#define _YUTIL_ARRAY_H_

typedef struct array {
    int item_count;
    int max_item_count;
    int realloc_size;
    int realloc_mask;
    void** items;
} array_t;

typedef int array_item_comparator_t( const void* item1, const void* item2 );

int array_add_item( array_t* array, void* item );
int array_add_items( array_t* array, array_t* other );
int array_insert_item( array_t* array, int index, void* item );
int array_remove_item( array_t* array, void* item );
int array_remove_item_from( array_t* array, int index );
int array_get_size( array_t* array );
void* array_get_item( array_t* array, int index );
int array_index_of( array_t* array, void* item );
int array_make_empty( array_t* array );
int array_sort( array_t* array, array_item_comparator_t* comparator );

int array_set_realloc_size( array_t* array, int realloc_size );

int init_array( array_t* array );
int destroy_array( array_t* array );

#endif /* _YUTIL_ARRAY_H_ */
