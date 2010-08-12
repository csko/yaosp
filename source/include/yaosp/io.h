/* Port I/O functions
 *
 * Copyright (c) 2010 Zoltan Kovacs
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

#ifndef _YAOSP_IO_H_
#define _YAOSP_IO_H_

#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline uint32_t inl( uint16_t port ) {
    register uint32_t value;

    __asm__ __volatile__(
        "inl %1, %0\n"
        : "=a" ( value )
        : "dN" ( port )
    );

    return value;
}

static inline void outl( uint32_t data, uint16_t port ) {
    __asm__ __volatile__(
        "outl %1, %0\n"
        :
        : "dN" ( port ), "a" ( data )
    );
}

#ifdef __cplusplus
}
#endif

#endif /* _YAOSP_IO_H_ */
