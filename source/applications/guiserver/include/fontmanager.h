/* Font loader and render engine
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

#ifndef _FONTMANAGER_H_
#define _FONTMANAGER_H_

#include <pthread.h>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_SYNTHESIS_H

#include <ygui/font.h>
#include <ygui/rect.h>
#include <ygui/point.h>
#include <yutil/hashtable.h>

typedef struct font_glyph {
    uint8_t* raster;
    rect_t bounds;
    point_t advance;
    int bytes_per_line;
} font_glyph_t;

struct font_style;

typedef struct font_node {
    hashitem_t hash;
    int ascender;
    int descender;
    int line_gap;
    int advance;
    struct font_style* style;
    font_properties_t properties;
    font_glyph_t** glyph_table;
} font_node_t;

typedef struct font_style {
    hashitem_t hash;
    const char* name;
    FT_Face face;
    int glyph_count;
    int scalable;
    int fixed_width;
    pthread_mutex_t mutex;
    hashtable_t nodes;
} font_style_t;

typedef struct font_family {
    hashitem_t hash;
    const char* name;
    hashtable_t styles;
} font_family_t;

font_glyph_t* font_node_get_glyph( font_node_t* node, unsigned int character );
int font_node_get_string_width( font_node_t* font, const char* text, int length );
int font_node_get_height( font_node_t* font );

font_node_t* font_manager_get( const char* family_name, const char* style_name, font_properties_t* properties );

int font_manager_load_fonts( void );

int init_font_manager( void );

#endif /* _FONTMANAGER_H_ */
