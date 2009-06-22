/* Graphics driver definitions
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

#include <config.h>

#ifdef ENABLE_GUI

#include <macros.h>
#include <errno.h>
#include <console.h>
#include <gui/graphicsdriver.h>
#include <lib/string.h>

#include <arch/gui/graphicsdriver.h>

static hashtable_t driver_table;
static graphics_driver_t* graphics_driver = NULL;

#define BITMAP_OFFSET_32( ptr, x, y, bpl ) ((uint32_t*)(((uint8_t*)(ptr)) + (x*4) + (y) * (bpl)))

#ifndef ARCH_HAVE_FILLRECT

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

    ASSERT( bitmap->width >= width );

    data = ( uint32_t* )bitmap->buffer + ( rect->top * bitmap->width + rect->left );
    padding = bitmap->width - width;

    for ( y = 0; y < height; y++ ) {
        for ( x = 0; x < width; x++ ) {
            *data++ = color;
        }

        data += padding;
    }
}

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

    ASSERT( bitmap->width >= width );

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

int graphics_driver_fill_rect( bitmap_t* bitmap, rect_t* rect, color_t* color, drawing_mode_t mode ) {
    switch ( mode ) {
        case DM_COPY :
            switch ( bitmap->color_space ) {
                case CS_RGB32 :
                    generic_fill_rect_rgb32_copy( bitmap, rect, color_to_uint32( color ) );
                    break;

                default :
                    kprintf( "generic_fill_rect(): Not supported for color space: %d\n", bitmap->color_space );
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

#endif /* ARCH_HAVE_FILLRECT */

#ifndef ARCH_HAVE_BLITBITMAP

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
                    int y;
                    int src_modulo;
                    int data_to_copy;
                    uint32_t* src_buffer;

                    src_buffer = BITMAP_OFFSET_32( src_bitmap->buffer, src_rect->left, src_rect->top, src_bitmap->bytes_per_line );
                    src_modulo = src_bitmap->bytes_per_line / 4;

                    data_to_copy = width * 4;

                    for ( y = 0; y < height; y++ ) {
                        memcpy( dst_buffer, src_buffer, data_to_copy );

                        dst_buffer += dst_modulo;
                        src_buffer += src_modulo;
                    } // for

                    break;
                } // case CS_RGB32

                default :
                    kprintf( "blit_bitmap_copy(): Invalid src color space: %d\n", src_bitmap->color_space );
                    break;
            } // switch

            break;
        } // case CS_RGB32

        default :
            kprintf( "blit_bitmap_copy(): Invalid dest color space: %d\n", dst_bitmap->color_space );
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

int graphics_driver_blit_bitmap( bitmap_t* dst_bitmap, point_t* dst_point, bitmap_t* src_bitmap, rect_t* src_rect, drawing_mode_t mode ) {
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

#endif /* ARCH_HAVE_BLITBITMAP */

int register_graphics_driver( graphics_driver_t* driver ) {
    int error;

    error = hashtable_add( &driver_table, ( hashitem_t* )driver );

    if ( error < 0 ) {
        return error;
    }

    return 0;
}

int select_graphics_driver( void ) {
    graphics_driver = ( graphics_driver_t* )hashtable_get( &driver_table, "VESA" );

    if ( graphics_driver == NULL ) {
        return -ENOENT;
    }

    if ( graphics_driver->fill_rect == NULL ) {
        graphics_driver->fill_rect = graphics_driver_fill_rect;
    }

    if ( graphics_driver->blit_bitmap == NULL ) {
        graphics_driver->blit_bitmap = graphics_driver_blit_bitmap;
    }

    return 0;
}

graphics_driver_t* get_graphics_driver( void ) {
    return graphics_driver;
}

static void* graphics_driver_key( hashitem_t* item ) {
    graphics_driver_t* driver;

    driver = ( graphics_driver_t* )item;

    return ( void* )driver->name;
}

static uint32_t graphics_driver_hash( const void* key ) {
    return hash_string( ( uint8_t* )key, strlen( ( const char* )key ) );
}

static bool graphics_driver_compare( const void* key1, const void* key2 ) {
    return ( strcmp( ( const char* )key1, ( const char* )key2 ) == 0 );
}

__init int init_graphics_driver_manager( void ) {
    int error;

    error = init_hashtable(
        &driver_table,
        32,
        graphics_driver_key,
        graphics_driver_hash,
        graphics_driver_compare
    );

    if ( error < 0 ) {
        return error;
    }

    return 0;
}

#endif /* ENABLE_GUI */
