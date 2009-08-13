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
    table->item_count = 0;
    table->key_func = key_func;
    table->hash_func = hash_func;
    table->compare_func = compare_func;

    return 0;
}

void destroy_hashtable( hashtable_t* table ) {
    kfree( table->items );
    table->items = NULL;
}

static void hashtable_resize( hashtable_t* table, uint32_t new_size ) {
    uint32_t i;
    uint32_t hash;
    const void* key;
    hashitem_t* tmp;
    hashitem_t* item;
    hashitem_t** new_items;

    new_items = ( hashitem_t** )kmalloc( sizeof( hashitem_t* ) * new_size );

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

    kfree( table->items );

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

int hashtable_iterate( hashtable_t* table, hashtable_iter_callback_t* callback, void* data ) {
    int result;
    uint32_t i;
    hashitem_t* item;
    hashitem_t* next;

    for ( i = 0; i < table->size; i++ ) {
        item = table->items[ i ];

        while ( item != NULL ) {
            next = item->next;
            result = callback( item, data );

            if ( result < 0 ) {
                return result;
            }

            item = next;
        }
    }

    return 0;
}

int hashtable_filtered_iterate( hashtable_t* table, hashtable_iter_callback_t* callback, void* data, hashtable_filter_callback_t* filter, void* filter_data ) {
    int result;
    uint32_t i;
    hashitem_t* item;
    hashitem_t* next;

    for ( i = 0; i < table->size; i++ ) {
        item = table->items[ i ];

        while ( item != NULL ) {
            next = item->next;
            result = filter( item, filter_data );

            if ( result < 0 ) {
                goto next;
            }

            result = callback( item, data );

            if ( result < 0 ) {
                return result;
            }

next:
            item = next;
        }
    }

    return 0;
}

uint32_t hashtable_get_item_count( hashtable_t* table ) {
    return table->item_count;
}

uint32_t hashtable_get_filtered_item_count( hashtable_t* table, hashtable_filter_callback_t* filter, void* data ) {
    int result;
    uint32_t i;
    uint32_t count;
    hashitem_t* item;

    count = 0;

    for ( i = 0; i < table->size; i++ ) {
        item = table->items[ i ];

        while ( item != NULL ) {
            result = filter( item, data );

            if ( result == 0 ) {
                count++;
            }

            item = item->next;
        }
    }

    return count;
}

uint32_t hash_number( uint8_t* data, size_t length ) {
    size_t i;
    uint32_t hash;

    hash = 0;

    for ( i = 0; i < length; i++, data++ ) {
        hash += *data;
        hash += ( hash << 10 );
        hash ^= ( hash >> 6 );
    }

    hash += ( hash << 3 );
    hash ^= ( hash >> 11 );
    hash += ( hash << 15 );

    return hash;
}

uint32_t hash_string( uint8_t* data, size_t length ) {
    size_t i;
    uint32_t hash = 2166136261U;

    for ( i = 0; i < length; i++, data++ ) {
        hash = ( hash ^ ( *data ) ) * 16777619;
    }

    hash += hash << 13;
    hash ^= hash >> 7;
    hash += hash << 3;
    hash ^= hash >> 17;
    hash += hash << 5;

    return hash;
}

uint32_t hash_int( const void* key ) {
    return hash_number( ( uint8_t* )key, sizeof( int ) );
}

uint32_t hash_int64( const void* key ) {
    return hash_number( ( uint8_t* )key, sizeof( uint64_t ) );
}

uint32_t hash_str( const void* key ) {
    const char* str;

    str = ( const char* )key;

    return hash_string( ( uint8_t* )str, strlen( str ) );
}

bool compare_int( const void* key1, const void* key2 ) {
    int* int1;
    int* int2;

    int1 = ( int* )key1;
    int2 = ( int* )key2;

    return ( *int1 == *int2 );
}

bool compare_int64( const void* key1, const void* key2 ) {
    uint64_t* int1;
    uint64_t* int2;

    int1 = ( uint64_t* )key1;
    int2 = ( uint64_t* )key2;

    return ( *int1 == *int2 );
}

bool compare_str( const void* key1, const void* key2 ) {
    const char* str1;
    const char* str2;

    str1 = ( const char* )key1;
    str2 = ( const char* )key2;

    return ( strcmp( str1, str2 ) == 0 );
}

