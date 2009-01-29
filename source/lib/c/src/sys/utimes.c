/* utimes function
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

#include <errno.h>
#include <utime.h>

#include <yaosp/syscall.h>
#include <yaosp/syscall_table.h>

int utimes( const char* filename, const timeval_t times[2] ) {
    struct utimbuf tmp;

    /* NOTE: no microsecond precision */
    tmp.actime = times[ 0 ].tv_sec;
    tmp.modtime = times[ 1 ].tv_sec;

    /* This is a wrapper around utime() */
    return utime( filename, &tmp );
}
