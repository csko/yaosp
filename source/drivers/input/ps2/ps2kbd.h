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

#define LED_SCRLOCK  0x01
#define LED_NUMLOCK  0x02
#define LED_CAPSLOCK 0x04

/* Ports used */
#define PS2KBD_PORT_KBD  0x60
#define PS2KBD_PORT_CONT 0x64

/* Commands on port 0x64 */
#define PS2KBD_CMD_STEST   0xAA /* Self test */
#define PS2KBD_CMD_KTEST   0xAB /* KBD test */
#define PS2KBD_CMD_ENABLE  0xAE /* Enable */
#define PS2KBD_CMD_LED     0xED /* LEDs */

int init_module( void );
int destroy_module( void );

#endif // _PS2KBD_H_
