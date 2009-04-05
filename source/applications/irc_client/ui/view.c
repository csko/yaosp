/* IRC client
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

#include <string.h>
#include <errno.h>

#include "ui.h"
#include "view.h"

extern int screen_w;
extern view_t* active_view;

static int view_add_line( view_t* view, const char* _line, size_t length ) {
    char* line;

    if ( length == 0 ) {
        return 0;
    }

    line = strndup( _line, length );

    if ( line == NULL ) {
        return -ENOMEM;
    }

    array_add_item( &view->lines, ( void* )line );

    return 0;
}

int view_add_text( view_t* view, const char* text ) {
    char* end;
    size_t length;

    end = strchr( text, '\n' );

    while ( end != NULL ) {
        length = ( size_t )( end - text );

        if ( text[ length - 1 ] == '\r' ) {
            length--;
        }

        while ( length > screen_w ) {
            view_add_line( view, text, screen_w );

            length -= screen_w;
            text += screen_w;
        }

        view_add_line( view, text, length );

        text = end + 1;
        end = strchr( text, '\n' );
    }

    length = strlen( text );

    if ( length > 0 ) {
        if ( text[ length - 1 ] == '\r' ) {
            length--;
        }

        while ( length > screen_w ) {
            view_add_line( view, text, screen_w );

            length -= screen_w;
            text += screen_w;
        }

        view_add_line( view, text, length );
    }

    if ( active_view == view ) {
        ui_draw_view( view );
    }

    return 0;
}

int active_view_add_text( const char* text ) {
    return view_add_text( active_view, text );
}

int init_view( view_t* view, view_operations_t* operations, void* data ) {
    int error;

    error = init_array( &view->lines );

    if ( error < 0 ) {
        return error;
    }

    array_set_realloc_size( &view->lines, 32 );

    view->operations = operations;
    view->data = data;

    return 0;
}
