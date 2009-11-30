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

#ifndef _YGUI_MENU_H_
#define _YGUI_MENU_H_

#include <ygui/window.h>
#include <yutil/array.h>

typedef struct menu {
    window_t* window;
    array_t items;
    widget_t* parent;
} menu_t;

menu_t* create_menu( void );

int menu_add_item( menu_t* menu, widget_t* item );

int menu_get_size( menu_t* menu, point_t* size );

int menu_popup_at( menu_t* menu, point_t* position );
int menu_popup_at_xy( menu_t* menu, int x, int y );

#endif /* _YGUI_MENU_H_ */
