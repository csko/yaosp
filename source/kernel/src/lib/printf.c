/* printf() like function for the kernel
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

#include <types.h>
#include <lib/printf.h>

static const char lower_digits[] = "0123456789abcdef";
static const char upper_digits[] = "0123456789ABCDEF";

static void print_number( printf_helper_t* helper, void* data, uint32_t number, const char* digits, int base ) {
    int off = 0;
    char buffer[ 32 ];

    while ( number > 0 ) {
        buffer[ off++ ] = digits[ number % base ];
        number = number / base;
    }

    if ( off == 0 ) {
        helper( data, '0' );
    } else {
        do {
            off--;
            helper( data, buffer[ off ] );
        } while ( off > 0 );
    }
}

static void print_string( printf_helper_t* helper, void* data, char* string ) {
    while ( *string ) {
        helper( data, *string++ );
    }
}

int do_printf( printf_helper_t* helper, void* data, const char* format, va_list args ) {
    bool modifier = false;

    for ( ; *format != 0; format++ ) {
        if ( modifier ) {
            switch ( *format ) {
                case 'c' : {
                    char c = ( char )va_arg( args, int );
                    helper( data, c );
                    break;
                }

                case 's' : {
                    char* string = va_arg( args, char* );
                    print_string( helper, data, string );
                    break;
                }

                case 'd' : {
                    int number = va_arg( args, int );

                    if ( number < 0 ) {
                        helper( data, '-' );
                        number = -number;
                    }

                    print_number( helper, data, number, lower_digits, 10 );

                    break;
                }

                case 'u' : {
                    uint32_t number = va_arg( args, uint32_t );
                    print_number( helper, data, number, lower_digits, 10 );
                    break;
                }

                case 'x' : {
                    uint32_t number = va_arg( args, uint32_t );
                    print_number( helper, data, number, lower_digits, 16 );
                    break;
                }

                case 'X' : {
                    uint32_t number = va_arg( args, uint32_t );
                    print_number( helper, data, number, upper_digits, 16 );
                    break;
                }
            }

            modifier = false;
        } else {
            switch ( *format ) {
                case '%' :
                    modifier = true;
                    break;

                default:
                    helper( data, *format );
                    break;
            }
        }
    }

    return 0;
}
