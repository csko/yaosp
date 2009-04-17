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

#include <stdlib.h>
#include <string.h>
#include <yaosp/semaphore.h>
#include <yaosp/ipc.h>

#include <ygui/font.h>
#include <ygui/protocol.h>

extern ipc_port_id app_reply_port;
extern ipc_port_id app_server_port;

extern semaphore_id app_lock;

font_t* create_font( const char* family, const char* style, font_properties_t* properties ) {
    int error;
    font_t* font;
    size_t family_len;
    size_t style_len;
    int request_len;
    msg_create_font_t* request;
    msg_create_font_reply_t reply;

    if ( ( family == NULL ) ||
         ( style == NULL ) ||
         ( properties == NULL ) ) {
        return NULL;
    }

    font = ( font_t* )malloc( sizeof( font_t ) );

    if ( font == NULL ) {
        goto error1;
    }

    family_len = strlen( family );
    style_len = strlen( style );
    request_len = sizeof( msg_create_font_t ) + family_len + 1 + style_len + 1;

    request = ( msg_create_font_t* )malloc( request_len );

    if ( request == NULL ) {
        goto error2;
    }

    request->reply_port = app_reply_port;
    memcpy( &request->properties, properties, sizeof( font_properties_t ) );
    memcpy( ( void* )( request + 1 ), family, family_len + 1 );
    memcpy( ( uint8_t* )( request + 1 ) + family_len + 1, style, style_len + 1 );

    LOCK( app_lock );

    error = send_ipc_message( app_server_port, MSG_CREATE_FONT, request, request_len );

    free( request );

    if ( error < 0 ) {
        UNLOCK( app_lock );

        goto error2;
    }

    error = recv_ipc_message( app_reply_port, NULL, &reply, sizeof( msg_create_font_reply_t ), INFINITE_TIMEOUT );

    UNLOCK( app_lock );

    if ( ( error < 0 ) ||
         ( reply.handle < 0 ) ) {
        goto error2;
    }

    font->handle = reply.handle;

    return font;

error2:
    free( font );

error1:
    return NULL;
}
