/* Terminal driver
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

#ifndef _TERMINAL_TERMINAL_H_
#define _TERMINAL_TERMINAL_H_

#define MAX_TERMINAL_COUNT 6

#define TERMINAL_ACCEPTS_USER_INPUT 0x01

typedef struct terminal {
    int master_pty;
    int flags;
} terminal_t;

extern terminal_t* terminals[ MAX_TERMINAL_COUNT ];

#endif // _TERMINAL_TERMINAL_H_
