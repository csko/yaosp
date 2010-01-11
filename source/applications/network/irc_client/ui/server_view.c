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

#include <stdlib.h>

#include "ui.h"
#include "view.h"

view_t server_view;

static const char* server_title = "::[ yaOSp IRC client ]::";

static const char* server_get_title( view_t* view ) {
    return server_title;
}

static int server_handle_command( view_t* view, const char* command, const char* params ) {
    return ui_handle_command( command, params );
}

static view_operations_t server_view_ops = {
    .get_title = server_get_title,
    .handle_command = server_handle_command,
    .handle_text = NULL
};

int init_server_view( void ) {
    int error;

    error = init_view( &server_view, &server_view_ops, NULL );

    if ( error < 0 ) {
        return error;
    }

    return 0;
}
