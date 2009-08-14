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
#include <lock/semaphore.h>

#define PS2_PORT_DATA    0x60
#define PS2_PORT_COMMAND 0x64
#define PS2_PORT_STATUS  0x64

#define PS2_CMD_RCTR 0x20
#define PS2_CMD_WCTR 0x60
#define PS2_CMD_TEST 0xAA

#define PS2_CMD_AUX_DISABLE 0xA7
#define PS2_CMD_AUX_ENABLE  0xA8
#define PS2_CMD_AUX_TEST    0xA9
#define PS2_CMD_AUX_LOOP    0xD3
#define PS2_CMD_AUX_SEND    0xD4

#define PS2_STATUS_IBF 0x02
#define PS2_STATUS_OBF 0x01

#define PS2_CTR_KBDINT 0x01
#define PS2_CTR_AUXINT 0x02
#define PS2_CTR_KBDDIS 0x10
#define PS2_CTR_AUXDIS 0x20
#define PS2_CTR_XLATE  0x40

#define PS2_RET_CTL_TEST 0x55

#define PS2_WAIT_TIMEOUT 10000
#define PS2_BUFSIZE      4096

#define LED_SCRLOCK  0x01
#define LED_NUMLOCK  0x02
#define LED_CAPSLOCK 0x04

typedef struct ps2_buffer {
    int read_pos;
    int write_pos;
    int buffer_size;
    uint8_t buffer[ PS2_BUFSIZE ];
    lock_id sync;
} ps2_buffer_t;

void ps2_lock( void );
void ps2_unlock( void );

int ps2_buffer_init( ps2_buffer_t* buffer );
int ps2_buffer_add( ps2_buffer_t* buffer, uint8_t data );
int ps2_buffer_size( ps2_buffer_t* buffer );
int ps2_buffer_sync( ps2_buffer_t* buffer );
uint8_t ps2_buffer_get( ps2_buffer_t* buffer );

int ps2_flush_buffer( void );

int ps2_command( uint8_t command );
int ps2_read_command( uint8_t command, uint8_t* data );
int ps2_write_command( uint8_t command, uint8_t data );
int ps2_write_read_command( uint8_t command, uint8_t* data );

int ps2_init_keyboard( void );
int ps2_init_mouse( void );

#endif /* _PS2_H_ */
