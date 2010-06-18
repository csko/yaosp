/* Image viewer application
 *
 * Copyright (c) 2010 Attila Magyar, Zoltan Kovacs
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

#ifndef _UI_EVENT_H_
#define _UI_EVENT_H_

#include <ygui/widget.h>

int event_open_file( widget_t* widget, void* data );
int event_zoom_in( widget_t* widget, void* data );
int event_zoom_out( widget_t* widget, void* data );
int event_application_exit( widget_t* widget, void* data );
int event_help_about( widget_t* widget, void* data );

#endif /* _UI_EVENT_H_ */
