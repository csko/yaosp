/* i386 specific debug output for the memory allocator
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

#include <mm/kmalloc.h>

#ifdef ENABLE_KMALLOC_DEBUG

#include <lib/string.h>
#include <arch/io.h>

#define EVENT_MALLOC  0x01
#define EVENT_FREE    0x02
#define EVENT_BT_ITEM 0x03

static int serial_init( void ) {
    outb( 0x00, 0x3F8 + 1 );
    outb( 0x80, 0x3F8 + 3 );
    outb( 0x03, 0x3F8 );
    outb( 0x00, 0x3F8 + 1 );
    outb( 0x03, 0x3F8 + 3 );
    outb( 0xC7, 0x3F8 + 2 );
    outb( 0x0B, 0x3F8 + 4 );

    return 0;
}

static inline int serial_output_byte( uint8_t c ) {
    while ( ( inb( 0x3F8 + 5 ) & 0x20 ) == 0 ) ;
    outb( c, 0x3F8 );
    return 0;
}

static int serial_output_uint32( uint32_t n ) {
    int i;
    uint8_t* p;

    p = ( uint8_t* )&n;

    for ( i = 0; i < sizeof(uint32_t); i++, p++ ) {
        serial_output_byte( *p );
    }

    return 0;
}

static int serial_output_pointer( void* _p ) {
    int i;
    uint8_t* p;

    p = ( uint8_t* )&_p;

    for ( i = 0; i < sizeof(void*); i++, p++ ) {
        serial_output_byte( *p );
    }

    return 0;
}

static int serial_malloc( uint32_t size, void* p ) {
    serial_output_byte( EVENT_MALLOC );
    serial_output_uint32( size );
    serial_output_pointer( p );

    return 0;
}

static int serial_malloc_trace( const char* name ) {
    size_t i;
    size_t length;

    serial_output_byte( EVENT_BT_ITEM );

    length = strlen( name );
    serial_output_byte( length );
    for ( i = 0; i < length; i++ ) {
        serial_output_byte( name[i] );
    }

    return 0;
}

static int serial_free( void* p ) {
    serial_output_byte( EVENT_FREE );
    serial_output_pointer( p );

    return 0;
}

static kmalloc_debug_output_t serial_kmalloc_output = {
    .init = serial_init,
    .malloc = serial_malloc,
    .malloc_trace = serial_malloc_trace,
    .free = serial_free
};

int init_serial_kmalloc_output( void ) {
    kmalloc_set_debug_output( &serial_kmalloc_output );
    return 0;
}

#endif
