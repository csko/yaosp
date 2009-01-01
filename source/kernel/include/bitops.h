/* Parallel AT Attachment driver
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

#ifndef _BITOPS_H_
#define _BITOPS_H_

#include <types.h>

static inline uint32_t bswap32( uint32_t value ) {
    return ( ( value & 0xFF000000 ) >> 24 |
             ( value & 0x00FF0000 ) >> 8 |
             ( value & 0x0000FF00 ) << 8 |
             ( value & 0x000000FF ) << 24 );
}

#endif // _BITOPS_H_
