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

#include <errno.h>
#include <stdlib.h>

#include <yutil/list.h>

static list_item_t* list_get_free_item( list_t* list ) {
    list_item_t* item;

    if ( list->cur_free > 0 ) {
        item = list->free;
        list->free = item->next;

        list->cur_free--;
    } else {
        item = ( list_item_t* )malloc( sizeof( list_item_t ) );
    }

    return item;
}

static void list_put_free_item( list_t* list, list_item_t* item ) {
    if ( list->cur_free < list->max_free ) {
        item->next = list->free;
        list->free = item;

        list->cur_free++;
    } else {
        free( item );
    }
}

int list_add_item( list_t* list, void* data ) {
    list_item_t* item;

    item = list_get_free_item( list );

    if ( item == NULL ) {
        return -ENOMEM;
    }

    item->data = data;
    item->next = NULL;

    if ( list->head == NULL ) {
        list->head = item;
        list->tail = item;
    } else {
        list->tail->next = item;
        list->tail = item;
    }

    list->size++;

    return 0;
}

int list_get_size( list_t* list ) {
    return list->size;
}

void* list_get_head( list_t* list ) {
    if ( list->size == 0 ) {
        return NULL;
    }

    return list->head->data;
}

void* list_get_tail( list_t* list ) {
    if ( list->size == 0 ) {
        return NULL;
    }

    return list->tail->data;
}

void* list_pop_head( list_t* list ) {
    void* data;
    list_item_t* item;

    if ( list->size == 0 ) {
        return NULL;
    }

    item = list->head;
    list->head = item->next;

    if ( list->head == NULL ) {
        list->tail = NULL;
    }

    list->size--;

    data = item->data;

    list_put_free_item( list, item );

    return data;
}

int list_set_max_free( list_t* list, int max_free ) {
    int diff;

    if ( ( list == NULL ) ||
         ( max_free < 0 ) ) {
        return -EINVAL;
    }

    list->max_free = max_free;
    diff = list->max_free - list->cur_free;

    if ( diff > 0 ) {
        int i;

        for ( i = 0; i < diff; i++ ) {
            list_item_t* item;

            item = ( list_item_t* )malloc( sizeof( list_item_t ) );

            if ( item == NULL ) {
                return -ENOMEM;
            }

            item->next = list->free;
            list->free = item;
        }
    }


    return 0;
}

int init_list( list_t* list ) {
    list->size = 0;
    list->head = NULL;
    list->tail = NULL;

    list->max_free = 0;
    list->cur_free = 0;
    list->free = NULL;

    return 0;
}

int destroy_list( list_t* list ) {
    while ( list->head != NULL ) {
        list_item_t* item;

        item = list->head;
        list->head = item->next;

        free( item );
    }

    while ( list->free != NULL ) {
        list_item_t* item;

        item = list->free;
        list->free = item->next;

        free( item );
    }

    return 0;
}
