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

    assert( ( top >= 0 ) && ( top < buffer->height ) );
    assert( ( bottom >= 0 ) && ( bottom < buffer->height ) );
    assert( top <= bottom );
    assert( lines != 0 );

    height = bottom - top + 1;

    /* Save the scrolled lines to the history */

    if ( ( top == 0 ) &&
         ( lines > 0 ) ) {
        int count;

        count = MIN( lines, height );

        for ( i = 0, line = buffer->lines; i < count; i++, line++ ) {
            terminal_history_add_line( &buffer->history, line );
        }
    }

    /* Scroll the region */

    if ( abs( lines ) >= height ) {
        /* clear the lines simply */

        for ( line = buffer->lines + top; top <= bottom; top++, line++ ) {
            buffer_do_clear_line( line, &buffer->attr, 0, buffer->width - 1 );
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

            buffer_do_clear_line( line, &buffer->attr, 0, buffer->width - 1 );
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

            buffer_do_clear_line( line, &buffer->attr, 0, buffer->width - 1 );
            line->size = 0;
        }
    }

    return 0;
}

int terminal_buffer_set_bg( terminal_buffer_t* buffer, terminal_color_t bg ) {
    assert( ( bg >= 0 ) && ( bg < T_COLOR_COUNT ) );

    buffer->attr.bg_color = bg;

    return 0;
}

int terminal_buffer_set_fg( terminal_buffer_t* buffer, terminal_color_t fg ) {
    assert( ( fg >= 0 ) && ( fg < T_COLOR_COUNT ) );

    buffer->attr.fg_color = fg;

    return 0;
}

int terminal_buffer_swap_bgfg( terminal_buffer_t* buffer ) {
    terminal_color_t tmp;

    tmp = buffer->attr.bg_color;
    buffer->attr.bg_color = buffer->attr.fg_color;
    buffer->attr.fg_color = tmp;

    return 0;
}

int terminal_buffer_save_attr( terminal_buffer_t* buffer ) {
    terminal_attr_copy( &buffer->saved_attr, &buffer->attr );

    return 0;
}

int terminal_buffer_restore_attr( terminal_buffer_t* buffer ) {
    terminal_attr_copy( &buffer->attr, &buffer->saved_attr );

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
        terminal_attr_copy( &line->attr[ buffer->cursor_x + i ], &buffer->attr );
    }

    line->size = MIN( line->size + count, buffer->width );

    return 0;
}

int terminal_buffer_insert_char( terminal_buffer_t* buffer, char c ) {
    terminal_line_t* line;

    /* Make sure that the cursor position is valid */

    assert( ( buffer->cursor_x >= 0 ) && ( buffer->cursor_x < buffer->width ) );
    assert( ( buffer->cursor_y >= 0 ) && ( buffer->cursor_y < buffer->height ) );

    /* Select the current line */

    line = buffer->lines + buffer->cursor_y;

    /* Put the new character and attribute into the buffer */

    line->buffer[ buffer->cursor_x ] = c;
    terminal_attr_copy( &line->attr[ buffer->cursor_x ], &buffer->attr );

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

    for ( i = 0, line = buffer->lines; i <= buffer->cursor_y; i++, line++ ) {
        buffer_do_clear_line( line, &buffer->attr, 0, buffer->width - 1 );
        line->size = 0;
    }

    return 0;
}

int terminal_buffer_erase_below( terminal_buffer_t* buffer ) {
    int i;
    terminal_line_t* line;

    for ( i = buffer->cursor_y, line = buffer->lines + buffer->cursor_y; i < buffer->height; i++, line++ ) {
        buffer_do_clear_line( line, &buffer->attr, 0, buffer->width - 1 );
        line->size = 0;
    }

    return 0;
}

int terminal_buffer_erase_before( terminal_buffer_t* buffer ) {
    buffer_do_clear_line(
        buffer->lines + buffer->cursor_y,
        &buffer->attr,
        0,
        buffer->cursor_x
    );

    return 0;
}

int terminal_buffer_erase_after( terminal_buffer_t* buffer ) {
    buffer_do_clear_line(
        buffer->lines + buffer->cursor_y,
        &buffer->attr,
        buffer->cursor_x,
        buffer->width - 1
    );

    return 0;
}

int terminal_buffer_erase( terminal_buffer_t* buffer, int count ) {
    int i;
    char* buf;
    terminal_attr_t* attr;
    terminal_line_t* line;

    line = buffer->lines + buffer->cursor_y;

    buf = line->buffer + buffer->cursor_x;
    attr = line->attr + buffer->cursor_x;

    for ( i = buffer->cursor_x; i < MIN( buffer->cursor_x + count, buffer->width ); i++, buf++, attr++ ) {
        *buf = ' ';
        terminal_attr_copy( attr, &buffer->attr );
    }

    return 0;
}

