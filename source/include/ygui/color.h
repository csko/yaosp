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

#ifndef _YGUI_COLOR_H_
#define _YGUI_COLOR_H_

#include <string.h>
#include <sys/types.h>

typedef struct color {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
    uint8_t alpha;
} color_t;

static inline void color_copy( color_t* new, color_t* old ) {
    memcpy( ( void* )new, ( void* )old, sizeof( color_t ) );
}

static inline uint32_t color_to_uint32( color_t* color ) {
    return ( ( color->alpha << 24 ) | ( color->red << 16 ) | ( color->green << 8 ) | ( color->blue ) );
}

static inline void color_from_uint32( color_t* color, uint32_t data ) {
    color->red = ( data >> 16 ) & 0xFF;
    color->green = ( data >> 8 ) & 0xFF;
    color->blue = data & 0xFF;
    color->alpha = ( data >> 24 ) & 0xFF;
}

static inline void color_invert( color_t* color ) {
    color->red = 255 - color->red;
    color->green = 255 - color->green;
    color->blue = 255 - color->blue;
}

static inline void color_init( color_t* color, uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha ) {
    color->red = red;
    color->green = green;
    color->blue = blue;
    color->alpha = alpha;
}

#endif /* _YGUI_COLOR_H_ */
