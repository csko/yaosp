/* Image viewer application
 *
 * Copyright (c) 2010 Attila Magyar, Zoltan Kovacs
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

#include <ygui/bitmap.h>
#include <ygui/image.h>
#include <ygui/window.h>
#include <ygui/dialog/filechooser.h>

#include "ui.h"
#include "ui-event.h"
#include "ui-about.h"
#include "worker.h"
#include "util.h"

static int image_set_callback( void* data ) {
    bitmap_t* bitmap = (bitmap_t*)data;

    image_set_bitmap( image_widget, bitmap );
    bitmap_dec_ref(bitmap);

    return 0;
}

static int image_open_done( work_header_t* work ) {
    open_work_t* open = (open_work_t*)work;

    ui_set_statusbar( "Image %s loaded.", extract_filename(open->path) );

    if ( orig_bitmap != NULL ) {
        bitmap_dec_ref(orig_bitmap);
    }

    orig_bitmap = open->bitmap;

    ui_set_image_info( "Size: %dx%d", bitmap_get_width(orig_bitmap), bitmap_get_height(orig_bitmap) );

    bitmap_inc_ref(orig_bitmap);

    point_init(
        &current_size,
        bitmap_get_width(orig_bitmap),
        bitmap_get_height(orig_bitmap)
    );

    window_insert_callback( window, image_set_callback, (void*)orig_bitmap );

    return 0;
}

static int image_open_failed( work_header_t* work ) {
    open_work_t* open = (open_work_t*)work;

    ui_set_statusbar( "Failed to load image: %s.", extract_filename(open->path) );

    return 0;
}

static int image_resize_done( work_header_t* work ) {
    resize_work_t* resize = (resize_work_t*)work;

    window_insert_callback( window, image_set_callback, (void*)resize->output );

    return 0;
}

static int event_open_file_chooser_done( file_chooser_t* chooser, chooser_event_t event, void* data ) {
    if ( event == E_CHOOSER_OK ) {
        open_work_t* work = (open_work_t*)malloc( sizeof(open_work_t) );

        work->header.type = OPEN_IMAGE;
        work->header.done = image_open_done;
        work->header.failed = image_open_failed;
        work->path = file_chooser_get_selected_file( chooser );

        ui_set_statusbar( "Opening image: %s ...", extract_filename(work->path) );

        worker_put( (work_header_t*)work );
    }

    return 0;
}

int event_open_file( widget_t* widget, void* data ) {
    file_chooser_t* chooser;

    chooser = create_file_chooser( T_OPEN_DIALOG, "/", event_open_file_chooser_done, NULL );
    file_chooser_show( chooser );

    return 0;
}

int event_zoom_in( widget_t* widget, void* data ) {
    resize_work_t* work = (resize_work_t*)malloc( sizeof(resize_work_t) );

    work->header.type = RESIZE_IMAGE;
    work->header.done = image_resize_done;
    work->header.failed = NULL;

    work->input = orig_bitmap;
    point_add_xy( &current_size, 100, 100 );
    point_copy( &work->size, &current_size );

    worker_put( (work_header_t*)work );

    return 0;
}

int event_zoom_out( widget_t* widget, void* data ) {
    resize_work_t* work;

    if ( ( current_size.x <= 100 ) ||
         ( current_size.y <= 100 ) ) {
        return 0;
    }

    work = (resize_work_t*)malloc( sizeof(resize_work_t) );

    work->header.type = RESIZE_IMAGE;
    work->header.done = image_resize_done;
    work->header.failed = NULL;

    work->input = orig_bitmap;

    point_sub_xy( &current_size, 100, 100 );
    point_copy( &work->size, &current_size );

    worker_put( (work_header_t*)work );

    return 0;
}

int event_application_exit( widget_t* widget, void* data ) {
    window_close( window );

    return 0;
}

int event_help_about( widget_t* widget, void* data ) {
    ui_about_open();

    return 0;
}
