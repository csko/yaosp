/* Image viewer application
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

#include <pthread.h>
#include <stdlib.h>
#include <yaosp/debug.h>

#include <yutil/array.h>

#include "worker.h"

static array_t work_list;
static pthread_mutex_t work_lock;
static pthread_cond_t work_sync;

static int worker_open_image( work_header_t* work ) {
    open_work_t* open = (open_work_t*)work;

    open->bitmap = bitmap_load_from_file(open->path);

    if ( open->bitmap == NULL ) {
        return -1;
    }

    return 0;
}

static int destroy_open_image( work_header_t* work ) {
    open_work_t* open = (open_work_t*)work;

    free(open->path);
    free(open);

    return 0;
}

static int worker_resize_image( work_header_t* work ) {
    resize_work_t* resize = (resize_work_t*)work;

    resize->output = bitmap_resize( resize->input, resize->size.x, resize->size.y, BICUBIC );

    if ( resize->output == NULL ) {
        return -1;
    }

    return 0;
}

static int destroy_resize_image( work_header_t* work ) {
    free(work);

    return 0;
}

static worker_function_t* worker_funcs[WORK_COUNT] = {
    worker_open_image,
    worker_resize_image
};

static worker_function_t* destroy_funcs[WORK_COUNT] = {
    destroy_open_image,
    destroy_resize_image
};

int worker_put( work_header_t* work ) {
    pthread_mutex_lock(&work_lock);
    array_add_item( &work_list, work );
    pthread_mutex_unlock(&work_lock);

    pthread_cond_signal(&work_sync);

    return 0;
}

static void* worker_thread( void* arg ) {
    pthread_mutex_lock( &work_lock );

    while ( 1 ) {
        int ret;
        work_header_t* work;

        while ( array_get_size(&work_list) == 0 ) {
            pthread_cond_wait( &work_sync, &work_lock );
        }

        work = array_get_item( &work_list, 0 );
        array_remove_item_from( &work_list, 0 );

        ret = worker_funcs[work->type](work);

        if ( ret == 0 ) {
            if ( work->done != NULL ) { work->done(work); }
        } else {
            if ( work->failed != NULL ) { work->failed(work); }
        }

        destroy_funcs[work->type](work);
    }

    return NULL;
}

int worker_init( void ) {
    init_array( &work_list );

    pthread_mutex_init( &work_lock, NULL );
    pthread_cond_init( &work_sync, NULL );

    return 0;
}

int worker_start( void ) {
    pthread_t thread;

    pthread_create( &thread, NULL, worker_thread, NULL );

    return 0;
}
