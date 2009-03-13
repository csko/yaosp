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

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_SYNTHESIS_H

#include <yaosp/semaphore.h>

#include <ygui/font.h>
#include <yutil/hashtable.h>

typedef struct font_glyph {
    int dummy;
} font_glyph_t;

typedef struct font_node {
    hashitem_t hash;
    int ascender;
    int descender;
    int line_gap;
    int advance;
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
    semaphore_id lock;
    hashtable_t nodes;
} font_style_t;

typedef struct font_family {
    hashitem_t hash;
    const char* name;
    hashtable_t styles;
} font_family_t;

int init_font_manager( void );

#endif /* _FONTMANAGER_H_ */
