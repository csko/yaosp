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

#ifndef _YGUI_INTERNAL_H_
#define _YGUI_INTERNAL_H_

#include <ygui/window.h>
#include <ygui/gc.h>
#include <ygui/menu.h>

typedef struct widget_wrapper {
    widget_t* widget;
    void* data;
} widget_wrapper_t;

typedef enum {
    M_PARENT_NONE,
    M_PARENT_BAR,
    M_PARENT_WINDOW
} menu_parent_t;

typedef struct menu_item {
    char* text;
    font_t* font;
    bitmap_t* image;
    int active;

    menu_t* submenu;

    menu_t* parent_menu;
    menu_parent_t parent_type;
} menu_item_t;

int initialize_render_buffer( window_t* window );
int allocate_render_packet( window_t* window, size_t size, void** buffer );
int flush_render_buffer( window_t* window );

int gc_push_restricted_area( gc_t* gc, rect_t* area );
int gc_pop_restricted_area( gc_t* gc );
rect_t* gc_current_restricted_area( gc_t* gc );

int gc_push_translate_checkpoint( gc_t* gc );
int gc_rollback_translate( gc_t* gc );

#endif /* _YGUI_INTERNAL_H_ */
