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
#include <errno.h>
#include <pthread.h>
#include <yaosp/ipc.h>

#include <ygui/font.h>
#include <ygui/protocol.h>

extern ipc_port_id app_reply_port;
extern ipc_port_id app_server_port;

extern pthread_mutex_t app_lock;

int font_get_height( font_t* font ) {
    if ( font == NULL ) {
        return 0;
    }

    return font->ascender - font->descender + font->line_gap;
}

int font_get_ascender( font_t* font ) {
    if ( font == NULL ) {
        return 0;
    }

    return font->ascender;
}

int font_get_descender( font_t* font ) {
    if ( font == NULL ) {
        return 0;
    }

    return font->descender;
}

int font_get_line_gap( font_t* font ) {
    if ( font == NULL ) {
        return 0;
    }

    return font->line_gap;
}

int font_get_string_width( font_t* font, const char* text, int length ) {
    int error;
    int request_len;
    msg_get_str_width_t* request;
    msg_get_str_width_reply_t reply;

    if ( ( font == NULL ) ||
         ( text == NULL ) ) {
        return -EINVAL;
    }

    if ( length == -1 ) {
        length = strlen( text );
    }

    if ( length == 0 ) {
        return 0;
    }

    request_len = sizeof( msg_get_str_width_t ) + length;

    request = ( msg_get_str_width_t* )malloc( request_len );

    if ( request == NULL ) {
        error = -ENOMEM;
        goto error1;
    }

    request->reply_port = app_reply_port;
    request->font_handle = font->handle;
    request->length = length;

    memcpy( ( void* )( request + 1 ), text, length );

    pthread_mutex_lock( &app_lock );

    error = send_ipc_message( app_server_port, MSG_FONT_GET_STR_WIDTH, request, request_len );

    free( request );

    if ( error < 0 ) {
        pthread_mutex_unlock( &app_lock );

        goto error1;
    }

    error = recv_ipc_message( app_reply_port, NULL, &reply, sizeof( msg_get_str_width_reply_t ), INFINITE_TIMEOUT );

    pthread_mutex_unlock( &app_lock );

    if ( error < 0 ) {
        goto error1;
    }

    return reply.width;

 error1:
    return error;
}

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

    pthread_mutex_lock( &app_lock );

    error = send_ipc_message( app_server_port, MSG_FONT_CREATE, request, request_len );

    free( request );

    if ( error < 0 ) {
        pthread_mutex_unlock( &app_lock );

        goto error2;
    }

    error = recv_ipc_message( app_reply_port, NULL, &reply, sizeof( msg_create_font_reply_t ), INFINITE_TIMEOUT );

    pthread_mutex_unlock( &app_lock );

    if ( ( error < 0 ) ||
         ( reply.handle < 0 ) ) {
        goto error2;
    }

    font->handle = reply.handle;
    font->ascender = reply.ascender;
    font->descender = reply.descender;
    font->line_gap = reply.line_gap;

    return font;

 error2:
    free( font );

 error1:
    return NULL;
}

int destroy_font( font_t* font ) {
    /* todo: unregister in guiserver */
    free( font );

    return 0;
}
