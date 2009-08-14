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

#define BITMAP_OFFSET_32( ptr, x, y, bpl ) ((uint32_t*)(((uint8_t*)(ptr)) + (x*4) + (y) * (bpl)))

#if defined( USE_i386_ASM )
int i386_fill_rect_rgb32_copy( bitmap_t* bitmap, rect_t* rect, uint32_t color );
int i386_blit_bitmap_copy_rgb32( int width, int height, uint32_t* dst_buffer, int dst_modulo, uint32_t* src_buffer, int src_modulo );
#endif /* USE_i386_ASM */

#if !defined( USE_i386_ASM )
static void generic_fill_rect_rgb32_copy( bitmap_t* bitmap, rect_t* rect, uint32_t color ) {
    int x;
    int y;
    int height;
    int width;
    uint32_t* data;
    uint32_t padding;

    rect_bounds( rect, &width, &height );

    if ( ( width == 0 ) || ( height == 0 ) ) {
        return;
    }

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
#endif /* USE_i386_ASM */

static void generic_fill_rect_rgb32_invert( bitmap_t* bitmap, rect_t* rect ) {
    int x;
    int y;
    int height;
    int width;
    uint32_t* data;
    uint32_t padding;
    color_t color;

    rect_bounds( rect, &width, &height );

    if ( ( width == 0 ) || ( height == 0 ) ) {
        return;
    }

    assert( bitmap->width >= width );

    data = ( uint32_t* )bitmap->buffer + ( rect->top * bitmap->width + rect->left );
    padding = bitmap->width - width;

    for ( y = 0; y < height; y++ ) {
        for ( x = 0; x < width; x++ ) {
            color_from_uint32( &color, *data );
            color_invert( &color );
            *data++ = color_to_uint32( &color );
        }

        data += padding;
    }
}

int generic_fill_rect( bitmap_t* bitmap, rect_t* rect, color_t* color, drawing_mode_t mode ) {
    switch ( mode ) {
        case DM_COPY :
            switch ( bitmap->color_space ) {
                case CS_RGB32 :
#if defined( USE_i386_ASM )
                    i386_fill_rect_rgb32_copy( bitmap, rect, color_to_uint32( color ) );
#else
                    generic_fill_rect_rgb32_copy( bitmap, rect, color_to_uint32( color ) );
#endif
                    break;

                default :
                    dbprintf( "generic_fill_rect(): Not supported for color space: %d\n", bitmap->color_space );
                    break;
            }

            break;

        case DM_BLEND :
            break;

        case DM_INVERT :
            generic_fill_rect_rgb32_invert( bitmap, rect );
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

                      uint8_t bg_alpha = bg_color >> 24;
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

    point_copy( &current_point, point );

    pthread_mutex_lock( &font->style->mutex );

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

    pthread_mutex_unlock( &font->style->mutex );

    return 0;
}

static int blit_bitmap_copy( bitmap_t* dst_bitmap, point_t* dst_point, bitmap_t* src_bitmap, rect_t* src_rect ) {
    int width;
    int height;

    rect_bounds( src_rect, &width, &height );

    if ( ( width == 0 ) || ( height == 0 ) ) {
        return 0;
    }

    switch ( dst_bitmap->color_space ) {
        case CS_RGB32 : {
            int dst_modulo;
            uint32_t* dst_buffer;

            dst_buffer = BITMAP_OFFSET_32( dst_bitmap->buffer, dst_point->x, dst_point->y, dst_bitmap->bytes_per_line );
            dst_modulo = dst_bitmap->bytes_per_line / 4;

            switch ( src_bitmap->color_space ) {
                case CS_RGB32 : {
                    int src_modulo;
                    uint32_t* src_buffer;

                    src_buffer = BITMAP_OFFSET_32( src_bitmap->buffer, src_rect->left, src_rect->top, src_bitmap->bytes_per_line );
                    src_modulo = src_bitmap->bytes_per_line / 4;

#if defined( USE_i386_ASM )
                    i386_blit_bitmap_copy_rgb32(
                        width,
                        height,
                        dst_buffer,
                        dst_modulo,
                        src_buffer,
                        src_modulo
                    );
#else
                    int y;
                    int data_to_copy;

                    data_to_copy = width * 4;

                    for ( y = 0; y < height; y++ ) {
                        memcpy( dst_buffer, src_buffer, data_to_copy );

                        dst_buffer += dst_modulo;
                        src_buffer += src_modulo;
                    } // for
#endif /* USE_i386_ASM */

                    break;
                } // case CS_RGB32

                default :
                    dbprintf( "blit_bitmap_copy(): Invalid src color space: %d\n", src_bitmap->color_space );
                    break;
            } // switch

            break;
        } // case CS_RGB32

        default :
            dbprintf( "blit_bitmap_copy(): Invalid dest color space: %d\n", dst_bitmap->color_space );
            break;
    } // switch

    return 0;
}

static int blit_bitmap_blend( bitmap_t* dst_bitmap, point_t* dst_point, bitmap_t* src_bitmap, rect_t* src_rect ) {
    int src_modulo;
    int dst_modulo;
    uint32_t src_alpha;
    uint32_t src_color;
    uint32_t* src_buffer;
    uint32_t* dst_buffer;

    int x;
    int y;
    int width;
    int height;

    rect_bounds( src_rect, &width, &height );

    if ( ( width == 0 ) || ( height == 0 ) ) {
        return 0;
    }

    src_buffer = BITMAP_OFFSET_32( src_bitmap->buffer, src_rect->left, src_rect->top, src_bitmap->bytes_per_line );
    src_modulo = ( src_bitmap->bytes_per_line - width * 4 ) / 4;

    dst_buffer = BITMAP_OFFSET_32( dst_bitmap->buffer, dst_point->x, dst_point->y, dst_bitmap->bytes_per_line );
    dst_modulo = ( dst_bitmap->bytes_per_line - width * 4 ) / 4;

    for ( y = 0; y < height; y++ ) {
        for ( x = 0; x < width; x++ ) {
            src_color = *src_buffer++;
            src_alpha = src_color >> 24;

            if ( src_alpha == 0xFF ) {
                *dst_buffer = ( *dst_buffer & 0xFF000000 ) | ( src_color & 0x00FFFFFF );
            } else if( src_alpha != 0x00 ) {
                uint32_t src1;
                uint32_t dst1;
                uint32_t dst_alpha;
                uint32_t dst_color;

                dst_color = *dst_buffer;
                dst_alpha = dst_color & 0xFF000000;

                src1 = src_color & 0xFF00FF;
                dst1 = dst_color & 0xFF00FF;
                dst1 = ( dst1 + ( ( src1 - dst1 ) * src_alpha >> 8 ) ) & 0xFF00FF;
                src_color &= 0xFF00;
                dst_color &= 0xFF00;
                dst_color = ( dst_color + ( ( src_color - dst_color ) * src_alpha >> 8 ) ) & 0xFF00;

                *dst_buffer = dst1 | dst_color | dst_alpha;
            }

            dst_buffer++;
        } // for

        dst_buffer += dst_modulo;
        src_buffer += src_modulo;
    } // for

    return 0;
}

int generic_blit_bitmap( bitmap_t* dst_bitmap, point_t* dst_point, bitmap_t* src_bitmap, rect_t* src_rect, drawing_mode_t mode ) {
    int error;

    switch ( mode ) {
        case DM_COPY :
            error = blit_bitmap_copy( dst_bitmap, dst_point, src_bitmap, src_rect );
            break;

        case DM_BLEND :
            if ( ( dst_bitmap->color_space != CS_RGB32 ) ||
                 ( src_bitmap->color_space != CS_RGB32 ) ) {
                error = -EINVAL;
            } else {
                error = blit_bitmap_blend( dst_bitmap, dst_point, src_bitmap, src_rect );
            }

            break;

        default :
            error = -EINVAL;
            break;
    }

    return error;
}
