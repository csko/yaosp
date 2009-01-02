/* Fork implementation
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

#ifndef _ARCH_FORK_H_
#define _ARCH_FORK_H_

static inline int fork( void ) {
    int new_tid;

    __asm__ __volatile__(
        "int $0x80\n"
        : "=a" ( new_tid )
        : "0" ( 0 )
    );

    return new_tid;
}

int arch_do_fork( thread_t* old_thread, thread_t* new_thread );

#endif // _ARCH_FORK_H_
