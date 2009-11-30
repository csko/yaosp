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

#ifndef _YGUI_FONT_H_
#define _YGUI_FONT_H_

#include <sys/types.h>

static inline int utf8_is_first_byte( uint8_t byte ) {
    return ( ( ( byte & 0x80 ) == 0x00 ) ||
             ( ( byte & 0xC0 ) == 0xC0 ) );
}

static inline int utf8_char_length( uint8_t byte ) {
    return ( ( ( 0xE5000000 >> ( ( byte >> 3 ) & 0x1E ) ) & 3 ) + 1 );
}

static inline int utf8_to_unicode( const char* text ) {
    if ( ( text[ 0 ] & 0x80 ) == 0 ) {
        return *text;
    } else if ( ( text[ 1 ] & 0xC0 ) != 0x80 ) {
        return 0xFFFD;
    } else if ( ( text[ 0 ] & 0x20 ) == 0 ) {
        return ( ( ( text[ 0 ] & 0x1F ) << 6 ) | ( text[ 1 ] & 0x3F ) );
    } else if ( ( text[ 2 ] & 0xC0 ) != 0x80 ) {
        return 0xFFFD;
    } else if ( ( text[ 0 ] & 0x10 ) == 0 ) {
        return ( ( ( text[ 0 ] & 0x0F ) << 12 ) | ( ( text[ 1 ] & 0x3F ) << 6 ) | ( text[ 2 ] & 0x3F ) );
    } else if ( ( text[ 3 ] & 0xC0 ) != 0x80 ) {
        return 0xFFFD;
    } else {
        int tmp;
        tmp = ( ( text[ 0 ] & 0x07 ) << 18 ) | ( ( text[ 1 ] & 0x3F ) << 12 ) | ( ( text[ 2 ] & 0x3F ) << 6 ) | ( text[ 3 ] & 0x3F );
        return ( ( ( 0xD7C0 + ( tmp >> 10 ) ) << 16 ) | ( 0xDC00 + ( tmp & 0x3FF ) ) );
    }
}

enum {
    FONT_SMOOTHED = 1
};

typedef struct font_properties {
    int point_size;
    int flags;
} font_properties_t;

typedef struct font {
    int handle;
    int ascender;
    int descender;
    int line_gap;
} font_t;

int font_get_height( font_t* font );
int font_get_ascender( font_t* font );
int font_get_descender( font_t* font );
int font_get_line_gap( font_t* font );

int font_get_string_width( font_t* font, const char* text, int length );

font_t* create_font( const char* family, const char* style, font_properties_t* properties );
int destroy_font( font_t* font );

#endif /* _YGUI_FONT_H_ */
