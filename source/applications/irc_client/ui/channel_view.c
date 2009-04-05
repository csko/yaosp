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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

#include "view.h"
#include "channel_view.h"
#include "../network/irc.h"

extern char* my_nick;

static const char* channel_get_title( view_t* view ) {
    channel_data_t* channel;

    channel = ( channel_data_t* )view->data;

    return channel->name;
}

static int channel_handle_text( view_t* view, const char* text ) {
    char buf[ 256 ];
    char timestamp[ 128 ];
    struct tm* tmval;
    time_t now;
    char* timestamp_format = "%D %T"; /* TODO: make global variable */

    channel_data_t* channel;

    channel = ( channel_data_t* )view->data;

    irc_send_privmsg( channel->name, text );

    /* Create timestamp */
    time( &now );

    if( now != (time_t) -1 ){
        tmval = ( struct tm* )malloc( sizeof( struct tm ) );
        gmtime_r( &now, tmval );

        if ( tmval != NULL ) {
            strftime( ( char* )timestamp, 128, timestamp_format, tmval );
        }
        free( tmval );
    }

    snprintf( buf, sizeof( buf ), "%s <%s> %s", timestamp, my_nick, text );
    view_add_text( view, buf );

    return 0;
}

static view_operations_t channel_operations = {
    .get_title = channel_get_title,
    .handle_command = NULL,
    .handle_text = channel_handle_text
};

view_t* create_channel_view( const char* name ) {
    int error;
    view_t* view;
    channel_data_t* channel;

    channel = ( channel_data_t* )malloc( sizeof( channel_data_t ) );

    if ( channel == NULL ) {
        goto error1;
    }

    channel->name = strdup( name );

    if ( channel->name == NULL ) {
        goto error2;
    }

    view = ( view_t* )malloc( sizeof( view_t ) );

    if ( view == NULL ) {
        goto error3;
    }

    error = init_view( view, &channel_operations, ( void* )channel );

    if ( error < 0 ) {
        goto error4;
    }

    return view;

error4:
    free( view );

error3:
    free( channel->name );

error2:
    free( channel );

error1:
    return NULL;
}

void destroy_channel_view( view_t* view ) {
    channel_data_t* channel = ( channel_data_t* ) view->data;
    free( channel->name );
    free( channel );
    free( view );
}
