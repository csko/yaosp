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

#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>
#include <yaosp/ipc.h>

#include <ygui/bitmap.h>
#include <ygui/protocol.h>
#include <ygui/image/imageloader.h>

#define IMAGE_BUF_SIZE 32768

extern ipc_port_id app_reply_port;
extern ipc_port_id app_server_port;

extern pthread_mutex_t app_lock;

static bitmap_t* bitmap_allocate( int width, int height, color_space_t color_space ) {
    bitmap_t* bitmap;

    bitmap = ( bitmap_t* )calloc( 1, sizeof( bitmap_t ) );

    if ( bitmap == NULL ) {
        return NULL;
    }

    bitmap->id = -1;
    bitmap->ref_count = 1;
    bitmap->width = width;
    bitmap->height = height;
    bitmap->color_space = color_space;

    return bitmap;
}

bitmap_t* bitmap_create( int width, int height, color_space_t color_space ) {
    int error;
    bitmap_t* bitmap;
    msg_create_bitmap_t request;
    msg_create_bmp_reply_t reply;

    bitmap = bitmap_allocate( width, height, color_space );

    if ( bitmap == NULL ) {
        goto error1;
    }

    request.reply_port = app_reply_port;
    request.width = width;
    request.height = height;
    request.color_space = color_space;

    pthread_mutex_lock( &app_lock );

    error = send_ipc_message( app_server_port, MSG_BITMAP_CREATE, &request, sizeof( msg_create_bitmap_t ) );

    if ( error < 0 ) {
        pthread_mutex_unlock( &app_lock );

        goto error2;
    }

    error = recv_ipc_message( app_reply_port, NULL, &reply, sizeof( msg_create_bmp_reply_t ), INFINITE_TIMEOUT );

    pthread_mutex_unlock( &app_lock );

    if ( ( error < 0 ) ||
         ( reply.id < 0 ) ) {
        goto error2;
    }

    bitmap->id = reply.id;
    bitmap->region = memory_region_clone_pages( reply.bitmap_region, ( void** )&bitmap->data );

    if ( bitmap->region < 0 ) {
        goto error3;
    }

    return bitmap;

 error3:
    /* TODO: destroy the bitmap in the GUI server as well ... */

 error2:
    free( bitmap );

 error1:
    return NULL;
}

bitmap_t* bitmap_clone( int id ) {
    int error;
    bitmap_t* bitmap;
    msg_clone_bitmap_t request;
    msg_clone_bmp_reply_t reply;

    request.reply_port = app_reply_port;
    request.bitmap_id = id;

    pthread_mutex_lock( &app_lock );

    error = send_ipc_message( app_server_port, MSG_BITMAP_CLONE, &request, sizeof( msg_clone_bitmap_t ) );

    if ( error < 0 ) {
        pthread_mutex_unlock( &app_lock );

        goto error1;
    }

    error = recv_ipc_message( app_reply_port, NULL, &reply, sizeof( msg_clone_bmp_reply_t ), INFINITE_TIMEOUT );

    pthread_mutex_unlock( &app_lock );

    if ( ( error < 0 ) ||
         ( reply.bitmap_region < 0 ) ) {
        goto error1;
    }

    bitmap = bitmap_allocate( reply.width, reply.height, reply.color_space );

    if ( bitmap == NULL ) {
        goto error2;
    }

    bitmap->id = id;
    bitmap->region = memory_region_clone_pages( reply.bitmap_region, ( void** )&bitmap->data );

    if ( bitmap->region < 0 ) {
        goto error3;
    }

    return bitmap;

 error3:
    free( bitmap );

 error2:
    /* TODO: destroy the bitmap in the GUI server ... */

 error1:
    return NULL;
}

int bitmap_get_width( bitmap_t* bitmap ) {
    if ( bitmap == NULL ) {
        return 0;
    }

    return bitmap->width;
}

int bitmap_get_height( bitmap_t* bitmap ) {
    if ( bitmap == NULL ) {
        return 0;
    }

    return bitmap->height;
}

