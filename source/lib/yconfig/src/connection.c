/* yaosp configuration library
 *
 * Copyright (c) 2010 Zoltan Kovacs
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

#include <time.h>
#include <pthread.h>
#include <yaosp/ipc.h>

ipc_port_id ycfg_server_port = -1;
ipc_port_id ycfg_reply_port = -1;

pthread_mutex_t ycfg_lock = PTHREAD_MUTEX_INITIALIZER;

int ycfg_init( void ) {
    /* Get the configserver port ... */

    while ( 1 ) {
        int error;
        struct timespec slp_time;

        error = get_named_ipc_port( "configserver", &ycfg_server_port );

        if ( error == 0 ) {
            break;
        }

        /* Wait 200ms ... */

        slp_time.tv_sec = 0;
        slp_time.tv_nsec = 200 * 1000 * 1000;

        nanosleep( &slp_time, NULL );
    }

    /* Create a reply port */

    ycfg_reply_port = create_ipc_port();

    if ( ycfg_reply_port < 0 ) {
        return -1;
    }

    return 0;
}
