/* PS/2 keyboard driver
 *
 * Copyright (c) 2009 Kornel Csernai
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

#ifndef _PS2KBD_H_
#define _PS2KBD_H_

#define PS2KBD_BUFSIZE 0x1000
#define SCRLOCK_TOGGLE  0x01
#define NUMLOCK_TOGGLE  0x02
#define CAPSLOCK_TOGGLE 0x04

int init_module( void );
int destroy_module( void );

#endif // _PS2KBD_H_
