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

#include <time.h>
#include <yaosp/ipc.h>
#include <yaosp/debug.h>

ipc_port_id guiserver_port = -1;

int create_application( void ) {
    int error;
    struct timespec slp_time;

    dbprintf( "Waiting for guiserver to start ...\n" );

    while ( 1 ) {
        error = get_named_ipc_port( "guiserver", &guiserver_port );

        if ( error == 0 ) {
            break;
        }

        slp_time.tv_sec = 1;
        slp_time.tv_nsec = 0;

        nanosleep( &slp_time, NULL );
    }

    dbprintf( "Guiserver port: %d\n", guiserver_port );

    return 0;
}

int run_application( void ) {
    return 0;
}
