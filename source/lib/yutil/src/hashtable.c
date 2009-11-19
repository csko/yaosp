/* Hashtable implementation
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

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <yutil/hashtable.h>

int init_hashtable(
    hashtable_t* table,
    uint32_t size,
    key_function_t* key_func,
    hash_function_t* hash_func,
    compare_function_t* compare_func
) {
    table->items = ( hashitem_t** )malloc( sizeof( hashitem_t* ) * size );

    if ( table->items == NULL ) {
        return -ENOMEM;
    }

    memset( table->items, 0, sizeof( hashitem_t* ) * size );

    table->size = size;
    table->item_count = 0;
    table->key_func = key_func;
    table->hash_func = hash_func;
    table->compare_func = compare_func;

    return 0;
}

void destroy_hashtable( hashtable_t* table ) {
    free( table->items );
    table->items = NULL;
}

static void hashtable_resize( hashtable_t* table, uint32_t new_size ) {
    uint32_t i;
    uint32_t hash;
    const void* key;
    hashitem_t* tmp;
    hashitem_t* item;
    hashitem_t** new_items;

    new_items = ( hashitem_t** )malloc( sizeof( hashitem_t* ) * new_size );

    if ( new_items == NULL ) {
        return;
    }

    memset( new_items, 0, sizeof( hashitem_t* ) * new_size );

    for ( i = 0; i < table->size; i++ ) {
        item = table->items[ i ];

        while ( item != NULL ) {
            tmp = item;
            item = item->next;

            key = table->key_func( tmp );
            hash = table->hash_func( key ) % new_size;

            tmp->next = new_items[ hash ];
            new_items[ hash ] = tmp;
        }
    }

    free( table->items );

    table->items = new_items;
    table->size = new_size;
}

int hashtable_add( hashtable_t* table, hashitem_t* item ) {
    uint32_t hash;
    const void* key;

    key = table->key_func( item );
    hash = table->hash_func( key ) % table->size;

    item->next = table->items[ hash ];
    table->items[ hash ] = item;

    table->item_count++;

    if ( table->item_count >= table->size ) {
        hashtable_resize( table, table->size * 2 );
    }

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

    hash = table->hash_func( key ) % table->size;

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

            table->item_count--;

            return 0;
        }

        prev = item;
        item = item->next;
    }

    return -EINVAL;
}

uint32_t do_hash_number( uint8_t* data, size_t length ) {
    size_t i;
    uint32_t hash;

    hash = 0;

    for ( i = 0; i < length; i++ ) {
        hash += data[ i ];
        hash += ( hash << 10 );
        hash ^= ( hash >> 6 );
    }

    hash += ( hash << 3 );
    hash ^= ( hash >> 11 );
    hash += ( hash << 15 );

    return hash;
}

uint32_t do_hash_string( uint8_t* data, size_t length ) {
    size_t i;
    uint32_t hash = 2166136261U;

    for ( i = 0; i < length; i++ ) {
        hash = ( hash ^ data[ i ] ) * 16777619;
    }

    hash += hash << 13;
    hash ^= hash >> 7;
    hash += hash << 3;
    hash ^= hash >> 17;
    hash += hash << 5;

    return hash;
}

uint32_t hash_int( const void* key ) {
    return do_hash_number( ( uint8_t* )key, sizeof( int ) );
}

uint32_t hash_string( const void* key ) {
    return do_hash_string( ( uint8_t* )key, strlen( ( const char* )key ) );
}

int compare_int( const void* _key1, const void* _key2 ) {
    int* key1 = ( int* )_key1;
    int* key2 = ( int* )_key2;

    return ( *key1 == *key2 );
}

int compare_string( const void* _key1, const void* _key2 ) {
    const char* key1 = ( const char* )_key1;
    const char* key2 = ( const char* )_key2;

    return ( strcmp( key1, key2 ) == 0 );
}
