/* uptime shell command
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

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

#include <yaosp/time.h>

int main( int argc, char** argv ) {
    uint32_t sec;
    uint32_t min;
    uint32_t hour;
    uint32_t day;

    time_t boot_time;
    time_t system_time;
    time_t uptime;

    boot_time = get_boot_time();
    system_time = get_system_time();
    uptime = ( system_time - boot_time ) / 1000000;

    sec = uptime % 60;
    uptime /= 60;

    min = uptime % 60;
    uptime /= 60;

    hour = uptime % 24;
    day = ( uint32_t )( uptime / 24 );

    printf( "Uptime:" );

    if ( uptime > 0 ) {
        printf( " %u day", ( uint32_t )uptime );
        if ( uptime > 1 ) {
            printf("s");
        }
    }

    if ( hour > 0 ) {
        printf( " %u hour", hour );
        if ( hour > 1 ) {
            printf("s");
        }
    }

    if ( min > 0 ) {
        printf( " %u minute", min );
        if ( min > 1 ) {
            printf("s");
        }
    }

    if ( sec > 0 ) {
        printf( " %u second", sec );
        if ( sec > 1 ) {
            printf("s");
        }

    }

    printf( "\n" );

    return EXIT_SUCCESS;
}
