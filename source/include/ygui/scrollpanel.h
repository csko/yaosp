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

#ifndef _YGUI_SCROLLPANEL_H_
#define _YGUI_SCROLLPANEL_H_

#include <ygui/widget.h>
#include <ygui/yconstants.h>

int scroll_panel_get_v_size( widget_t* widget );

int scroll_panel_set_v_offset( widget_t* widget, int offset );

widget_t* create_scroll_panel( scrollbar_policy_t v_scroll_policy, scrollbar_policy_t h_scroll_policy );

#endif /* _YGUI_SCROLLPANEL_H_ */
