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

#ifndef _YGUI_MENUITEM_H_
#define _YGUI_MENUITEM_H_

#include <ygui/widget.h>
#include <ygui/bitmap.h>
#include <ygui/menu.h>

widget_t* create_menuitem_with_label( const char* text );
widget_t* create_menuitem_with_label_and_image( const char* text, bitmap_t* image );

widget_t* create_separator_menuitem( void );

int menuitem_has_image( widget_t* widget );

int menuitem_set_submenu( widget_t* widget, menu_t* menu );

#endif /* _YGUI_MENUITEM_H_ */