int bitmap_inc_ref( bitmap_t* bitmap ) {
    if ( bitmap == NULL ) {
        return -EINVAL;
    }

    bitmap->ref_count++;

    return 0;
}

int bitmap_dec_ref( bitmap_t* bitmap ) {
    if ( bitmap == NULL ) {
        return -EINVAL;
    }

    if ( --bitmap->ref_count == 0 ) {
        msg_delete_bitmap_t request;
        request.bitmap_id = bitmap->id;

        send_ipc_message( app_server_port, MSG_BITMAP_DELETE, &request, sizeof( msg_delete_bitmap_t ) );

        memory_region_delete( bitmap->region );
        free( bitmap );
    }

    return 0;
}

bitmap_t* bitmap_load_from_file( const char* file ) {
    int f;
    int data;
    int finalize;
    uint8_t* buf;

    int error;
    void* private;
    bitmap_t* bitmap = NULL;
    int bitmap_size = 0;
    uint8_t* bitmap_data = NULL;
    image_loader_t* loader;

    if ( file == NULL ) {
        return NULL;
    }

    /* Open the file. */

    f = open( file, O_RDONLY );

    if ( f < 0 ) {
        goto error1;
    }

    /* Create a buffer for reading from the image file. */

    buf = ( uint8_t* )malloc( IMAGE_BUF_SIZE );

    if ( buf == NULL ) {
        goto error2;
    }

    /* Read the first chunk of the data, so we can select
       an image loader for this image. */

    data = read( f, buf, IMAGE_BUF_SIZE );

    if ( data < 0 ) {
        goto error3;
    }

    /* Find the appropriate image loader */

    error = image_loader_find( buf, data, &loader, &private );

    if ( error < 0 ) {
        goto error3;
    }

    /* Pass the first (already read) chunk to the loader */

    finalize = ( data < IMAGE_BUF_SIZE );
    loader->add_data( private, buf, data, finalize );

    /* Check if we have some information available about the image size */

    if ( loader->get_available_size( private ) >= sizeof( image_info_t ) ) {
        image_info_t img_info;

        loader->read_data( private, ( uint8_t* )&img_info, sizeof( image_info_t ) );

        bitmap = bitmap_create( img_info.width, img_info.height, img_info.color_space );

        if ( bitmap == NULL ) {
            goto error4;
        }

        bitmap_data = bitmap->data;
        bitmap_size = img_info.width * img_info.height * colorspace_to_bpp( img_info.color_space );
    }

    while ( !finalize ) {
        data = read( f, buf, IMAGE_BUF_SIZE );
        finalize = ( data < IMAGE_BUF_SIZE );

        loader->add_data( private, buf, data, finalize );

        if ( bitmap == NULL ) {
            if ( loader->get_available_size( private ) >= sizeof( image_info_t ) ) {
                image_info_t img_info;

                loader->read_data( private, ( uint8_t* )&img_info, sizeof( image_info_t ) );

                bitmap = bitmap_create( img_info.width, img_info.height, img_info.color_space );

                if ( bitmap == NULL ) {
                    goto error4;
                }

                bitmap_data = bitmap->data;
                bitmap_size = img_info.width * img_info.height * colorspace_to_bpp( img_info.color_space );
            }
        } else {
            int avail;

            avail = loader->get_available_size( private );

            if ( avail > 0 ) {
                int size;

                size = loader->read_data( private, bitmap_data, avail );

                bitmap_data += size;
                bitmap_size -= size;
            }
        }
    }

    free( buf );
    close( f );

    while ( bitmap_size > 0 ) {
        int avail;

        avail = loader->get_available_size( private );

        if ( avail > 0 ) {
            int size;

            size = loader->read_data( private, bitmap_data, avail );

            bitmap_data += size;
            bitmap_size -= size;
        } else {
            break;
        }
    }

    loader->destroy( private );

    return bitmap;

 error4:
    loader->destroy( private );

 error3:
    free( buf );

 error2:
    close( f );

 error1:
    return NULL;
}
