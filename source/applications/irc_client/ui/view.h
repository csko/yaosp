/* IRC client
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

#ifndef _UI_VIEW_H_
#define _UI_VIEW_H_

#include <yutil/array.h>

struct view;

typedef struct view_operations {
    const char* ( *get_title )( struct view* view );
    int ( *handle_command )( struct view* view, const char* command, const char* params );
    int ( *handle_text )( struct view* view, const char* text );
} view_operations_t;

typedef struct view {
    array_t lines;
    view_operations_t* operations;
    void* data;
} view_t;

extern view_t server_view;

int view_add_text( view_t* view, const char* text );

int active_view_add_text( const char* text );

int init_server_view( void );

int init_view( view_t* view, view_operations_t* operations, void* data );

#endif /* _UI_VIEW_H_ */
