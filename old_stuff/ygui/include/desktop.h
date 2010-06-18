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

#ifndef _YGUI_DESKTOP_H_
#define _YGUI_DESKTOP_H_

#include <ygui/point.h>
#include <ygui/yconstants.h>

#include <yutil/array.h>

int desktop_get_size( point_t* size );

array_t* desktop_get_screen_modes( void );
int desktop_put_screen_modes( array_t* mode_list );

int desktop_set_screen_mode( screen_mode_info_t* mode_info );

#endif /* _YGUI_DESKTOP_H_ */
