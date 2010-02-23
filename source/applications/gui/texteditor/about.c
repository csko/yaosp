/* Text editor application
 *
 * Copyright (c) 2009, 2010 Zoltan Kovacs
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
#include <ygui/layout/borderlayout.h>

#include "about.h"

int about_open( void ) {
    layout_t* layout;
    widget_t* label;
    widget_t* container;
    window_t* about_win;

    point_t size = { 175, 30 };
    point_t pos = { ( 640 - size.x ) / 2, ( 480 - size.y ) / 2 };

    about_win = create_window( "Text editor - About", &pos, &size, W_ORDER_NORMAL, WINDOW_FIXED_SIZE );

    container = window_get_container( about_win );

    layout = create_borderlayout();
    panel_set_layout( container, layout );
    layout_dec_ref( layout );

    label = create_label_with_text( "yaOSp text editor v0.1" );
    widget_add( container, label, BRD_CENTER );
    widget_dec_ref( label );

    window_show( about_win );

    return 0;
}
