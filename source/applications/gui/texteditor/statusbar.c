/* Text editor application
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

#include <stdio.h>
#include <stdlib.h>

#include <ygui/window.h>
#include <ygui/label.h>

extern window_t* window;
extern widget_t* statusbar;

static int statusbar_updater( void* data ) {
    label_set_text( statusbar, ( char* )data );
    free( data );

    return 0;
}

int statusbar_set_text( const char* format, ... ) {
    va_list ap;
    char text[ 512 ];

    va_start( ap, format );
    vsnprintf( text, sizeof( text ), format, ap );
    va_end( ap );

    window_insert_callback( window, statusbar_updater, ( void* )strdup( text ) );

    return 0;
}
