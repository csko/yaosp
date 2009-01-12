/* Date and time handling
 *
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
#ifndef _LIBTIME_H_
#define _LIBTIME_H_
#include <time.h> /* Time structures */

#define EPOCH 1970 // Counting only time elapsed since 1 Jan 1970

#define SECONDS_PER_DAY 86400
#define SECONDS_PER_HOUR 3600
#define SECONDS_PER_MINUTE 60
#define MINUTES_PER_HOUR 60
#define HOURS_PER_DAY 24

/* The number of days that come before each month */
extern const unsigned short int monthdays[ 13 ]; /* Common year */
extern const unsigned short int monthdays2[ 13 ]; /* Leap year */
/* The number of days passed in N years after 1970 or any leap year */
extern const unsigned short int sumofdays[ 60 ];

/* Converts a UNIX timestamp to a broken-down time */
tm_t gettime(time_t time);

/* Returns the day of the week, 0=Sunday */
int dayofweek(int year, int month, int day);

/* Returns the number of days since the epoch */
int daysdiff(int year, int month, int day);

/* Gets the system uptime in seconds */
uint64_t uptime( void );

#endif // _LIBTIME_H_
