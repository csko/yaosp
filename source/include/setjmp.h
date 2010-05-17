/* yaosp C library
 *
 * Copyright (c) 2009, 2010 Zoltan Kovacs
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

#ifndef _SETJMP_H_
#define _SETJMP_H_

#include <signal.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned long jmp_buf[ 6 ];

typedef struct _sigjmp_buf {
    jmp_buf buf;
    int restore_mask;
    sigset_t mask;
} sigjmp_buf[1];

int setjmp( jmp_buf env );
int sigsetjmp( sigjmp_buf env, int savemask );

void longjmp( jmp_buf env, int val );
void siglongjmp( sigjmp_buf env, int val );

#ifdef __cplusplus
}
#endif

#endif /* _SETJMP_H_ */
