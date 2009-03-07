/* Bitmap implementation
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
#include <yaosp/semaphore.h>

#include <bitmap.h>

static int bitmap_id_counter;
static hashtable_t bitmap_table;
static semaphore_id bitmap_lock;

static int insert_bitmap( bitmap_t* bitmap ) {
    lock_semaphore( bitmap_lock, 1, INFINITE_TIMEOUT );

    do {
        bitmap->id = bitmap_id_counter++;

        if ( bitmap_id_counter < 0 ) {
            bitmap_id_counter = 0;
        }
    } while ( hashtable_get( &bitmap_table, ( const void* )&bitmap->id ) != NULL );

    hashtable_add( &bitmap_table, ( hashitem_t* )bitmap );

    unlock_semaphore( bitmap_lock, 1 );

    return 0;
}

bitmap_t* create_bitmap_from_buffer( uint32_t width, uint32_t height, color_space_t color_space, void* buffer ) {
    int error;
    bitmap_t* bitmap;

    bitmap = ( bitmap_t* )malloc( sizeof( bitmap_t ) );

    if ( bitmap == NULL ) {
        return NULL;
    }

    bitmap->ref_count = 1;
    bitmap->width = width;
    bitmap->height = height;
    bitmap->color_space = color_space;
    bitmap->buffer = buffer;

    error = insert_bitmap( bitmap );

    if ( error < 0 ) {
        free( bitmap );
        return NULL;
    }

    return bitmap;
}

static void* bitmap_key( hashitem_t* item ) {
    bitmap_t* bitmap;

    bitmap = ( bitmap_t* )item;

    return ( void* )&bitmap->id;
}

static uint32_t bitmap_hash( const void* key ) {
    return hash_number( ( uint8_t* )key, sizeof( bitmap_id ) );
}

static int bitmap_compare( const void* key1, const void* key2 ) {
    bitmap_id* id1;
    bitmap_id* id2;

    id1 = ( bitmap_id* )key1;
    id2 = ( bitmap_id* )key2;

    return ( *id1 == *id2 );
}

int init_bitmap( void ) {
    int error;

    error = init_hashtable(
        &bitmap_table,
        256,
        bitmap_key,
        bitmap_hash,
        bitmap_compare
    );

    if ( error < 0 ) {
        goto error1;
    }

    bitmap_lock = create_semaphore( "bitmap lock", SEMAPHORE_BINARY, 0, 1 );

    if ( bitmap_lock < 0 ) {
        error = bitmap_lock;
        goto error2;
    }

    bitmap_id_counter = 0;

    return 0;

error2:
    destroy_hashtable( &bitmap_table );

error1:
    return error;
}
