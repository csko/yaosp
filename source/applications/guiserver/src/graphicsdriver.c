/* Graphics driver implementation
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
#include <errno.h>
#include <yaosp/debug.h>

#include <graphicsdriver.h>

static void fill_rect_rgb32( bitmap_t* bitmap, rect_t* rect, uint32_t color ) {
    int x;
    int y;
    int height;
    int width;
    uint32_t* data;
    uint32_t padding;

    rect_bounds( rect, &width, &height );

    assert( bitmap->width >= width );

    data = ( uint32_t* )bitmap->buffer + ( rect->top * bitmap->width + rect->left );
    padding = bitmap->width - width;

    for ( y = 0; y < height; y++ ) {
        for ( x = 0; x < width; x++ ) {
            *data++ = color;
        }

        data += padding;
    }
}

int generic_fill_rect( bitmap_t* bitmap, rect_t* rect, color_t* color ) {
    switch ( bitmap->color_space ) {
        case CS_RGB32 :
            fill_rect_rgb32( bitmap, rect, color_to_uint32( color ) );
            break;

        default :
            dbprintf( "generic_fill_rect(): Not supported for color space: %d\n", bitmap->color_space );
            break;
    }

    return 0;
}

static inline void render_glyph( bitmap_t* bitmap, font_glyph_t* glyph, point_t* position, rect_t* clip_rect, color_t* color ) {
    int src_x;
    int src_y;
    int width;
    int height;

    int glyph_modulo;
    uint8_t* glyph_raster;

    rect_t glyph_bounds;
    rect_t visible_glyph_rect;

    rect_add_point_n( &glyph_bounds, &glyph->bounds, position );
    rect_and_n( &visible_glyph_rect, &glyph_bounds, clip_rect );

    if ( !rect_is_valid( &visible_glyph_rect ) ) {
        return;
    }

    src_x = visible_glyph_rect.left - glyph_bounds.left;
    src_y = visible_glyph_rect.top - glyph_bounds.top;

    rect_bounds( &visible_glyph_rect, &width, &height );
    glyph_modulo = glyph->bytes_per_line - width;
    glyph_raster = glyph->raster + src_y * glyph->bytes_per_line + src_x;

    switch ( bitmap->color_space ) {
        case CS_RGB32 : {
            int x;
            int y;
            uint8_t alpha;
            uint32_t dest_color;
            int bitmap_modulo;
            uint32_t* bitmap_raster;

            dest_color = color_to_uint32( color );

            bitmap_modulo = bitmap->bytes_per_line / 4 - width;
            bitmap_raster = ( uint32_t* )bitmap->buffer +
                visible_glyph_rect.top * bitmap->bytes_per_line / 4 +
                visible_glyph_rect.left;

            for ( y = 0; y < height; ++y ) {
                for ( x = 0; x < width; ++x ) {
                    alpha = *glyph_raster++;

                    if ( alpha == 0xFF ) {
                      *bitmap_raster = dest_color;
                    } else if ( alpha > 0 ) {
                      register uint32_t bg_color = *bitmap_raster;

                      uint8_t bg_alpha = ( bg_color >> 24 ) & 0xFF;
                      uint8_t bg_red  = ( bg_color >> 16 ) & 0xFF;
                      uint8_t bg_green = ( bg_color >> 8 ) & 0xFF;
                      uint8_t bg_blue = bg_color & 0xFF;

                      uint8_t tmp_red = bg_red + ( color->red - bg_red ) * alpha / 255;
                      uint8_t tmp_green = bg_green + ( color->green - bg_green ) * alpha / 255;
                      uint8_t tmp_blue = bg_blue + ( color->blue - bg_blue ) * alpha / 255;

                      *bitmap_raster = ( bg_alpha << 24 ) | ( tmp_red << 16 ) | ( tmp_green << 8 ) | tmp_blue;
                  }

                  bitmap_raster++;
                } // for x

                glyph_raster += glyph_modulo;
                bitmap_raster += bitmap_modulo;
            } // for y

            break;
        }

        default :
            dbprintf( "render_glyph(): Unknown color space: %d\n", bitmap->color_space );
            break;
    }
}

int generic_draw_text( bitmap_t* bitmap, point_t* point, rect_t* clip_rect, font_node_t* font, color_t* color, const char* text, int length ) {
    font_glyph_t* glyph;
    point_t current_point;

    if ( ( font == NULL ) || ( text == NULL ) ) {
        return -EINVAL;
    }

    if ( length == -1 ) {
        length = strlen( text );
    }

    memcpy( &current_point, point, sizeof( point_t ) );

    LOCK( font->style->lock );

    while ( length > 0 ) {
        int char_length = utf8_char_length( *text );

        if ( char_length > length ) {
            break;
        }

        glyph = font_node_get_glyph( font, utf8_to_unicode( text ) );

        text += char_length;
        length -= char_length;

        if ( glyph == NULL ) {
            continue;
        }

        render_glyph( bitmap, glyph, &current_point, clip_rect, color );

        current_point.x += glyph->advance.x;
    }

    UNLOCK( font->style->lock );

    return 0;
}
