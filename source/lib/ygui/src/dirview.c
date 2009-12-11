/* yaosp GUI library
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
#include <pthread.h>
#include <dirent.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <sys/stat.h>
#include <yaosp/time.h>

#include <ygui/dirview.h>
#include <ygui/bitmap.h>
#include <yutil/array.h>

#define LINE_HEIGHT 18

enum {
    E_ITEM_SELECTED,
    E_ITEM_DOUBLE_CLICKED,
    E_COUNT
};

typedef struct dir_item {
    char* name;
    directory_item_type_t type;
} dir_item_t;

typedef struct dir_view {
    char* path;

    array_t items;
    int pending_request;
    pthread_mutex_t lock;
    pthread_cond_t condition;
    pthread_t worker_thread;

    int selected;
    uint64_t click_time;

    font_t* font;
    bitmap_t* img_folder;
    bitmap_t* img_file;
} dir_view_t;

static color_t bg_color = { 255, 255, 255, 255 };
static color_t border_color = { 0, 0, 0, 255 };
static color_t black = { 0, 0, 0, 255 };
static color_t sel_color = { 51, 102, 152, 255 };

static int dirview_events[ E_COUNT ] = {
    -1,
    -1
};

static event_type_t dirview_event_types[ E_COUNT ] = {
    { "item-selected", &dirview_events[ E_ITEM_SELECTED ] },
    { "item-double-clicked", &dirview_events[ E_ITEM_DOUBLE_CLICKED ] }
};

static dir_item_t* create_dir_item( char* name, directory_item_type_t type ) {
    size_t name_len;
    dir_item_t* item;

    name_len = strlen( name );

    item = ( dir_item_t* )malloc( sizeof( dir_item_t ) + name_len + 1 );

    if ( item == NULL ) {
        return NULL;
    }

    item->name = ( char* )( item + 1 );
    item->type = type;
    memcpy( item->name, name, name_len + 1 );

    return item;
}

static int dir_item_comparator( const void* data1, const void* data2 ) {
    dir_item_t* item1 = *( dir_item_t** )data1;
    dir_item_t* item2 = *( dir_item_t** )data2;

    if ( ( item1->type == T_DIRECTORY ) &&
        ( item2->type == T_FILE ) ) {
        return -1;
    } else if ( ( item1->type == T_FILE ) &&
                ( item2->type == T_DIRECTORY ) ) {
        return 1;
    }

    return strcmp( item1->name, item2->name );
}

static void* dirview_worker( void* data ) {
    widget_t* widget;
    dir_view_t* dir_view;

    widget = ( widget_t* )data;
    dir_view = ( dir_view_t* )widget_get_data( widget );

    pthread_mutex_lock( &dir_view->lock );

    while ( 1 ) {
        int i;
        int size;
        DIR* dir;
        char path[ 256 ];
        struct dirent* entry;

        while ( !dir_view->pending_request ) {
            pthread_cond_wait( &dir_view->condition, &dir_view->lock );
        }

        dir_view->pending_request = 0;

        /* Empty the item array */

        size = array_get_size( &dir_view->items );

        for ( i = 0; i < size; i++ ) {
            free( array_get_item( &dir_view->items, i ) );
        }

        array_make_empty( &dir_view->items );

        /* Populate directory entries ... */

        dir = opendir( dir_view->path );

        if ( dir == NULL ) {
            continue;
        }

        int found_d = 0;
        int found_dd = 0;
        dir_item_t* dir_item;

        while ( ( entry = readdir( dir ) ) != NULL ) {
            dir_item = NULL;

            if ( strcmp( entry->d_name, "." ) == 0 ) {
                found_d = 1;
                dir_item = create_dir_item( entry->d_name, T_DIRECTORY );
            } else if ( strcmp( entry->d_name, ".." ) == 0 ) {
                found_dd = 1;
                dir_item = create_dir_item( entry->d_name, T_DIRECTORY );
            } else {
                struct stat st;

                snprintf( path, sizeof( path ), "%s/%s", dir_view->path, entry->d_name );

                if ( stat( path, &st ) == 0 ) {
                    dir_item = create_dir_item( entry->d_name, S_ISDIR( st.st_mode ) ? T_DIRECTORY : T_FILE );
                }
            }

            if ( dir_item != NULL ) {
                array_add_item( &dir_view->items, ( void* )dir_item );
            }
        }

        closedir( dir );

        if ( strcmp( dir_view->path, "/" ) != 0 ) {
            if ( !found_d ) {
                dir_item = create_dir_item( ".", T_DIRECTORY );
                array_add_item( &dir_view->items, ( void* )dir_item );
            }
            if ( !found_dd ) {
                dir_item = create_dir_item( "..", T_DIRECTORY );
                array_add_item( &dir_view->items, ( void* )dir_item );
            }
        }

        array_sort( &dir_view->items, dir_item_comparator );

        /* Invalidate the widget ... */

        pthread_mutex_unlock( &dir_view->lock );

        widget_signal_event_handler(
            widget,
            widget->event_ids[ E_PREF_SIZE_CHANGED ]
        );

        pthread_mutex_lock( &dir_view->lock );
    }

    pthread_mutex_unlock( &dir_view->lock );

    return NULL;
}

