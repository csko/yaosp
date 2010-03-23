/* Image viewer application
 *
 * Copyright (c) 2010 Zoltan Kovacs
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

#include <ygui/window.h>
#include <ygui/label.h>
#include <ygui/panel.h>
#include <ygui/desktop.h>
#include <ygui/layout/borderlayout.h>

#include "ui-about.h"

int ui_about_open( void ) {
    point_t desk_size;
    layout_t* layout;
    widget_t* label;
    widget_t* container;
    window_t* about_win;

    desktop_get_size( &desk_size );

    point_t size = { 175, 30 };
    point_t pos = { ( desk_size.x - size.x ) / 2, ( desk_size.y - size.y ) / 2 };

    about_win = create_window( "iView - About", &pos, &size, W_ORDER_NORMAL, WINDOW_FIXED_SIZE );

    container = window_get_container( about_win );

    layout = create_borderlayout();
    panel_set_layout( container, layout );
    layout_dec_ref( layout );

    label = create_label_with_text( "yaOSp image viewer v0.1" );
    widget_add( container, label, BRD_CENTER );

    window_show( about_win );

    return 0;
}
