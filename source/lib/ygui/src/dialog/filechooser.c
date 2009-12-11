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
#include <errno.h>
#include <stdio.h>

#include <ygui/panel.h>
#include <ygui/button.h>
#include <ygui/textfield.h>
#include <ygui/dirview.h>
#include <ygui/label.h>
#include <ygui/scrollpanel.h>
#include <ygui/dialog/filechooser.h>
#include <ygui/layout/borderlayout.h>

static int file_chooser_window_closed( window_t* window, void* data ) {
    /* todo: destroy the file chooser stuffs */

    return 0;
}

static int file_chooser_open_pressed( widget_t* widget, void* data ) {
    file_chooser_t* chooser;

    chooser = ( file_chooser_t* )data;
    window_close( chooser->window );

    if ( chooser->callback != NULL ) {
        chooser->callback( chooser, E_CHOOSER_OK, chooser->data );
    }

    /* todo: destroy the file chooser stuffs */

    return 0;
}

static int file_chooser_item_selected( widget_t* widget, void* data ) {
    char* name;
    file_chooser_t* chooser;

    chooser = ( file_chooser_t* )data;
    name = directory_view_get_selected_item_name( widget );

    if ( name != NULL ) {
        textfield_set_text( chooser->filename_field, name );
        free( name );
    }

    return 0;
}

static int file_chooser_item_double_clicked( widget_t* widget, void* data ) {
    char* name;
    file_chooser_t* chooser;
    directory_item_type_t type;

    if ( directory_view_get_selected_item_type_and_name( widget, &type, &name ) != 0 ) {
        return 0;
    }

    chooser = ( file_chooser_t* )data;

    if ( type == T_DIRECTORY ) {
        char path[ 256 ];
        char* new_path;

        if ( strcmp( name, "." ) == 0 ) {
            /* do nothing */
            goto out;
        } else if ( strcmp( name, ".." ) == 0 ) {
            char* pos;

            snprintf( path, sizeof( path ), "%s", chooser->current_path );

            pos = strrchr( path, '/' );

            if ( pos == path ) {
                pos++;
            }

            *pos = 0;
        } else {
            if ( strcmp( chooser->current_path, "/" ) == 0 ) {
                snprintf( path, sizeof( path ), "/%s", name );
            } else {
                snprintf( path, sizeof( path ), "%s/%s", chooser->current_path, name );
            }
        }

        new_path = strdup( path );

        if ( new_path != NULL ) {
            free( chooser->current_path );
            chooser->current_path = new_path;

            label_set_text( chooser->path_label, new_path );
            widget_set_scroll_offset( chooser->directory_view, NULL );

            directory_view_set_path( chooser->directory_view, new_path );
        }
    }

 out:
    free( name );

    return 0;
}

file_chooser_t* create_file_chooser( chooser_type_t type, const char* path,
                                     file_chooser_callback_t* callback, void* data ) {
    file_chooser_t* chooser;

    chooser = ( file_chooser_t* )malloc( sizeof( file_chooser_t ) );

    if ( chooser == NULL ) {
        goto error1;
    }

    chooser->current_path = strdup( path ); /* todo: check return value! */
    chooser->callback = callback;
    chooser->data = data;

    point_t position = { 50, 50 };
    point_t size = { 275, 325 };

    chooser->window = create_window(
        type == T_OPEN_DIALOG ? "Open file" : "Save file",
        &position, &size, WINDOW_NONE
    );

    if ( chooser->window == NULL ) {
        goto error2;
    }

    window_set_event_handler( chooser->window, WE_CLOSED, file_chooser_window_closed, ( void* )chooser );

    widget_t* container = window_get_container( chooser->window );

    layout_t* layout = create_border_layout();
    panel_set_layout( container, layout );
    layout_dec_ref( layout );

    chooser->path_label = create_label( path );
    label_set_horizontal_alignment( chooser->path_label, H_ALIGN_LEFT );
    widget_add( container, chooser->path_label, BRD_PAGE_START );
    widget_dec_ref( chooser->path_label );

    widget_t* scrollpanel = create_scroll_panel( SCROLLBAR_ALWAYS, SCROLLBAR_ALWAYS );
    widget_add( container, scrollpanel, BRD_CENTER );

    chooser->directory_view = create_directory_view( path );
    widget_add( scrollpanel, chooser->directory_view, NULL );
    widget_dec_ref( chooser->directory_view );
    widget_dec_ref( scrollpanel );

    widget_connect_event_handler(
        chooser->directory_view, "item-selected",
        file_chooser_item_selected, ( void* )chooser
    );
    widget_connect_event_handler(
        chooser->directory_view, "item-double-clicked",
        file_chooser_item_double_clicked, ( void* )chooser
    );

    widget_t* panel = create_panel();
    widget_add( container, panel, BRD_PAGE_END );
    widget_dec_ref( panel );

    layout = create_border_layout();
    panel_set_layout( panel, layout );
    layout_dec_ref( layout );

    chooser->filename_field = create_textfield();
    widget_add( panel, chooser->filename_field, BRD_CENTER );
    widget_dec_ref( chooser->filename_field );

    widget_t* button = create_button( type == T_OPEN_DIALOG ? "Open" : "Save" );
    widget_add( panel, button, BRD_LINE_END );
    widget_dec_ref( button );

    widget_connect_event_handler( button, "clicked", file_chooser_open_pressed, ( void* )chooser );

    return chooser;

 error2:
    free( chooser );

 error1:
    return NULL;
}

char* file_chooser_get_selected_file( file_chooser_t* chooser ) {
    char path[ 256 ];
    char* filename;

    filename = textfield_get_text( chooser->filename_field );

    if ( strcmp( chooser->current_path, "/" ) == 0 ) {
        snprintf( path, sizeof( path ), "/%s", filename );
    } else {
        snprintf( path, sizeof( path ), "%s/%s", chooser->current_path, filename );
    }

    free( filename );

    return strdup( path );
}

int file_chooser_show( file_chooser_t* chooser ) {
    if ( chooser == NULL ) {
        return -EINVAL;
    }

    window_show( chooser->window );

    return 0;
}
