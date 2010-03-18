/* Desktop application
 *
 * Copyright (c) 2010 Zoltan Kovacs
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
#include <string.h>
#include <stdlib.h>
#include <pthread.h>

#include <ygui/window.h>
#include <ygui/image.h>
#include <yutil/array.h>

typedef enum {
    CHANGE,
    RESIZE
} wallpaper_cmd_t;

typedef struct {
    wallpaper_cmd_t command;
} wallpaper_header_t;

typedef struct {
    wallpaper_cmd_t command;
    int width;
    int height;
} wallpaper_resize_t;

point_t wallpaper_size;

static pthread_t thread;
static array_t cmd_queue;
static pthread_cond_t cmd_sync;
static pthread_mutex_t cmd_lock;

static bitmap_t* img_wallpaper = NULL;

extern window_t* window;
extern widget_t* image_wallpaper;

static int wallpaper_set_image( void* data ) {
    bitmap_t* bitmap;

    bitmap = (bitmap_t*)data;
    image_set_bitmap(image_wallpaper, bitmap);
    bitmap_dec_ref(bitmap);

    return 0;
}

static int wallpaper_do_change( char* filename ) {
    bitmap_t* bitmap;

    bitmap = bitmap_load_from_file(filename);

    if ( bitmap == NULL ) {
        return 0;
    }

    if ( img_wallpaper != NULL ) {
        bitmap_dec_ref(img_wallpaper);
    }

    img_wallpaper = bitmap;
    bitmap = bitmap_resize(img_wallpaper, wallpaper_size.x, wallpaper_size.y, BILINEAR);
    window_insert_callback( window, wallpaper_set_image, (void*)bitmap );

    return 0;
}

static int wallpaper_do_resize( wallpaper_resize_t* resize ) {
    bitmap_t* bitmap;

    bitmap = bitmap_resize(img_wallpaper, resize->width, resize->height, BILINEAR);
    window_insert_callback( window, wallpaper_set_image, (void*)bitmap );

    return 0;
}

static void* wallpaper_thread( void* arg ) {
    while ( 1 ) {
        wallpaper_header_t* header;

        pthread_mutex_lock(&cmd_lock);

        while ( array_get_size(&cmd_queue) == 0 ) {
            pthread_cond_wait( &cmd_sync, &cmd_lock );
        }

        header = (wallpaper_header_t*)array_get_item( &cmd_queue, 0 );
        array_remove_item_from( &cmd_queue, 0 );

        pthread_mutex_unlock(&cmd_lock );

        switch ( header->command ) {
            case CHANGE :
                wallpaper_do_change( (char*)( header + 1 ) );
                break;

            case RESIZE :
                wallpaper_do_resize( (wallpaper_resize_t*)header );
                break;
        }

        free(header);
    }

    return NULL;
}

int wallpaper_set( char* filename ) {
    size_t length;
    wallpaper_header_t* header;

    length = sizeof(wallpaper_header_t) + strlen(filename) + 1;
    header = (wallpaper_header_t*)malloc(length);

    if ( header == NULL ) {
        return -ENOMEM;
    }

    header->command = CHANGE;
    strcpy( (char*)( header + 1 ), filename );

    pthread_mutex_lock(&cmd_lock);
    array_add_item( &cmd_queue, (void*)header );
    pthread_mutex_unlock(&cmd_lock);
    pthread_cond_signal(&cmd_sync );

    return 0;
}

int wallpaper_resize( int width, int height ) {
    wallpaper_resize_t* resize;

    resize = (wallpaper_resize_t*)malloc( sizeof(wallpaper_resize_t) );

    if ( resize == NULL ) {
        return -ENOMEM;
    }

    resize->command = RESIZE;
    resize->width = width;
    resize->height = height;

    pthread_mutex_lock(&cmd_lock);
    array_add_item( &cmd_queue, (void*)resize );
    pthread_mutex_unlock(&cmd_lock);
    pthread_cond_signal(&cmd_sync );

    return 0;
}

int wallpaper_init( void ) {
    init_array( &cmd_queue );
    pthread_cond_init( &cmd_sync, NULL );
    pthread_mutex_init( &cmd_lock, NULL );

    return 0;
}

int wallpaper_start( void ) {
    pthread_create( &thread, NULL, wallpaper_thread, NULL );

    return 0;
}
