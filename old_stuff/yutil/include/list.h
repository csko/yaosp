/* List implementation
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

#ifndef _YUTIL_LIST_H_
#define _YUTIL_LIST_H_

typedef struct list_item {
    void* data;
    struct list_item* next;
} list_item_t;

typedef struct list {
    int size;
    list_item_t* head;
    list_item_t* tail;

    int cur_free;
    int max_free;
    list_item_t* free;
} list_t;

int list_add_item( list_t* list, void* data );
int list_get_size( list_t* list );
void* list_get_head( list_t* list );
void* list_get_tail( list_t* list );
void* list_pop_head( list_t* list );

int list_set_max_free( list_t* list, int max_free );

int init_list( list_t* list );
int destroy_list( list_t* list );

#endif /* _YUTIL_LIST_H_ */
