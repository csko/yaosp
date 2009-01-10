/* yaosp C library
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

#ifndef _TIME_H_
#define _TIME_H_
#include <sys/types.h>

#define time_t uint64_t
#define suseconds_t int

typedef struct timeval {
    time_t      tv_sec;    /* Seconds */
    suseconds_t tv_usec;   /* Microseconds */
} timeval_t ;

typedef struct timezone {
    int tz_minuteswest;
    int tz_dsttime;
} timezone_t ;
        

typedef struct tm {
    int sec;    /* Seconds. [0-60] (1 leap second) */
    int min;    /* Minutes. [0-59] */
    int hour;   /* Hours.   [0-23] */
    int mday;   /* Day.     [1-31] */
    int mon;    /* Month.   [0-11] */
    int year;   /* Year [1970; ...] */
    int wday;   /* Day of week. [0-6], 0=Sunday */
    int yday;   /* Days in year. [0-365] */
    int isdst;  /* Daylight saving [-1/0/1] */
} tm_t ;

#endif // _TIME_H_
