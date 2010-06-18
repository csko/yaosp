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

#ifndef _YGUI_DIRVIEW_H_
#define _YGUI_DIRVIEW_H_

#include <ygui/widget.h>

typedef enum directory_item_type {
    T_NONE,
    T_DIRECTORY,
    T_FILE
} directory_item_type_t;

widget_t* create_directory_view( const char* path );

char* directory_view_get_selected_item_name( widget_t* widget );
int directory_view_get_selected_item_type_and_name( widget_t* widget, directory_item_type_t* type, char** name );

int directory_view_set_path( widget_t* widget, const char* path );

#endif /* _YGUI_DIRVIEW_H_ */
