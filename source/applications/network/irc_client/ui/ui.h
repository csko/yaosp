/* IRC client
 *
 * Copyright (c) 2009 Zoltan Kovacs, Kornel Csernai
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

#ifndef _UI_UI_H_
#define _UI_UI_H_

#include "view.h"

view_t* ui_get_channel( const char* chan_name );
int ui_handle_command( const char* command, const char* params );

void ui_draw_view( view_t* view );
int ui_activate_view( view_t* view );
int ui_error_message( const char* errormsg );
int ui_debug_message( const char* debugmsg );

int init_ui( void );
int destroy_ui( void );

#endif /* _UI_UI_H_ */
