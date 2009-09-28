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
#include <string.h>
#include <errno.h>
#include <sys/param.h>

#include "buffer.h"

static inline void buffer_do_clear_line( terminal_line_t* line, terminal_attr_t* attr, int x1, int x2 ) {
    char* cbuf;
    terminal_attr_t* abuf;

    cbuf = line->buffer + x1;
    abuf = line->attr + x1;

    for ( ; x1 <= x2; x1++, cbuf++, abuf++ ) {
        *cbuf = ' ';
        terminal_attr_copy( abuf, attr );
    }
}

static int buffer_do_scroll( terminal_buffer_t* buffer, int top, int bottom, int lines ) {
    int i;
    int height;
    terminal_line_t* line;

    /* TOOD */
    terminal_attr_t dummy_attr = {
        .fg_color = T_COLOR_WHITE,
        .bg_color = T_COLOR_BLACK
    };

    assert( ( top >= 0 ) && ( top < buffer->height ) );
    assert( ( bottom >= 0 ) && ( bottom < buffer->height ) );
    assert( top <= bottom );
    assert( lines != 0 );

    height = bottom - top + 1;

    if ( abs( lines ) >= height ) {
        /* clear the lines simply */

        for ( line = buffer->lines + top; top <= bottom; top++, line++ ) {
            buffer_do_clear_line( line, &dummy_attr, 0, buffer->width - 1 );
            line->size = 0;
        }
    } else if ( lines > 0 ) {
        /* scroll up */

        for ( i = 0; i < height - lines; i++ ) {
            terminal_line_t* to = &buffer->lines[ top + i ];
            terminal_line_t* from = &buffer->lines[ top + i + lines ];

            memcpy( to->buffer, from->buffer, buffer->width );
            memcpy( to->attr, from->attr, sizeof( terminal_attr_t ) * buffer->width );
            to->size = from->size;
        }

        for ( i = bottom - lines + 1; i <= bottom; i++ ) {
            line = &buffer->lines[ i ];

            buffer_do_clear_line( line, &dummy_attr, 0, buffer->width - 1 );
            line->size = 0;
        }
    } else {
        /* scroll down */

        assert( lines != 0 );
        lines = -lines;

        for ( i = 0; i < height - lines; i++ ) {
            terminal_line_t* to = &buffer->lines[ bottom - i ];
            terminal_line_t* from = &buffer->lines[ bottom - ( i + lines ) ];

            memcpy( to->buffer, from->buffer, buffer->width );
            memcpy( to->attr, from->attr, sizeof( terminal_attr_t ) * buffer->width );
            to->size = from->size;
        }

        for ( i = 0; i < lines; i++ ) {
            line = &buffer->lines[ top + i ];

            buffer_do_clear_line( line, &dummy_attr, 0, buffer->width - 1 );
            line->size = 0;
        }
    }

    return 0;
}

int terminal_buffer_insert_cr( terminal_buffer_t* buffer ) {
    buffer->cursor_x = 0;

    return 0;
}

int terminal_buffer_insert_lf( terminal_buffer_t* buffer ) {
    if ( buffer->cursor_y == buffer->scroll_bottom ) {
        buffer_do_scroll( buffer, buffer->scroll_top, buffer->scroll_bottom, 1 );
    } else {
        buffer->cursor_y++;
    }

    return 0;
}

int terminal_buffer_insert_backspace( terminal_buffer_t* buffer ) {
    if ( buffer->cursor_x == 0 ) {
        if ( buffer->cursor_y > 0 ) {
            buffer->cursor_y--;
        }
    } else {
        buffer->cursor_x--;
    }

    return 0;
}

