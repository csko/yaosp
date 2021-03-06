/* yaosp C library
 *
 * Copyright (c) 2009, 2010 Zoltan Kovacs
 * Copyright (c) 2009 Kornel Csernai
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

#ifdef __cplusplus
extern "C" {
#endif

typedef long clock_t;

typedef struct timeval {
    time_t      tv_sec;  /* Seconds */
    suseconds_t tv_usec; /* Microseconds */
} timeval_t;

typedef struct timezone {
    int tz_minuteswest;
    int tz_dsttime;
} timezone_t;

struct timespec {
    time_t tv_sec;  /* Seconds */
    long   tv_nsec; /* Nanoseconds */
};

typedef struct tm {
    int tm_sec;    /* Seconds. [0-60] (1 leap second) */
    int tm_min;    /* Minutes. [0-59] */
    int tm_hour;   /* Hours.   [0-23] */
    int tm_mday;   /* Day.     [1-31] */
    int tm_mon;    /* Month.   [0-11] */
    int tm_year;   /* Year [1970; ...] */
    int tm_wday;   /* Day of week. [0-6], 0=Sunday */
    int tm_yday;   /* Days in year. [0-365] */
    int tm_isdst;  /* Daylight saving [-1/0/1] */
} tm_t;

void tzset( void );

clock_t clock( void );
double difftime( time_t time1, time_t time0 );

time_t time(time_t* tloc);
int stime(time_t* tptr);

size_t strftime(char *s, size_t max, const char *format,
                const struct tm *tm);

/* Converts a broken-down time to UNIX timestamp */
time_t mktime(tm_t* tloc);

char* asctime(const tm_t* tm);
char* asctime_r(const tm_t* tm, char* buf);

tm_t* gmtime(const time_t* timep);
tm_t* gmtime_r(const time_t* timep, tm_t* result);

tm_t* localtime(const time_t* timep);
tm_t* localtime_r(const time_t* timep, tm_t* result);

char* ctime( const time_t* timep );
char* ctime_r( const time_t* timep, char* buf );

int nanosleep( const struct timespec* req, struct timespec* rem );

#ifdef __cplusplus
}
#endif

#endif /* _TIME_H_ */
