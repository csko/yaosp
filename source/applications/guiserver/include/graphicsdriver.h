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

#ifndef _GRAPHICSDRIVER_H_
#define _GRAPHICSDRIVER_H_

#include <sys/types.h>
#include <yaosp/region.h>

#include <ygui/rect.h>
#include <ygui/color.h>
#include <ygui/point.h>
#include <ygui/yconstants.h>

#include <bitmap.h>
#include <fontmanager.h>

typedef struct screen_mode {
    uint32_t width;
    uint32_t height;
    color_space_t color_space;
    void* private;
} screen_mode_t;

typedef struct graphics_driver {
    const char* name;
    int ( *detect )( void );
    uint32_t ( *get_screen_mode_count )( void );
    int ( *get_screen_mode_info )( uint32_t index, screen_mode_t* screen_mode );
    int ( *set_screen_mode )( screen_mode_t* screen_mode );
    int ( *get_framebuffer_info )( void** address );
    int ( *fill_rect )( bitmap_t* bitmap, rect_t* rect, color_t* color, drawing_mode_t mode );
    int ( *draw_text )( bitmap_t* bitmap, point_t* point, rect_t* clip_rect, font_node_t* font, color_t* color, const char* text, int length );
    int ( *blit_bitmap )( bitmap_t* dst_bitmap, point_t* dst_point, bitmap_t* src_bitmap, rect_t* src_rect, drawing_mode_t mode );
} graphics_driver_t;

extern rect_t screen_rect;
extern bitmap_t* screen_bitmap;
extern screen_mode_t active_screen_mode;
extern graphics_driver_t* graphics_driver;

int generic_fill_rect( bitmap_t* bitmap, rect_t* rect, color_t* color, drawing_mode_t mode );
int generic_draw_text( bitmap_t* bitmap, point_t* point, rect_t* clip_rect, font_node_t* font, color_t* color, const char* text, int length );
int generic_blit_bitmap( bitmap_t* dst_bitmap, point_t* dst_point, bitmap_t* src_bitmap, rect_t* src_rect, drawing_mode_t mode );

#endif /* _GRAPHICSDRIVER_H_ */