int terminal_buffer_insert_space( terminal_buffer_t* buffer, int count ) {
    int i;
    terminal_line_t* line;

    line = buffer->lines + buffer->cursor_y;

    if ( buffer->cursor_x >= ( buffer->width - count ) ) {
        return 0;
    }

    memmove(
        &line->buffer[ buffer->cursor_x + count ],
        &line->buffer[ buffer->cursor_x ],
        buffer->width - ( buffer->cursor_x + count )
    );

    memmove(
        &line->attr[ buffer->cursor_x + count ],
        &line->attr[ buffer->cursor_x ],
        sizeof( terminal_attr_t ) * ( buffer->width - ( buffer->cursor_x + count ) )
    );

    for ( i = 0; i < count; i++ ) {
        line->buffer[ buffer->cursor_x + i ] = ' ';
        /* TODO: attrib! */
    }

    line->size = MIN( line->size + count, buffer->width );

    return 0;
}

int terminal_buffer_insert_char( terminal_buffer_t* buffer, terminal_attr_t* attr, char c ) {
    terminal_line_t* line;

    /* Make sure that the cursor position is valid */

    assert( ( buffer->cursor_x >= 0 ) && ( buffer->cursor_x < buffer->width ) );
    assert( ( buffer->cursor_y >= 0 ) && ( buffer->cursor_y < buffer->height ) );

    /* Select the current line */

    line = buffer->lines + buffer->cursor_y;

    /* Put the new character and attribute into the buffer */

    line->buffer[ buffer->cursor_x ] = c;
    terminal_attr_copy( &line->attr[ buffer->cursor_x ], attr );

    /* Save the last dirty character in the line */

    line->size = MAX( line->size, buffer->cursor_x + 1 );

    /* Move the cursor */

    buffer->cursor_x++;

    if ( buffer->cursor_x == buffer->width ) {
        if ( buffer->cursor_y == buffer->scroll_bottom ) {
            buffer_do_scroll( buffer, buffer->scroll_top, buffer->scroll_bottom, 1 );
        } else {
            buffer->cursor_y++;
        }

        buffer->cursor_x = 0;
    }

    return 0;
}

int terminal_buffer_erase_above( terminal_buffer_t* buffer ) {
    int i;
    terminal_line_t* line;

    /* TOOD */
    terminal_attr_t dummy_attr = {
        .fg_color = T_COLOR_WHITE,
        .bg_color = T_COLOR_BLACK
    };

    for ( i = 0, line = buffer->lines; i <= buffer->cursor_y; i++, line++ ) {
        buffer_do_clear_line( line, &dummy_attr, 0, buffer->width - 1 );
        line->size = 0;
    }

    return 0;
}

int terminal_buffer_erase_below( terminal_buffer_t* buffer ) {
    int i;
    terminal_line_t* line;

    /* TOOD */
    terminal_attr_t dummy_attr = {
        .fg_color = T_COLOR_WHITE,
        .bg_color = T_COLOR_BLACK
    };

    for ( i = buffer->cursor_y, line = buffer->lines + buffer->cursor_y; i < buffer->height; i++, line++ ) {
        buffer_do_clear_line( line, &dummy_attr, 0, buffer->width - 1 );
        line->size = 0;
    }

    return 0;
}

int terminal_buffer_erase_before( terminal_buffer_t* buffer ) {
    /* TOOD */
    terminal_attr_t dummy_attr = {
        .fg_color = T_COLOR_WHITE,
        .bg_color = T_COLOR_BLACK
    };

    buffer_do_clear_line(
        buffer->lines + buffer->cursor_y,
        &dummy_attr,
        0,
        buffer->cursor_x
    );

    return 0;
}

int terminal_buffer_erase_after( terminal_buffer_t* buffer ) {
    /* TOOD */
    terminal_attr_t dummy_attr = {
        .fg_color = T_COLOR_WHITE,
        .bg_color = T_COLOR_BLACK
    };

    buffer_do_clear_line(
        buffer->lines + buffer->cursor_y,
        &dummy_attr,
        buffer->cursor_x,
        buffer->width - 1
    );

    return 0;
}

