/* printf implementation
 *
 * Copyright (c) 2008, 2009 Zoltan Kovacs
 * Copyright (c) 2008 Kornel Csernai
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

#include "__printf.h"

int __printf( printf_helper_t* helper, void* data, const char* format, va_list args ) {
    int state, radix, ret;
    unsigned char *where, buf[PRINTF_BUFLEN];
    unsigned int flags, given_wd, actual_wd;
    state = flags = given_wd = ret = 0;
    long num;

    for ( ; *format != 0; format++ ) {
        switch(state){
            case 0:
                if(*format != '%'){
                    helper( data, *format );
                    ret++;
                    break;
                }else{
                    state++;
                    if(*(++format) == 0)
                        break;
                }
            /* No break */
            case 1:
                if(*format == '%'){ /* %%, we are done with this one */
                    helper( data, *format );
                    ret++;
                    state = flags = given_wd = 0;
                    break;
                }
                if(*format == '-'){ /* Left justify */
                    flags &= ~PRINTF_LZERO; /* '-' overrides '0' */
                    if(flags & PRINTF_LEFT) /* %-- is not allowed */
                        state = flags = given_wd = 0;
                    else
                        flags |= PRINTF_LEFT;
                    break;
                }
                if(*format == '+'){ /* Always have sign */
                    flags |= PRINTF_NEEDPLUS;
                    break;
                }
                state++;
                if(*format == '0' && !(flags & PRINTF_LEFT)){ /* Left padding with '0' */
                    flags |= PRINTF_LZERO;
                    format++;
                }
            /* No break */
            case 2:
                if(*format >= '0' && *format <= '9'){
                    given_wd = 10 * given_wd + (*format - '0');
                    break;
                }
                state++;
            case 3:
                if(*format == 'N'){
                    break;
                }
                if(*format == 'l'){
                    flags |= PRINTF_LONG;
                    break;
                }
                if(*format == 'h'){
                    flags |= PRINTF_SHORT;
                    break;
                }
                state++;
            case 4:
                where = buf + PRINTF_BUFLEN - 1;
                *where = '\0';
                switch(*format){
                    case 'X':
                        flags |= PRINTF_CAPITAL;
                        /* No break */
                    case 'x':
                    case 'p':
                    case 'n':
/*                        flags &= ~PRINTF_SIGNED; */
                        radix = 16;
                        goto PRINTF_DO_NUM;
                    case 'd':
                    case 'i':
                        flags |= PRINTF_SIGNED;
                        /* No break */
                    case 'u':
                        radix = 10;
                        goto PRINTF_DO_NUM;
                    case 'o':
                        radix = 8;
PRINTF_DO_NUM:
                        if(flags & PRINTF_LONG){
                            num = va_arg(args, unsigned long);
                        }else if(flags & PRINTF_SHORT){
                            if(flags & PRINTF_SIGNED)
                                num = va_arg(args, int);
                            else
                                num = va_arg(args, unsigned int);
                        } else {
                            if(flags & PRINTF_SIGNED)
                                num = va_arg(args, int);
                            else
                                num = va_arg(args, unsigned int);
                        }
                        if(flags & PRINTF_SIGNED){
                            if(num < 0){
                                flags |= PRINTF_NEEDSIGN;
                                num = -num;
                            }
                        }
                        do { /* Convert the number to the radix */
                            unsigned long temp;
                            temp = (unsigned long)num % radix;
                            where--;
                            if(temp < 10)
                                *where = temp + '0';
                            else if(flags & PRINTF_CAPITAL)
                                *where = temp - 10 + 'A';
                            else
                                *where = temp - 10 + 'a';
                            num = (unsigned long)num / radix;
                        } while(num != 0);
                        goto PRINTF_OUT;
                    case 'c':
                        flags &= ~PRINTF_LZERO;
                        where--;
                        *where = (unsigned char) va_arg(args, unsigned int);
                        actual_wd = 1;
                        goto PRINTF_OUT2;
                    case 's':
                        flags &= ~PRINTF_LZERO;
                        where = va_arg(args, unsigned char *);
PRINTF_OUT:
                        actual_wd = strlen((char *)where);
                        if(flags & PRINTF_NEEDSIGN || flags & PRINTF_NEEDPLUS)
                            actual_wd++;
                        if((flags & (PRINTF_NEEDSIGN | PRINTF_LZERO)) == (PRINTF_NEEDSIGN | PRINTF_LZERO)) {
                            helper( data, '-' );
                            ret++;
                        }else if(flags & PRINTF_NEEDPLUS && !(flags & PRINTF_NEEDSIGN)) {
                            helper( data, '+' );
                            ret++;
                        }

PRINTF_OUT2:
                        if((flags & PRINTF_LEFT) == 0){
                            while(given_wd > actual_wd){
                                helper( data, flags & PRINTF_LZERO ? '0' : ' ' );
                                ret++;
                                given_wd--;
                            }
                        }
                        if((flags & (PRINTF_NEEDSIGN | PRINTF_LZERO)) == PRINTF_NEEDSIGN){
                            helper( data, '-' );
                            ret++;
                        }
                        while(*where != '\0'){
                            helper( data, *where++ );
                            ret++;
                        }
                        if  (given_wd < actual_wd)
                            given_wd = 0;
                        else
                            given_wd -= actual_wd;
                        for(; given_wd; given_wd--){
                            helper( data, ' ' );
                            ret++;
                        }
                        break;
                    default:
                        break;
                }
            default:
                state = flags = given_wd = 0;
        }
    }

    return ret;
}