int terminal_buffer_delete( terminal_buffer_t* buffer, int count ) {
    int remaining;
    terminal_line_t* line;

    line = buffer->lines + buffer->cursor_y;

    remaining = buffer->width - ( buffer->cursor_x + count );

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

    if ( buffer->cursor_x < 0 ) { buffer->cursor_x = 0; }
    else if ( buffer->cursor_x >= buffer->width ) { buffer->cursor_x = buffer->width - 1; }

    buffer->cursor_y += dy;

#if 0
    if ( buffer->cursor_y < buffer->scroll_top ) {
        int lines = buffer->scroll_top - buffer->cursor_y;

        buffer_do_scroll(
            buffer,
            buffer->scroll_top,
            buffer->scroll_bottom,
            lines
        );

        buffer->cursor_y += lines;
        assert( buffer->cursor_y == buffer->scroll_top );
    } else if ( buffer->cursor_y > buffer->scroll_bottom ) {
        int lines = buffer->scroll_bottom - buffer->cursor_y;

        buffer_do_scroll(
            buffer,
            buffer->scroll_top,
            buffer->scroll_bottom,
            lines
        );

        buffer->cursor_y += lines;
        assert( buffer->cursor_y == buffer->scroll_bottom );
    }
#endif

    assert( ( buffer->cursor_y >= 0 ) && ( buffer->cursor_y < buffer->height ) );

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

terminal_line_t* terminal_buffer_get_line_at( terminal_buffer_t* buffer, int index ) {
    int line_count;
    int history_size;

    line_count = terminal_buffer_get_line_count( buffer );

    assert( ( index >= 0 ) && ( index < line_count ) );

    history_size = terminal_history_get_size( &buffer->history );

    if ( index < history_size ) {
        return terminal_history_get_line_at( &buffer->history, index );
    } else {
        return buffer->lines + ( index - history_size );
    }
}

int terminal_buffer_get_line_count( terminal_buffer_t* buffer ) {
    return ( buffer->height + terminal_history_get_size( &buffer->history ) );
}

int terminal_buffer_get_history_size( terminal_buffer_t* buffer ) {
    return terminal_history_get_size( &buffer->history );
}

int terminal_buffer_dump( terminal_buffer_t* buffer ) {
    int i, j;

    dbprintf( "Terminal buffer info:\n" );
    dbprintf( "  size: w=%d, h=%d\n", buffer->width, buffer->height );
    dbprintf( "  cursor: x=%d, y=%d\n", buffer->cursor_x, buffer->cursor_y );
    dbprintf( "  scrolled region: top=%d, bottom=%d\n", buffer->scroll_top, buffer->scroll_bottom );

    dbprintf( "Attribute map:\n" );

    for ( i = 0; i < buffer->height; i++ ) {
        terminal_attr_t* attr = buffer->lines[ i ].attr;

        dbprintf( " " );

        for ( j = 0; j < buffer->width; j++, attr++ ) {
            dbprintf( " %d:%d", attr->bg_color, attr->fg_color );
        }

        dbprintf( "\n" );
    }

    dbprintf( "Character map:\n" );

    for ( i = 0; i < buffer->height; i++ ) {
        char* buf = buffer->lines[ i ].buffer;

        dbprintf( "  '" );

        for ( j = 0; j < buffer->width; j++, buf++ ) {
            dbprintf( "%c", *buf );
        }

        dbprintf( "' size=%d\n", buffer->lines[i].size );
    }

    return 0;
}

int terminal_buffer_init( terminal_buffer_t* buffer, int width, int height ) {
    int i, j;
    int error;
    terminal_line_t* line;

    error = terminal_history_init( &buffer->history, width );

    if ( error < 0 ) {
        goto error1;
    }

    buffer->width = width;
    buffer->height = height;
    buffer->cursor_x = 0;
    buffer->cursor_y = 0;
    buffer->saved_cursor_x = -1;
    buffer->saved_cursor_y = -1;
    buffer->scroll_top = 0;
    buffer->scroll_bottom = height - 1;

    buffer->attr.bg_color = T_COLOR_BLACK;
    buffer->attr.fg_color = T_COLOR_WHITE;

    buffer->lines = ( terminal_line_t* )malloc( sizeof( terminal_line_t ) * height );

    if ( buffer->lines == NULL ) {
        error = -ENOMEM;
        goto error2;
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

 error2:
    /* TODO: destroy the history */

 error1:
    return error;
}