int terminal_buffer_delete( terminal_buffer_t* buffer, int count ) {
    int remaining;
    terminal_line_t* line;

    line = buffer->lines + buffer->cursor_y;

    remaining = buffer->width - ( buffer->cursor_x + 1 );

    if ( remaining > 0 ) {
        memmove(
            &line->buffer[ buffer->cursor_x ],
            &line->buffer[ buffer->cursor_x + count ],
            remaining
        );

        memmove(
            &line->attr[ buffer->cursor_x ],
            &line->attr[ buffer->cursor_x + count ],
            sizeof( terminal_attr_t ) * remaining
        );
    }

    line->size = buffer->cursor_x + remaining;

    return 0;
}

int terminal_buffer_save_cursor( terminal_buffer_t* buffer ) {
    assert( ( buffer->saved_cursor_x == -1 ) && ( buffer->saved_cursor_y == -1 ) );

    buffer->saved_cursor_x = buffer->cursor_x;
    buffer->saved_cursor_y = buffer->cursor_y;

    return 0;
}

int terminal_buffer_restore_cursor( terminal_buffer_t* buffer ) {
    assert( ( buffer->saved_cursor_x != -1 ) && ( buffer->saved_cursor_y != -1 ) );

    buffer->cursor_x = buffer->saved_cursor_x;
    buffer->cursor_y = buffer->saved_cursor_y;

    buffer->saved_cursor_x = -1;
    buffer->saved_cursor_y = -1;

    return 0;
}

int terminal_buffer_move_cursor( terminal_buffer_t* buffer, int dx, int dy ) {
    buffer->cursor_x += dx;
    buffer->cursor_y += dy;

    if ( buffer->cursor_x < 0 ) { buffer->cursor_x = 0; }
    else if ( buffer->cursor_x >= buffer->width ) { buffer->cursor_x = buffer->width - 1; }

    if ( buffer->cursor_y < 0 ) { buffer->cursor_y = 0; }
    else if ( buffer->cursor_y >= buffer->height ) { buffer->cursor_y = buffer->height - 1; }

    return 0;
}

int terminal_buffer_move_cursor_to( terminal_buffer_t* buffer, int x, int y ) {
    if ( x != -1 ) {
        assert( ( x >= 0 ) && ( x < buffer->width ) );
        buffer->cursor_x = x;
    }

    if ( y != -1 ) {
        assert( ( y >= 0 ) && ( y < buffer->height ) );
        buffer->cursor_y = y;
    }

    return 0;
}

int terminal_buffer_scroll_by( terminal_buffer_t* buffer, int lines ) {
    buffer_do_scroll( buffer, buffer->scroll_top, buffer->scroll_bottom, lines );

    return 0;
}

int terminal_buffer_set_scroll_region( terminal_buffer_t* buffer, int top, int bottom ) {
    assert( ( bottom >= 0 ) && ( bottom < buffer->height ) );
    assert( ( top >= 0 ) && ( top <= bottom ) );

    buffer->scroll_top = top;
    buffer->scroll_bottom = bottom;

    return 0;
}

int terminal_buffer_init( terminal_buffer_t* buffer, int width, int height ) {
    int i, j;
    terminal_line_t* line;

    buffer->width = width;
    buffer->height = height;
    buffer->cursor_x = 0;
    buffer->cursor_y = 0;
    buffer->saved_cursor_x = -1;
    buffer->saved_cursor_y = -1;
    buffer->scroll_top = 0;
    buffer->scroll_bottom = height - 1;

    buffer->lines = ( terminal_line_t* )malloc( sizeof( terminal_line_t ) * height );

    if ( buffer->lines == NULL ) {
        return -ENOMEM;
    }

    line = buffer->lines;

    for ( i = 0; i < height; i++, line++ ) {
        line->buffer = ( char* )malloc( width );

        if ( line->buffer == NULL ) {
            return -ENOMEM;
        }

        line->attr = ( terminal_attr_t* )malloc( sizeof( terminal_attr_t ) * width );

        if ( line->attr == NULL ) {
            return -ENOMEM;
        }

        line->size = 0;

        for ( j = 0; j < width; j++ ) {
            line->buffer[ j ] = ' ';

            line->attr[ j ].bg_color = T_COLOR_BLACK;
            line->attr[ j ].fg_color = T_COLOR_WHITE;
        }
    }

    return 0;
}
