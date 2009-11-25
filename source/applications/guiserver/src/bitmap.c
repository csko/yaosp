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
#include <assert.h>
#include <string.h>
#include <pthread.h>

#include <bitmap.h>

static int bitmap_id_counter;
static hashtable_t bitmap_table;
static pthread_mutex_t bitmap_lock;

static int insert_bitmap( bitmap_t* bitmap ) {
    int error;

    do {
        bitmap->id = bitmap_id_counter++;

        if ( bitmap_id_counter < 0 ) {
            bitmap_id_counter = 0;
        }
    } while ( hashtable_get( &bitmap_table, ( const void* )&bitmap->id ) != NULL );

    error = hashtable_add( &bitmap_table, ( hashitem_t* )bitmap );

    if ( error < 0 ) {
        return error;
    }

    return 0;
}

bitmap_t* create_bitmap( uint32_t width, uint32_t height, color_space_t color_space ) {
    int error;
    void* buffer;
    bitmap_t* bitmap;
    uint32_t buffer_size;

    bitmap = ( bitmap_t* )malloc( sizeof( bitmap_t ) );

    if ( bitmap == NULL ) {
        goto error1;
    }

    bitmap->bytes_per_line = width * colorspace_to_bpp( color_space );
    buffer_size = bitmap->bytes_per_line * height;

    buffer = malloc( buffer_size );

    if ( buffer == NULL ) {
        goto error2;
    }

    memset( buffer, 0, buffer_size );

    bitmap->ref_count = 1;
    bitmap->width = width;
    bitmap->height = height;
    bitmap->color_space = color_space;
    bitmap->buffer = buffer;
    bitmap->flags = BITMAP_FREE_BUFFER;
    bitmap->region = -1;

    pthread_mutex_lock( &bitmap_lock );

    error = insert_bitmap( bitmap );

    pthread_mutex_unlock( &bitmap_lock );

    if ( error < 0 ) {
        goto error3;
    }

    return bitmap;

error3:
    free( buffer );

error2:
    free( bitmap );

error1:
    return NULL;
}

bitmap_t* create_bitmap_from_buffer( uint32_t width, uint32_t height, color_space_t color_space, void* buffer ) {
    int error;
    bitmap_t* bitmap;

    bitmap = ( bitmap_t* )malloc( sizeof( bitmap_t ) );

    if ( bitmap == NULL ) {
        goto error1;
    }

    bitmap->ref_count = 1;
    bitmap->width = width;
    bitmap->height = height;
    bitmap->bytes_per_line = width * colorspace_to_bpp( color_space );
    bitmap->color_space = color_space;
    bitmap->buffer = buffer;
    bitmap->flags = 0;
    bitmap->region = -1;

    pthread_mutex_lock( &bitmap_lock );

    error = insert_bitmap( bitmap );

    pthread_mutex_unlock( &bitmap_lock );

    if ( error < 0 ) {
        goto error2;
    }

    return bitmap;

error2:
    free( bitmap );

error1:
    return NULL;
}

bitmap_t* bitmap_get( bitmap_id id ) {
    bitmap_t* bitmap;

    pthread_mutex_lock( &bitmap_lock );

    bitmap = ( bitmap_t* )hashtable_get( &bitmap_table, ( const void* )&id );

    if ( bitmap != NULL ) {
        bitmap->ref_count++;
    }

    pthread_mutex_unlock( &bitmap_lock );

    return bitmap;
}

static int do_bitmap_put_times( bitmap_t* bitmap, int times ) {
    int do_delete;

    do_delete = 0;

    pthread_mutex_lock( &bitmap_lock );

    assert( bitmap->ref_count >= times );
    bitmap->ref_count -= times;

    if ( bitmap->ref_count == 0 ) {
        hashtable_remove( &bitmap_table, ( const void* )&bitmap->id );
        do_delete = 1;
    }

    pthread_mutex_unlock( &bitmap_lock );

    if ( do_delete ) {
        if ( bitmap->flags & BITMAP_FREE_BUFFER ) {
            free( bitmap->buffer );
            bitmap->buffer = NULL;
        }

        if ( bitmap->region != -1 ) {
            memory_region_delete( bitmap->region );
            bitmap->region = -1;
        }

        free( bitmap );
    }

    return 0;
}

int bitmap_put( bitmap_t* bitmap ) {
    return do_bitmap_put_times( bitmap, 1 );
}

int handle_create_bitmap( application_t* app, msg_create_bitmap_t* request ) {
    int size;
    void* buffer;
    bitmap_t* bitmap;
    region_id bitmap_region;
    msg_create_bmp_reply_t reply;

    size = request->width * request->height * colorspace_to_bpp( request->color_space );

    bitmap_region = memory_region_create( "bitmap", PAGE_ALIGN( size ), REGION_READ | REGION_WRITE, &buffer );

    if ( bitmap_region < 0 ) {
        goto error1;
    }

    if ( memory_region_alloc_pages( bitmap_region ) != 0 ) {
        goto error2;
    }

    bitmap = create_bitmap_from_buffer( request->width, request->height, request->color_space, buffer );

    if ( bitmap == NULL ) {
        goto error2;
    }

    bitmap->region = bitmap_region;

    reply.id = bitmap->id;
    reply.bitmap_region = bitmap_region;

    application_insert_bitmap( app, bitmap );

    send_ipc_message( request->reply_port, 0, &reply, sizeof( msg_create_bmp_reply_t ) );

    return 0;

 error2:
    memory_region_delete( bitmap_region );

 error1:
    reply.id = -1;

    send_ipc_message( request->reply_port, 0, &reply, sizeof( msg_create_bmp_reply_t ) );

    return 0;
}

int handle_clone_bitmap( application_t* app, msg_clone_bitmap_t* request ) {
    bitmap_t* bitmap;
    msg_clone_bmp_reply_t reply;

    bitmap = bitmap_get( request->bitmap_id );

    if ( ( bitmap == NULL ) ||
         ( bitmap->region == -1 ) ) {
        goto error1;
    }

    reply.width = bitmap->width;
    reply.height = bitmap->height;
    reply.color_space = bitmap->color_space;
    reply.bitmap_region = bitmap->region;

    application_insert_bitmap( app, bitmap );

    send_ipc_message( request->reply_port, 0, &reply, sizeof( msg_clone_bmp_reply_t ) );

    return 0;

 error1:
    reply.bitmap_region = -1;

    send_ipc_message( request->reply_port, 0, &reply, sizeof( msg_clone_bmp_reply_t ) );

    return 0;
}

int handle_delete_bitmap( application_t* app, msg_delete_bitmap_t* request ) {
    bitmap_t* bitmap;

    bitmap = bitmap_get( request->bitmap_id );

    if ( bitmap == NULL ) {
        return -1;
    }

    do_bitmap_put_times( bitmap, 2 );

    application_remove_bitmap( app, bitmap );

    return 0;
}

static void* bitmap_key( hashitem_t* item ) {
    bitmap_t* bitmap;

    bitmap = ( bitmap_t* )item;

    return ( void* )&bitmap->id;
}

int init_bitmap( void ) {
    int error;

    error = init_hashtable(
        &bitmap_table, 256,
        bitmap_key, hash_int, compare_int
    );

    if ( error < 0 ) {
        goto error1;
    }

    error = pthread_mutex_init( &bitmap_lock, NULL );

    if ( error < 0 ) {
        goto error2;
    }

    bitmap_id_counter = 0;

    return 0;

error2:
    destroy_hashtable( &bitmap_table );

error1:
    return error;
}
