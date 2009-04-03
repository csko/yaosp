/* GUI server
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

#include <windowdecorator.h>
#include <graphicsdriver.h>

#define BORDER_SIZE 5

static color_t border_colors[] = {
    { 0x4E, 0x7A, 0xA0, 0xFF },
    { 0x83, 0xA7, 0xC6, 0xFF },
    { 0x74, 0x9A, 0xBB, 0xFF },
    { 0x5D, 0x86, 0xA9, 0xFF },
    { 0x5C, 0x5C, 0x5B, 0xFF }
};

static color_t top_border_colors[] = {
    { 0x4E, 0x7A, 0xA0, 0xFF },
    { 0x83, 0xA7, 0xC6, 0xFF },
    { 0x74, 0x9A, 0xBB, 0xFF },
    { 0x5D, 0x86, 0xA9, 0xFF },
    { 0x5D, 0x86, 0xA9, 0xFF },
    { 0x5D, 0x86, 0xA8, 0xFF },
    { 0x5D, 0x85, 0xA8, 0xFF },
    { 0x5C, 0x84, 0xA7, 0xFF },
    { 0x5B, 0x83, 0xA6, 0xFF },
    { 0x5B, 0x83, 0xA5, 0xFF },
    { 0x5A, 0x82, 0xA4, 0xFF },
    { 0x5A, 0x81, 0xA2, 0xFF },
    { 0x5A, 0x80, 0xA1, 0xFF },
    { 0x58, 0x7F, 0xA0, 0xFF },
    { 0x58, 0x7E, 0x9F, 0xFF },
    { 0x57, 0x7E, 0x9E, 0xFF },
    { 0x56, 0x7C, 0x9C, 0xFF },
    { 0x56, 0x7B, 0x9B, 0xFF },
    { 0x56, 0x7B, 0x9A, 0xFF },
    { 0x55, 0x7A, 0x99, 0xFF },
    { 0x54, 0x79, 0x98, 0xFF },
    { 0x54, 0x78, 0x97, 0xFF },
    { 0x54, 0x77, 0x96, 0xFF },
    { 0x54, 0x77, 0x96, 0xFF },
    { 0x53, 0x77, 0x95, 0xFF },
    { 0x5C, 0x5C, 0x5B, 0xFF }
};

static int decorator_update_border( window_t* window ) {
    int i;
    int width;
    int height;
    rect_t rect;
    color_t* color;
    bitmap_t* bitmap;

    bitmap = window->bitmap;

    rect_bounds( &window->screen_rect, &width, &height );

    color = &border_colors[ 0 ];

    for ( i = 0; i < 5; i++, color++ ) {
        /* Left | */

        rect_init( &rect, i, i, i, height - ( i + 1 ) );
        graphics_driver->fill_rect( bitmap, &rect, color );

        /* Right | */

        rect_init( &rect, width - ( i + 1 ), i, width - ( i + 1 ), height - ( i + 1 ) );
        graphics_driver->fill_rect( bitmap, &rect, color );

        /* Bottom - */

        rect_init( &rect, i, height - ( i + 1 ), width - ( i + 1 ), height - ( i + 1 ) );
        graphics_driver->fill_rect( bitmap, &rect, color );
    }

    /* Top - */

    color = &top_border_colors[ 0 ];

    for ( i = 0; i < 4; i++, color++ ) {
        rect_init( &rect, i, i, width - ( i + 1 ), i );
        graphics_driver->fill_rect( bitmap, &rect, color );
    }

    for ( i = 4; i < 26; i++, color++ ) {
        rect_init( &rect, 4, i, width - 5, i );
        graphics_driver->fill_rect( bitmap, &rect, color );
    }

    return 0;
}

window_decorator_t default_decorator = {
    .border_size = {
        .x = 10,
        .y = 31
    },
    .lefttop_offset = {
        .x = 5,
        .y = 26
    },
    .update_border = decorator_update_border
};
