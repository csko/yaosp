/* yaosp GUI library
 *
 * Copyright (c) 2009, 2010 Zoltan Kovacs
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
#include <errno.h>

#include <ygui/image.h>
#include <ygui/region.h>

typedef struct image {
    bitmap_t* bitmap;
} image_t;

static int image_paint( widget_t* widget, gc_t* gc ) {
    rect_t bounds;
    image_t* image;
    point_t position;

    widget_get_bounds( widget, &bounds );

    image = ( image_t* )widget_get_data( widget );

    if ( image->bitmap == NULL ) {
        return 0;
    }

    int bitmap_w = bitmap_get_width(image->bitmap);
    int bitmap_h = bitmap_get_height(image->bitmap);
    rect_t bitmap_rect;

    if ( rect_width( &bounds ) > bitmap_get_width( image->bitmap ) ) {
        position.x = ( rect_width( &bounds ) - bitmap_w ) / 2;
    } else {
        position.x = 0;
    }

    if ( rect_height( &bounds ) > bitmap_get_height( image->bitmap ) ) {
        position.y = ( rect_height( &bounds ) - bitmap_h ) / 2;
    } else {
        position.y = 0;
    }

    rect_init(
        &bitmap_rect,
        position.x, position.y,
        position.x + bitmap_w - 1, position.y + bitmap_h - 1
    );

    gc_set_drawing_mode( gc, DM_BLEND );
    gc_draw_bitmap( gc, &position, image->bitmap );
    gc_set_drawing_mode( gc, DM_COPY );

    /* Fill those parts of the widget that is not covered by the image itself. */

    static color_t black = { 0, 0, 0, 255 };
    gc_set_pen_color( gc, &black );

    region_t region;
    clip_rect_t* clip_rect;

    region_init(&region);
    region_add(&region, &bounds);
    region_exclude(&region, &bitmap_rect);

    for ( clip_rect = region.rects; clip_rect != NULL; clip_rect = clip_rect->next ) {
        gc_fill_rect( gc, &clip_rect->rect );
    }

    region_destroy(&region);

    return 0;
}

static int image_get_preferred_size( widget_t* widget, point_t* size ) {
    image_t* image;

    image = ( image_t* )widget_get_data( widget );

    if ( image->bitmap == NULL ) {
        point_init( size, 0, 0 );
    } else {
        point_init(
            size,
            bitmap_get_width( image->bitmap ),
            bitmap_get_height( image->bitmap )
        );
    }

    return 0;
}

static int image_destroy( widget_t* widget ) {
    image_t* image;

    image = ( image_t* )widget_get_data( widget );

    bitmap_dec_ref( image->bitmap );
    free( image );

    return 0;
}

static widget_operations_t image_ops = {
    .paint = image_paint,
    .key_pressed = NULL,
    .key_released = NULL,
    .mouse_entered = NULL,
    .mouse_exited = NULL,
    .mouse_moved = NULL,
    .mouse_pressed = NULL,
    .mouse_released = NULL,
    .mouse_scrolled = NULL,
    .get_minimum_size = NULL,
    .get_preferred_size = image_get_preferred_size,
    .get_maximum_size = NULL,
    .get_viewport = NULL,
    .do_validate = NULL,
    .size_changed = NULL,
    .added_to_window = NULL,
    .child_added = NULL,
    .destroy = image_destroy
};

widget_t* create_image( bitmap_t* bitmap ) {
    image_t* image;
    widget_t* widget;

    image = ( image_t* )malloc( sizeof( image_t ) );

    if ( image == NULL ) {
        goto error1;
    }

    widget = create_widget( W_IMAGE, WIDGET_NONE, &image_ops, image );

    if ( widget == NULL ) {
        goto error2;
    }

    image->bitmap = bitmap;

    if ( image->bitmap != NULL ) {
        bitmap_inc_ref( image->bitmap );
    }

    return widget;

 error2:
    free( image );

 error1:
    return NULL;
}

int image_set_bitmap( widget_t* widget, bitmap_t* bitmap ) {
    image_t* image;

    if ( ( widget == NULL ) ||
         ( widget_get_id( widget ) != W_IMAGE ) ) {
        return -EINVAL;
    }

    image = ( image_t* )widget_get_data( widget );

    if ( image->bitmap != NULL ) {
        bitmap_dec_ref( image->bitmap );
    }

    image->bitmap = bitmap;

    if ( image->bitmap != NULL ) {
        bitmap_inc_ref( image->bitmap );
    }

    widget_signal_event_handler(
        widget,
        widget->event_ids[ E_PREF_SIZE_CHANGED ]
    );

    widget_invalidate( widget );

    return 0;
}
