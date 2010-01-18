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

#include <errno.h>

#include "ps2.h"

static int ps2_device_default_command( ps2_device_t* device, uint8_t command, uint8_t* output,
                                       int out_count, uint8_t* input, int in_count, uint64_t timeout );

ps2_device_t ps2_devices[ PS2_DEV_COUNT ] = {
    {
        .flags = ATOMIC_INIT( PS2_FLAG_KEYBOARD ),
        .result_semaphore = -1,
        .command = ps2_device_default_command
    },
    {
        .index = 0,
        .flags = ATOMIC_INIT( 0 ),
        .result_semaphore = -1,
        .command = ps2_device_default_command
    },
    {
        .index = 1,
        .flags = ATOMIC_INIT( 0 ),
        .result_semaphore = -1,
        .command = ps2_device_default_command

    },
    {
        .index = 2,
        .flags = ATOMIC_INIT( 0 ),
        .result_semaphore = -1,
        .command = ps2_device_default_command

    },
    {
        .index = 3,
        .flags = ATOMIC_INIT( 0 ),
        .result_semaphore = -1,
        .command = ps2_device_default_command
    }
};

static int ps2_device_default_command( ps2_device_t* device, uint8_t command, uint8_t* output,
                                       int out_count, uint8_t* input, int in_count, uint64_t timeout ) {
    int i;
    int ret;

    ret = 0;

    device->result_size = in_count;
    device->result_index = 0;
    device->result_buffer = input;

    semaphore_reset( device->result_semaphore );

    for ( i = -1; ret == 0 && i < out_count; i++ ) {
        atomic_and(
            &device->flags,
            ~( PS2_FLAG_ACK | PS2_FLAG_NACK | PS2_FLAG_GET_ID )
        );

        ps2_lock_controller();

        if ( ( atomic_get( &device->flags ) & PS2_FLAG_KEYBOARD ) == 0 ) {
            uint8_t prefix_cmd;

            if ( ps2_mux_enabled ) {
                prefix_cmd = 0x90 + device->index;
            } else {
                prefix_cmd = 0xd4;
            }

            ret = ps2_wait_write();

            if ( ret == 0 ) {
                ps2_write_command( prefix_cmd );
            }
        }

        ret = ps2_wait_write();

        if ( ret == 0 ) {
            if ( i == -1 ) {
                if ( command == PS2_CMD_GET_DEVICE_ID) {
                    atomic_or( &device->flags, PS2_FLAG_CMD | PS2_FLAG_GET_ID );
                } else {
                    atomic_or( &device->flags, PS2_FLAG_CMD );
                }

                ps2_write_data( command );
            } else {
                ps2_write_data( output[ i ] );
            }
        }

        ps2_unlock_controller();

        ret = semaphore_timedlock( device->result_semaphore, 1, LOCK_IGNORE_SIGNAL, timeout );

        if ( ret != 0 ) {
            atomic_and( &device->flags, ~PS2_FLAG_CMD );
        }

        if ( atomic_get( &device->flags ) & PS2_FLAG_NACK ) {
            atomic_and( &device->flags, ~PS2_FLAG_CMD );
            ret = -EIO;
        }

        if ( ret != 0 ) {
            break;
        }
    }

    if ( ret == 0 ) {
        if ( in_count == 0 ) {
            atomic_and( &device->flags, ~PS2_FLAG_CMD );
        } else {
            ret = semaphore_timedlock( device->result_semaphore, 1, LOCK_IGNORE_SIGNAL, timeout );

            atomic_and( &device->flags, ~PS2_FLAG_CMD );

            if ( device->result_size != 0 ) {
                in_count -= device->result_size;
                device->result_size = 0;
                ret = -EIO;
            }
        }
    }

    return ret;
}

int ps2_device_init( void ) {
    int i;

    for ( i = 0; i < PS2_DEV_COUNT; i++ ) {
        ps2_devices[ i ].result_semaphore = semaphore_create( "PS/2 command completion", 0 );
    }

    return 0;
}

int ps2_device_create( ps2_device_t* device ) {
    int error;

    if ( atomic_get( &device->flags ) & PS2_FLAG_KEYBOARD ) {
        error = ps2_keyboard_create( device );
    } else {
        error = ps2_detect_mouse( device );

        if ( error == 0 ) {
            error = ps2_mouse_create( device );
        }
    }

    return error;
}

int ps2_device_interrupt( ps2_device_t* device, uint8_t status, uint8_t data ) {
    uint32_t flags;

    flags = atomic_get( &device->flags );

    if ( flags & PS2_FLAG_CMD ) {
        if ( ( flags & ( PS2_FLAG_ACK | PS2_FLAG_NACK ) ) == 0 ) {
            int cnt = 1;

            if ( data == PS2_REPLY_ACK ) {
                atomic_or( &device->flags, PS2_FLAG_ACK );
            } else if ( data == PS2_REPLY_RESEND || data == PS2_REPLY_ERROR ) {
                atomic_or( &device->flags, PS2_FLAG_NACK );
            } else if ( ( flags & PS2_FLAG_GET_ID ) && (data == 0 || data == 3 || data == 4)) {
                // workaround for broken mice that don't ack the "get id" command

                atomic_or( &device->flags, PS2_FLAG_ACK );

                if ( device->result_size > 0 ) {
                    device->result_buffer[ device->result_index++ ] = data;

                    if ( --device->result_size == 0 ) {
                        atomic_and( &device->flags, ~PS2_FLAG_CMD );
                        cnt++;
                    }
                }
            } else {
                goto pass_to_handler;
            }

            semaphore_unlock( device->result_semaphore, cnt );

            return 0;
        } else if ( device->result_size > 0 ) {
            device->result_buffer[ device->result_index++ ] = data;

            if ( --device->result_size == 0 ) {
                atomic_and( &device->flags, ~PS2_FLAG_CMD );
                semaphore_unlock( device->result_semaphore, 1 );
                return 0;
            }
        } else {
            goto pass_to_handler;
        }

        return 0;
    }

 pass_to_handler:
    /* If the device is not enabled, simply discard the interrupt. */

    if ( ( flags & PS2_FLAG_ENABLED ) ==  0) {
        return 0;
    }

    return device->interrupt( device, status, data );
}

int ps2_device_command( ps2_device_t* device, uint8_t command, uint8_t* output,
                        int out_count, uint8_t* input, int in_count ) {
    return ps2_device_command_timeout( device, command, output, out_count,
                                       input, in_count, 4 * 1000 * 1000 );
}

int ps2_device_command_timeout( ps2_device_t* device, uint8_t command, uint8_t* output,
                                int out_count, uint8_t* input, int in_count, uint64_t timeout ) {
    return device->command( device, command, output, out_count, input, in_count, timeout );
}
