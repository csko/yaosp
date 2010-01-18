/* PS/2 controller driver
 *
 * Copyright (c) 2010 Zoltan Kovacs
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
#include <irq.h>
#include <lock/semaphore.h>

#include <arch/io.h>
#include <arch/atomic.h>

#define PS2_PORT_DATA    0x60
#define PS2_PORT_COMMAND 0x64
#define PS2_PORT_STATUS  0x64

#define PS2_STATUS_OBF      0x01
#define PS2_STATUS_IBF      0x02
#define PS2_STATUS_AUX_DATA 0x20

#define PS2_CTRL_READ_CMD  0x20
#define PS2_CTRL_WRITE_CMD 0x60
#define PS2_CTRL_SELF_TEST 0xAA

#define PS2_CMD_GET_DEVICE_ID   0xF2
#define PS2_CMD_SET_SAMPLE_RATE 0xF3
#define PS2_CMD_ENABLE          0xF4
#define PS2_CMD_DISABLE         0xF5
#define PS2_CMD_RESET           0xFF

#define PS2_KBD_INT   0x01
#define PS2_AUX_INT   0x02
#define PS2_KBD_DIS   0x10
#define PS2_AUX_DIS   0x20
#define PS2_TRANSLATE 0x40

#define PS2_KBD_IRQ 1
#define PS2_AUX_IRQ 12

#define PS2_DEV_KEYBOARD 0
#define PS2_DEV_MOUSE    1
#define PS2_DEV_COUNT    5

#define PS2_FLAG_KEYBOARD 0x01
#define PS2_FLAG_ENABLED  0x02
#define PS2_FLAG_ACK      0x04
#define PS2_FLAG_NACK     0x08
#define PS2_FLAG_CMD      0x10
#define PS2_FLAG_GET_ID   0x20

#define PS2_REPLY_ACK                   0xfa
#define PS2_REPLY_RESEND                0xfe
#define PS2_REPLY_ERROR                 0xfc

#define PS2_DEV_ID_STANDARD     0
#define PS2_DEV_ID_INTELLIMOUSE 3

#define PS2_PACKET_STANDARD     3
#define PS2_PACKET_INTELLIMOUSE 4

#define PS2_CTRL_WAIT_TIMEOUT 500000

static inline uint8_t ps2_read_data( void ) {
    return inb( PS2_PORT_DATA );
}

static inline uint8_t ps2_read_status( void ) {
    return inb( PS2_PORT_STATUS );
}

static inline void ps2_write_data( uint8_t data ) {
    outb( data, PS2_PORT_DATA );
}

static inline void ps2_write_command( uint8_t command ) {
    outb( command, PS2_PORT_COMMAND );
}

#define PS2_KBD_BUF_SIZE   ( 4 * 1024 )
#define PS2_MOUSE_BUF_SIZE ( 4 * 1024 )

typedef struct ps2_keyboard_cookie {
    int buffer_pos;
    uint8_t buffer[ PS2_KBD_BUF_SIZE ];
    lock_id buffer_sync;
} ps2_keyboard_cookie_t;

typedef struct ps2_mouse_cookie {
    int packet_index;
    uint8_t packet[ 4 ];

    int buffer_pos;
    uint8_t buffer[ PS2_MOUSE_BUF_SIZE ];
    lock_id buffer_sync;
} ps2_mouse_cookie_t;

typedef struct ps2_device {
    int index;
    atomic_t flags;
    lock_id result_semaphore;

    int result_size;
    int result_index;
    uint8_t* result_buffer;

    void* cookie;
    int packet_size;

    int ( *command )( struct ps2_device* device, uint8_t command, uint8_t* output, int out_count,
                      uint8_t* input, int in_count, uint64_t timeout );
    int ( *interrupt )( struct ps2_device* device, uint8_t status, uint8_t data );
} ps2_device_t;

extern int ps2_mux_enabled;
extern ps2_device_t ps2_devices[ PS2_DEV_COUNT ];

/* Controller functions */

void ps2_lock_controller( void );
void ps2_unlock_controller( void );

int ps2_flush( void );
int ps2_wait_read( void );
int ps2_wait_write( void );
int ps2_command( uint8_t command, uint8_t* output, int out_count, uint8_t* input, int in_count );
int ps2_selftest( void );

int ps2_interrupt( int irq, void* _data, registers_t* regs );
int ps2_controller_init( void );

/* Device functions */

int ps2_device_init( void );
int ps2_device_create( ps2_device_t* device );
int ps2_device_interrupt( ps2_device_t* device, uint8_t status, uint8_t data );
int ps2_device_command( ps2_device_t* device, uint8_t command, uint8_t* output,
                        int out_count, uint8_t* inpunt, int in_count );
int ps2_device_command_timeout( ps2_device_t* device, uint8_t command, uint8_t* output,
                                int out_count, uint8_t* inpunt, int in_count, uint64_t timeout );

/* Keyboard functions */

int ps2_keyboard_create( ps2_device_t* device );

/* AUX functions */

int ps2_setup_active_multiplexing( int* enabled );
int ps2_reset_mouse( ps2_device_t* device );
int ps2_detect_mouse( ps2_device_t* device );
int ps2_mouse_create( ps2_device_t* device );

#endif /* _PS2_H_ */
