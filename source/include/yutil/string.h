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

#ifndef _YUTIL_STRING_H_
#define _YUTIL_STRING_H_

typedef struct string {
    int length;
    int max_length;
    int realloc_size;
    int realloc_mask;
    char* buffer;
} string_t;

int string_clear( string_t* string );
int string_append( string_t* string, char* data, int size );
int string_insert( string_t* string, int pos, char* data, int size );
int string_remove( string_t* string, int pos, int count );

int string_length( string_t* string );

const char* string_c_str( string_t* string );
char* string_dup_buffer( string_t* string );

/* UTF8 handling functions */

int string_prev_utf8_char( string_t* string, int pos );
int string_erase_utf8_char( string_t* string, int pos );

int init_string( string_t* string );
int init_string_from_buffer( string_t* string, const char* data, size_t size );
int destroy_string( string_t* string );

#endif /* _YUTIL_STRING_H_ */
