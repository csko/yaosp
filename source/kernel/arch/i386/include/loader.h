/* Application loader
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

#ifndef _ARCH_LOADER_H_
#define _ARCH_LOADER_H_

int execve( const char* path, char** argv, char** envp ) {
    int result;

    __asm__ __volatile__(
        "int $0x80\n"
        : "=a" ( result )
        : "0" ( 1 ), "b" ( path ), "c" ( argv ), "d" ( envp )
    );

    return result;
}

#endif // _ARCH_LOADER_H_
