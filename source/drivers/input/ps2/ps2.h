/* PS/2 driver
 *
 * Copyright (c) 2009 Kornel Csernai, Zoltan Kovacs
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

#ifndef _PS2_H_
#define _PS2_H_

#include <types.h>

#define PS2_PORT_DATA    0x60
#define PS2_PORT_COMMAND 0x64
#define PS2_PORT_STATUS  0x64

#define PS2_CMD_RCTR 0x20
#define PS2_CMD_WCTR 0x60

#define PS2_STATUS_IBF 0x02
#define PS2_STATUS_OBF 0x01

#define PS2_CTR_KBDINT 0x01
#define PS2_CTR_AUXINT 0x02
#define PS2_CTR_KBDDIS 0x10
#define PS2_CTR_AUXDIS 0x20
#define PS2_CTR_XLATE  0x40

#define PS2_WAIT_TIMEOUT 10000
#define PS2_KBD_BUFSIZE   4096

#define LED_SCRLOCK  0x01
#define LED_NUMLOCK  0x02
#define LED_CAPSLOCK 0x04

void ps2_lock( void );
void ps2_unlock( void );

int ps2_flush_buffer( void );
int ps2_read_command( uint8_t command, uint8_t* data );
int ps2_write_command( uint8_t command, uint8_t data );

int ps2_init_keyboard( void );

#endif // _PS2_H_
