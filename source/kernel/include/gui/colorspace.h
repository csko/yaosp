/* Color space definitions
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

#ifndef _COLORSPACE_H_
#define _COLORSPACE_H_

typedef enum color_space {
    CS_UNKNOWN,
    CS_RGB16,
    CS_RGB24,
    CS_RGB32
} color_space_t;

static inline int colorspace_to_bpp( color_space_t color_space ) {
    switch ( color_space ) {
        case CS_RGB16 : return 2;
        case CS_RGB24 : return 3;
        case CS_RGB32 : return 4;
        default :
        case CS_UNKNOWN :
            return 0;
    }
}

static inline color_space_t bpp_to_colorspace( int bits_per_pixel ) {
    switch ( bits_per_pixel ) {
        case 16 : return CS_RGB16;
        case 24 : return CS_RGB24;
        case 32 : return CS_RGB32;
        default : return CS_UNKNOWN;
    }
}

#endif /* _COLORSPACE_H_ */