static int dirview_paint( widget_t* widget, gc_t* gc ) {
    int i;
    int size;
    rect_t bounds;
    dir_view_t* dir_view;
    point_t position;

    dir_view = ( dir_view_t* )widget_get_data( widget );

    widget_get_bounds( widget, &bounds );

    /* Border */

    gc_set_pen_color( gc, &border_color );
    gc_draw_rect( gc, &bounds );

    /* Background */

    rect_resize( &bounds, 1, 1, -1, -1 );

    gc_set_pen_color( gc, &bg_color );
    gc_fill_rect( gc, &bounds );

    /* Items ... */

    rect_resize( &bounds, 2, 2, -2, -2 );
    gc_set_clip_rect( gc, &bounds );

    gc_set_pen_color( gc, &black );
    gc_set_font( gc, dir_view->font );

    point_init( &position, 2, 2 );

    int asc = font_get_ascender( dir_view->font );
    int desc = font_get_descender( dir_view->font );
    int text_offset = ( 16 - ( asc - desc ) ) / 2 + asc;

    pthread_mutex_lock( &dir_view->lock );

    size = array_get_size( &dir_view->items );

    for ( i = 0; i < size; i++ ) {
        dir_item_t* item;

        item = ( dir_item_t* )array_get_item( &dir_view->items, i );

        if ( dir_view->selected == i ) {
            rect_t tmp;

            rect_init( &tmp, 0, position.y, bounds.right, position.y + LINE_HEIGHT - 1 );

            gc_set_pen_color( gc, &sel_color );
            gc_fill_rect( gc, &tmp );
            gc_set_pen_color( gc, &black );
        }

        point_add_xy( &position, 1, 1 );

        gc_set_drawing_mode( gc, DM_BLEND );
        switch ( item->type ) {
            case T_DIRECTORY : gc_draw_bitmap( gc, &position, dir_view->img_folder ); break;
            case T_FILE : gc_draw_bitmap( gc, &position, dir_view->img_file ); break;
            default : break;
        }
        gc_set_drawing_mode( gc, DM_COPY );

        position.y += text_offset;
        position.x += 18;
        gc_draw_text( gc, &position, item->name, -1 );
        position.x -= 18;

        position.x -= 1;
        position.y -= text_offset;
        position.y += 16;
        position.y += 1;
    }

    pthread_mutex_unlock( &dir_view->lock );

    return 0;
}

static int dirview_mouse_pressed( widget_t* widget, point_t* position, int button ) {
    uint64_t now;
    int old_selected;
    int new_selected;
    dir_view_t* dir_view;

    dir_view = ( dir_view_t* )widget_get_data( widget );
    new_selected = ( position->y / LINE_HEIGHT );

    pthread_mutex_lock( &dir_view->lock );

    old_selected = dir_view->selected;

    if ( new_selected >= array_get_size( &dir_view->items ) ) {
        new_selected = -1;
    }

    dir_view->selected = new_selected;

    pthread_mutex_unlock( &dir_view->lock );

    /* Notify ITEM_SELECTED listeners */

    widget_signal_event_handler( widget, dirview_events[ E_ITEM_SELECTED ] );

    now = get_system_time();

    if ( old_selected != new_selected ) {
        /* Invalidate the widget if the selected item is changed */

        dir_view->click_time = now;
        widget_invalidate( widget );
    } else {
        /* Notify ITEM_DOUBLE_CLICKED listeners if needed */

        #define DBL_CLICK_TIME ( 250 * 1000 )

        if ( ( now - dir_view->click_time ) <= DBL_CLICK_TIME ) {
            dir_view->click_time = 0;

            widget_signal_event_handler( widget, dirview_events[ E_ITEM_DOUBLE_CLICKED ] );
        } else {
            dir_view->click_time = now;
        }

        #undef DBL_CLICK_TIME
    }

    return 0;
}

static int dirview_get_preferred_size( widget_t* widget, point_t* size ) {
    dir_view_t* dir_view;

    dir_view = ( dir_view_t* )widget_get_data( widget );

    pthread_mutex_lock( &dir_view->lock );
    point_init(
        size,
        0,
        LINE_HEIGHT * array_get_size( &dir_view->items ) + 8
    );
    pthread_mutex_unlock( &dir_view->lock );

    return 0;
}

static widget_operations_t dirview_ops = {
    .paint = dirview_paint,
    .key_pressed = NULL,
    .key_released = NULL,
    .mouse_entered = NULL,
    .mouse_exited = NULL,
    .mouse_moved = NULL,
    .mouse_pressed = dirview_mouse_pressed,
    .mouse_released = NULL,
    .get_minimum_size = NULL,
    .get_preferred_size = dirview_get_preferred_size,
    .get_maximum_size = NULL,
    .do_validate = NULL,
    .size_changed = NULL,
    .added_to_window = NULL,
    .child_added = NULL
};

