/* Hashtable implementation
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

#include <errno.h>
#include <mm/kmalloc.h>
#include <lib/string.h>
#include <lib/hashtable.h>

int init_hashtable(
    hashtable_t* table,
    uint32_t size,
    key_function_t* key_func,
    hash_function_t* hash_func,
    compare_function_t* compare_func
) {
    table->items = ( hashitem_t** )kmalloc( sizeof( hashitem_t* ) * size );

    if ( table->items == NULL ) {
        return -ENOMEM;
    }

    memset( table->items, 0, sizeof( hashitem_t* ) * size );

    table->size = size;
    table->key_func = key_func;
    table->hash_func = hash_func;
    table->compare_func = compare_func;

    return 0;
}

int hashtable_add( hashtable_t* table, hashitem_t* item ) {
    uint32_t hash;
    const void* key;

    key = table->key_func( item );
    hash = table->hash_func( key ) % table->size;

    item->next = table->items[ hash ];
    table->items[ hash ] = item;

    return 0;
}

hashitem_t* hashtable_get( hashtable_t* table, const void* key ) {
    uint32_t hash;
    hashitem_t* item;
    const void* other_key;

    hash = table->hash_func( key ) % table->size;
    item = table->items[ hash ];

    while ( item != NULL ) {
        other_key = table->key_func( item );

        if ( table->compare_func( key, other_key ) ) {
            break;
        }

        item = item->next;
    }

    return item;
}

int hashtable_remove( hashtable_t* table, const void* key ) {
    uint32_t hash;
    hashitem_t* prev;
    hashitem_t* item;
    const void* other_key;

    hash = table->hash_func( key ) & table->size;
    
    prev = NULL;
    item = table->items[ hash ];

    while ( item != NULL ) {
        other_key = table->key_func( item );

        if ( table->compare_func( key, other_key ) ) {
            if ( prev == NULL ) {
                table->items[ hash ] = item->next;
            } else {
                prev->next = item->next;
            }

            return 0;
        }

        prev = item;
        item = item->next;
    }

    return -EINVAL;
}
