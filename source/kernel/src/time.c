/* Date and time handling
 *
 * Copyright (c) 2009 Kornel Csernai, Zoltan Kovacs
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

#include <lib/string.h>
#include <lib/time.h>
#include <time.h>
#include <errno.h>

int sys_stime( int* tptr ) {
    if ( tptr == NULL ) {
        return -EINVAL;
    }

    set_system_time( ( time_t* )tptr );

    return 0;
}

time_t time( time_t* tloc ) {
    time_t ret;

    ret = ( time_t )( get_system_time() / 1000000 );

    if ( tloc != NULL ) {
        *tloc = ret;
    }

    return ret;
}

time_t mktime( tm_t* _time ) {
    if ( _time->tm_year > 2100 ) {
        return -1;
    }

    return daysdiff(
        _time->tm_year,
        _time->tm_mon,
        _time->tm_mday
    ) * SECONDS_PER_DAY + _time->tm_hour * SECONDS_PER_HOUR + _time->tm_min * SECONDS_PER_MINUTE + _time->tm_sec;
}

int sys_get_boot_time( time_t* _time ) {
    *_time = get_boot_time();
    return 0;
}

int sys_get_system_time( time_t* _time ) {
    *_time = get_system_time();
    return 0;
}

int sys_get_idle_time( time_t* _time ) {
    *_time = get_idle_time();
    return 0;
}
