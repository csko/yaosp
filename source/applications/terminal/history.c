/* Terminal application
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

#include <assert.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "history.h"

int terminal_history_add_line( terminal_history_t* history, terminal_line_t* line ) {
    terminal_line_t* new_line;

    assert( line->size <= history->width );

    new_line = ( terminal_line_t* )malloc(
        sizeof( terminal_line_t ) + sizeof( char ) * history->width + sizeof( terminal_attr_t ) * history->width
    );

    if ( new_line == NULL ) {
        return -ENOMEM;
    }

    new_line->buffer = ( char* )( new_line + 1 );
    new_line->attr = ( terminal_attr_t* )( new_line->buffer + history->width );

    memcpy( new_line->buffer, line->buffer, history->width );
    memcpy( new_line->attr, line->attr, sizeof( terminal_attr_t ) * history->width );
    new_line->size = line->size;

    array_add_item( &history->lines, ( void* )new_line );

    return 0;
}

int terminal_history_get_size( terminal_history_t* history ) {
    return array_get_size( &history->lines );
}

terminal_line_t* terminal_history_get_line_at( terminal_history_t* history, int index ) {
    assert( ( index >= 0 ) && ( index < array_get_size( &history->lines ) ) );

    return ( terminal_line_t* )array_get_item( &history->lines, index );
}

int terminal_history_init( terminal_history_t* history, int width ) {
    int error;

    error = init_array( &history->lines );

    if ( error < 0 ) {
        return error;
    }

    array_set_realloc_size( &history->lines, 32 );

    history->width = width;

    return 0;
}