widget_t* create_directory_view( const char* path ) {
    widget_t* widget;
    dir_view_t* dir_view;
    font_properties_t properties;

    dir_view = ( dir_view_t* )malloc( sizeof( dir_view_t ) );

    if ( dir_view == NULL ) {
        goto error1;
    }

    if ( init_array( &dir_view->items ) != 0 ) {
        goto error2;
    }

    array_set_realloc_size( &dir_view->items, 32 );

    if ( pthread_mutex_init( &dir_view->lock, NULL ) != 0 ) {
        goto error3;
    }

    pthread_cond_init( &dir_view->condition, NULL ); /* todo: error checking */

    dir_view->path = strdup( path );

    if ( dir_view->path == NULL ) {
        goto error4;
    }

    properties.point_size = 8 * 64;
    properties.flags = FONT_SMOOTHED;

    dir_view->font = create_font( "DejaVu Sans", "Book", &properties );

    if ( dir_view->font == NULL ) {
        goto error5;
    }

    dir_view->pending_request = 1;
    dir_view->selected = -1;
    dir_view->click_time = 0;
    dir_view->img_folder = bitmap_load_from_file( "/system/images/folder.png" );
    dir_view->img_file = bitmap_load_from_file( "/system/images/file.png" );

    widget = create_widget( W_DIRVIEW, &dirview_ops, ( void* )dir_view );

    if ( widget == NULL ) {
        goto error6;
    }

    if ( widget_add_events( widget, dirview_event_types, dirview_events, E_COUNT ) != 0 ) {
        goto error7;
    }

    pthread_attr_t attrib;

    pthread_attr_init( &attrib );
    pthread_attr_setname( &attrib, "dirview_worker" );

    pthread_create(
        &dir_view->worker_thread,
        &attrib,
        dirview_worker,
        ( void* )widget
    );

    pthread_attr_destroy( &attrib );

    return widget;

 error7:
    /* todo: destroy the widget! */

 error6:
    /* todo: destroy the font! */

 error5:
    free( dir_view->path );

 error4:
    pthread_mutex_destroy( &dir_view->lock );

 error3:
    destroy_array( &dir_view->items );

 error2:
    free( dir_view );

 error1:
    return NULL;
}

char* directory_view_get_selected_item_name( widget_t* widget ) {
    char* name;
    dir_view_t* dir_view;

    if ( widget_get_id( widget ) != W_DIRVIEW ) {
        return NULL;
    }

    dir_view = ( dir_view_t* )widget_get_data( widget );

    if ( dir_view->selected < 0 ) {
        return NULL;
    }

    pthread_mutex_lock( &dir_view->lock );

    if ( dir_view->selected >= array_get_size( &dir_view->items ) ) {
        name = NULL;
    } else {
        dir_item_t* dir_item = array_get_item( &dir_view->items, dir_view->selected );
        name = strdup( dir_item->name );
    }

    pthread_mutex_unlock( &dir_view->lock );

    return name;
}

int directory_view_get_selected_item_type_and_name( widget_t* widget, directory_item_type_t* type, char** name ) {
    int error;
    dir_view_t* dir_view;
    dir_item_t* dir_item;

    if ( ( widget == NULL ) ||
         ( widget_get_id( widget ) != W_DIRVIEW ) ) {
        return -EINVAL;
    }

    dir_view = ( dir_view_t* )widget_get_data( widget );

    if ( dir_view->selected < 0 ) {
        return -ENOENT;
    }

    pthread_mutex_lock( &dir_view->lock );

    if ( dir_view->selected >= array_get_size( &dir_view->items ) ) {
        *type = T_NONE;
        *name = NULL;

        error = -EINVAL;
    } else {
        dir_item = array_get_item( &dir_view->items, dir_view->selected );

        *type = dir_item->type;
        *name = strdup( dir_item->name );

        error = 0;
    }

    pthread_mutex_unlock( &dir_view->lock );

    return error;
}

int directory_view_set_path( widget_t* widget, const char* path ) {
    dir_view_t* dir_view;

    if ( ( widget == NULL ) ||
         ( widget_get_id( widget ) != W_DIRVIEW ) ) {
        return -EINVAL;
    }

    dir_view = ( dir_view_t* )widget_get_data( widget );

    pthread_mutex_lock( &dir_view->lock );

    free( dir_view->path );
    dir_view->path = strdup( path ); /* todo: error checking */

    dir_view->selected = -1;
    dir_view->pending_request = 1;

    pthread_mutex_unlock( &dir_view->lock );
    pthread_cond_signal( &dir_view->condition );

    widget_invalidate( widget );

    return 0;
}
