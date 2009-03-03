/* yaosp C library
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

#include <time.h>

#include "time_int.h"

time_t mktime( tm_t* tm ) {
    if ( tm->tm_year > 2100 ) {
        return -1;
    }

    return daysdiff( tm->tm_year, tm->tm_mon, tm->tm_mday ) * SECONDS_PER_DAY +
           tm->tm_hour * SECONDS_PER_HOUR + tm->tm_min * SECONDS_PER_MINUTE + tm->tm_sec;
}
