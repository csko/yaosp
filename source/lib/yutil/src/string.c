/* String implementation
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

#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <inttypes.h>
#include <sys/param.h>

#include <yutil/string.h>

int string_clear( string_t* string ) {
    string->length = 0;
    string->buffer[ 0 ] = 0;

    return 0;
}

int string_append( string_t* string, char* data, int size ) {
    return string_insert( string, string->length, data, size );
}

int string_insert( string_t* string, int pos, char* data, int size ) {
    if ( ( pos < 0 ) ||
         ( pos > string->length ) ) {
        return -EINVAL;
    }

    if ( ( string->length + size ) > string->max_length ) {
        int new_length;
        char* new_buffer;

        new_length = string->max_length + ( ( size + string->realloc_size - 1 ) & ~( string->realloc_mask ) );

        new_buffer = ( char* )realloc( string->buffer, new_length + 1 );

        if ( new_buffer == NULL ) {
            return -ENOMEM;
        }

        string->buffer = new_buffer;
        string->max_length = new_length;
    }

    memmove(
        &string->buffer[ pos + size ],
        &string->buffer[ pos ],
        string->length - pos + 1
    );

    memcpy(
        &string->buffer[ pos ],
        data,
        size
    );

    string->length += size;

    return 0;
}

int string_remove( string_t* string, int pos, int count ) {
    if ( ( pos < 0 ) ||
         ( pos > string->length ) ||
         ( count < 0 ) ) {
        return -EINVAL;
    }

    count = MIN( count, string->length - pos );

    if ( count == 0 ) {
        return 0;
    }

    memmove(
        &string->buffer[ pos ],
        &string->buffer[ pos + count ],
        string->length - ( pos + count ) + 1
    );

    string->length -= count;

    return 0;
}

int string_length( string_t* string ) {
    return string->length;
}

const char* string_c_str( string_t* string ) {
    return string->buffer;
}

char* string_dup_buffer( string_t* string ) {
    char* new_buffer;

    new_buffer = ( char* )malloc( string->length + 1 );

    if ( new_buffer == NULL ) {
        return NULL;
    }

    memcpy( new_buffer, string->buffer, string->length + 1 );

    return new_buffer;
}

static inline int utf8_is_first_byte( uint8_t byte ) {
    return ( ( ( byte & 0x80 ) == 0x00 ) ||
             ( ( byte & 0xC0 ) == 0xC0 ) );
}

static inline int utf8_char_length( uint8_t byte ) {
    return ( ( ( 0xE5000000 >> ( ( byte >> 3 ) & 0x1E ) ) & 3 ) + 1 );
}

int string_prev_utf8_char( string_t* string, int pos ) {
    if ( ( string == NULL ) ||
         ( pos < 0 ) ||
         ( pos > string->length ) ) {
        return -EINVAL;
    }

    if ( pos == 0 ) {
        return 0;
    }

    pos--;

    while ( ( pos > 0 ) &&
            ( !utf8_is_first_byte( string->buffer[ pos ] ) ) ) {
        pos--;
    }

    return pos;
}

int string_erase_utf8_char( string_t* string, int pos ) {
    if ( ( string == NULL ) ||
         ( pos < 0 ) ||
         ( pos >= string->length ) ||
         ( !utf8_is_first_byte( string->buffer[ pos ] ) ) ) {
        return -EINVAL;
    }

    return string_remove( string, pos, utf8_char_length( string->buffer[ pos ] ) );
}

int init_string( string_t* string ) {
    string->length = 0;
    string->max_length = 8;
    string->realloc_size = 8;
    string->realloc_mask = string->realloc_size - 1;

    string->buffer = ( char* )malloc( string->max_length + 1 );

    if ( string->buffer == NULL ) {
        return -ENOMEM;
    }

    string->buffer[ 0 ] = 0;

    return 0;
}

int init_string_from_buffer( string_t* string, const char* data, size_t size ) {
    string->realloc_size = 8;
    string->realloc_mask = string->realloc_size - 1;

    string->length = size;
    string->max_length = ( size + string->realloc_size - 1 ) & ~string->realloc_mask;

    string->buffer = ( char* )malloc( string->max_length + 1 );

    if ( string->buffer == NULL ) {
        return -ENOMEM;
    }

    memcpy( string->buffer, data, size );
    string->buffer[ size ] = 0;

    return 0;
}

int destroy_string( string_t* string ) {
    if ( string->buffer != NULL ) {
        free( string->buffer );
        string->buffer = NULL;
    }

    return 0;
}

