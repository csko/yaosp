/* Basic screen output handling functions
 *
 * Copyright (c) 2008 Zoltan Kovacs
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

#include <types.h>
#include <console.h>
#include <lib/string.h>

#include <arch/screen.h>
#include <arch/io.h>

static uint16_t* video_memory;

static void screen_move_cursor( console_t* console ) {
    uint32_t tmp = ( console->y * console->width ) + console->x;

    /* Write the new cursor position to the VGA registers */

    outb( 14, 0x3D4 );
    outb( tmp >> 8, 0x3D5 );
    outb( 15, 0x3D4 );
    outb( tmp, 0x3D5 );
}

static void screen_clear( console_t* console ) {
    /* Fill the video memory with spaces */

    memsetw( video_memory, ( 7 << 8 ) | ' ', console->width * console->height );

    /* Set the cursor position to the left-top corner of the screen */

    console->x = 0;
    console->y = 0;

    /* Move the cursor to the specified position */

    screen_move_cursor( console );
}

static void screen_putchar( console_t* console, char c ) {
    uint16_t* p = video_memory;

    /* Get the position in the video memory according
       to the X and Y positions of the cursor */

    p += ( console->y * console->width ) + console->x;

    /* Handle the printed character */

    switch ( c ) {
        case '\r' :
            console->x = 0;

            break;

        case '\n' :
            console->x = 0;
            console->y++;

            break;

        default :
            *p++ = ( 7 << 8 ) | c;
            console->x++; 

            break;
    }

    /* Check if we reached the end of the line */

    if ( console->x == console->width ) {
        console->y++;
        console->x = 0;
    }

    /* Check if we filled the last line of the screen */

    if ( console->y == console->height ) {
        uint16_t* p;

        console->y--;

        /* Scroll the lines up */

        memmove( video_memory, video_memory + console->width, console->width * ( console->height - 1 ) * 2 );

        /* Empty the last line of the screen */

        p = video_memory + console->width * ( console->height - 1 );

        memsetw( p, ( 7 << 8 ) | ' ', console->width );
    }

    /* Move the cursor to the modified position */

    screen_move_cursor( console );
}

static void screen_gotoxy( console_t* console, int x, int y ) {
    /* Do a little sanity checking */

    if ( ( x < 0 ) || ( y < 0 ) ) {
        return;
    }

    if ( ( x >= console->width ) || ( y >= console->height ) ) {
        return;
    }

    /* Set the new cursor positions */

    console->x = x;
    console->y = y;

    /* ... and move the cursor */

    screen_move_cursor( console );
}
 
static console_operations_t screen_ops = {
    .clear = screen_clear,
    .putchar = screen_putchar,
    .gotoxy = screen_gotoxy
};

static console_t screen = {
    .x = 0,
    .y = 0,
    .width = SCREEN_WIDTH,
    .height = SCREEN_HEIGHT,
    .ops = &screen_ops
};

int init_screen( void ) {
    video_memory = ( uint16_t* )0xB8000;

    screen_clear( &screen );
    console_set_screen( &screen );

    return 0;
}
