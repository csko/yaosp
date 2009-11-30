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

#include <ygui/panel.h>
#include <ygui/button.h>
#include <ygui/textfield.h>
#include <ygui/dirview.h>
#include <ygui/label.h>
#include <ygui/dialog/filechooser.h>
#include <ygui/layout/borderlayout.h>

static int file_chooser_window_closed( window_t* window, void* data ) {
    return 0;
}

static int file_chooser_open_pressed( widget_t* widget, void* data ) {
    file_chooser_t* chooser;

    chooser = ( file_chooser_t* )data;
    window_close( chooser->window );

    return 0;
}

file_chooser_t* create_file_chooser( chooser_type_t type ) {
    file_chooser_t* chooser;

    chooser = ( file_chooser_t* )malloc( sizeof( file_chooser_t ) );

    if ( chooser == NULL ) {
        goto error1;
    }

    point_t position = { 50, 50 };
    point_t size = { 250, 150 };

    chooser->window = create_window( type == T_OPEN_DIALOG ? "Open file" : "Save file", &position, &size, WINDOW_NONE );

    if ( chooser->window == NULL ) {
        goto error2;
    }

    window_set_event_handler( chooser->window, WE_CLOSED, file_chooser_window_closed, ( void* )chooser );

    widget_t* container = window_get_container( chooser->window );

    layout_t* layout = create_border_layout();
    panel_set_layout( container, layout );
    layout_dec_ref( layout );

    chooser->path = create_label( "/" );
    label_set_horizontal_alignment( chooser->path, H_ALIGN_LEFT );
    widget_add( container, chooser->path, BRD_PAGE_START );
    widget_dec_ref( chooser->path );

    chooser->dirview = create_directory_view( "/" );
    widget_add( container, chooser->dirview, BRD_CENTER );
    widget_dec_ref( chooser->dirview );

    widget_t* panel = create_panel();
    widget_add( container, panel, BRD_PAGE_END );
    widget_dec_ref( panel );

    layout = create_border_layout();
    panel_set_layout( panel, layout );
    layout_dec_ref( layout );

    chooser->filename = create_textfield();
    widget_add( panel, chooser->filename, BRD_CENTER );
    widget_dec_ref( chooser->filename );

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

int file_chooser_show( file_chooser_t* chooser ) {
    if ( chooser == NULL ) {
        return -EINVAL;
    }

    window_show( chooser->window );

    return 0;
}
