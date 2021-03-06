/* fpurge function
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

#include <stdio.h>

#include <yaosp/debug.h>

int fpurge( FILE* stream ) {
    if ( stream->flags & __FILE_NOBUF ) {
        return 0;
    }

    stream->has_ungotten = 0;

    if ( stream->flags & __FILE_BUFINPUT ) {
        stream->buffer_pos = stream->buffer_data_size;
    } else {
        stream->buffer_pos = 0;
    }

    return 0;
}
