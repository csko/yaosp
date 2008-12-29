/* Port I/O functions
 *
 * Copyright (c) 2008 Zoltan Kovacs
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

#ifndef _ARCH_IO_H_
#define _ARCH_IO_H_

#include <types.h>

static inline uint8_t inb( uint16_t port ) {
    register uint8_t value;

    __asm__ __volatile__(
        "inb %1, %0\n"
        : "=a" ( value )
        : "dN" ( port )
    );

    return value;
}

static inline void outb( uint8_t data, uint16_t port ) {
    __asm__ __volatile__(
        "outb %1, %0\n"
        :
        : "dN" ( port ), "a" ( data )
    );
}

static inline uint16_t inw( uint16_t port ) {
    register uint16_t value;

    __asm__ __volatile__(
        "inw %1, %0\n"
        : "=a" ( value )
        : "dN" ( port )
    );

    return value;
}

static inline void outw( uint16_t data, uint16_t port ) {
    __asm__ __volatile__(
        "outw %1, %0\n"
        :
        : "dN" ( port ), "a" ( data )
    );
}

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

static inline void inws( uint16_t* buffer, size_t count, uint16_t port ) {
    __asm__ __volatile__(
        "rep insw\n"
        : "=c" ( count ), "=D" ( buffer )
        : "d" ( port ), "0" ( count ), "1" ( buffer )
        : "memory"
    );
}

static inline void outws( const uint16_t* buffer, size_t count, uint16_t port ) {
    __asm__ __volatile__(
        "rep outsw\n"
        : "=c" ( count ), "=S" ( buffer )
        : "d" ( port ), "0" ( count ), "1" ( buffer )
        : "memory"
    );
}

static inline void inls( uint32_t* buffer, size_t count, uint16_t port ) {
    __asm__ __volatile__(
        "rep insl\n"
        : "=c" ( count ), "=D" ( buffer )
        : "d" ( port ), "0" ( count ), "1" ( buffer )
        : "memory"
    );
}

static inline void outls( const uint32_t* buffer, size_t count, uint16_t port ) {
    __asm__ __volatile__(
        "rep outsl\n"
        : "=c" ( count ), "=S" ( buffer )
        : "d" ( port ), "0" ( count ), "1" ( buffer )
        : "memory"
    );
}

#endif // _ARCH_IO_H_
