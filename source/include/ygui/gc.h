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

#ifndef _YGUI_GC_H_
#define _YGUI_GC_H_

#include <ygui/color.h>
#include <ygui/rect.h>
#include <ygui/point.h>
#include <ygui/font.h>
#include <ygui/bitmap.h>
#include <yutil/stack.h>

struct window;

typedef struct gc {
    point_t lefttop;
    rect_t clip_rect;
    color_t pen_color;
    int is_clip_valid;
    struct window* window;
    stack_t tr_stack;
    stack_t res_area_stack;

    /* Cache */

    int active_font;
} gc_t;

int gc_get_pen_color( gc_t* gc, color_t* color );

int gc_set_pen_color( gc_t* gc, color_t* color );
int gc_set_font( gc_t* gc, font_t* font );
int gc_set_clip_rect( gc_t* gc, rect_t* rect );
int gc_reset_clip_rect( gc_t* gc );
int gc_translate( gc_t* gc, point_t* point );
int gc_translate_xy( gc_t* gc, int x, int y );
int gc_set_drawing_mode( gc_t* gc, drawing_mode_t mode );
int gc_draw_rect( gc_t* gc, rect_t* rect );
int gc_fill_rect( gc_t* gc, rect_t* rect );
int gc_draw_text( gc_t* gc, point_t* position, const char* text, int length );
int gc_draw_bitmap( gc_t* gc, point_t* position, bitmap_t* bitmap );

int gc_clean_up( gc_t* gc );

int init_gc( struct window* window, gc_t* gc );
int destroy_gc( gc_t* gc );

#endif /* _YGUI_GC_H_ */
