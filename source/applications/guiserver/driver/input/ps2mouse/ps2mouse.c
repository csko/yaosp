/* GUI server
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

#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <yaosp/debug.h>
#include <yaosp/thread.h>

#include <input.h>

#define PS2_AUX_SET_RES     0xE8
#define PS2_AUX_SET_SCALE11 0xE6
#define PS2_AUX_SET_SCALE21 0xE7
#define PS2_AUX_GET_SCALE   0xE9
#define PS2_AUX_SET_STREAM  0xEA
#define PS2_AUX_SET_SAMPLE  0xF3
#define PS2_AUX_ENABLE_DEV  0xF4
#define PS2_AUX_DISABLE_DEV 0xF5
#define PS2_AUX_RESET       0xFF
#define PS2_AUX_ACK         0xFA
#define PS2_AUX_SEND_ID     0xF2

#define PS2_AUX_ID_ERROR -1
#define PS2_AUX_ID_PS2   0
#define PS2_AUX_ID_IMPS2 3

static int mouse_device = -1;
static thread_id mouse_thread = -1;

static input_driver_t* input_drivers[] = {
    &ps2mouse_driver,
    NULL
};

static uint8_t basic_init[] = {
    PS2_AUX_ENABLE_DEV, PS2_AUX_SET_SAMPLE, 100
};

static uint8_t imps2_init[] = {
    PS2_AUX_SET_SAMPLE, 200, PS2_AUX_SET_SAMPLE, 100, PS2_AUX_SET_SAMPLE, 80
};

static uint8_t ps2_init[] = {
    PS2_AUX_SET_SCALE11, PS2_AUX_ENABLE_DEV, PS2_AUX_SET_SAMPLE, 100, PS2_AUX_SET_RES, 3
};

static int ps2mouse_write( uint8_t* data, size_t size ) {
    size_t i;
    int error;
    uint8_t ack;

    error = 0;

    for ( i = 0; i < size; i++, data++ ) {
        write( mouse_device, data, 1 );

        if ( ( read( mouse_device, &ack, 1 ) != 1 ) ||
             ( ack != PS2_AUX_ACK ) ) {
            error++;
        }
    }

    return error;
}

static int ps2mouse_read_id( void ) {
    int error;
    uint8_t data;

    data = PS2_AUX_SEND_ID;

    error = write( mouse_device, &data, 1 );

    if ( error < 0 ) {
        return PS2_AUX_ID_ERROR;
    }

    if ( ( read( mouse_device, &data, 1 ) != 1 ) ||
         ( data != PS2_AUX_ACK ) ) {
        return PS2_AUX_ID_ERROR;
    }

    if ( read( mouse_device, &data, 1 ) != 1 ) {
        return PS2_AUX_ID_ERROR;
    }

    return data;
}

static int ps2mouse_init( void ) {
    int id;
    int error;

    mouse_device = open( "/device/input/ps2mouse", O_RDONLY );

    if ( mouse_device < 0 ) {
        return mouse_device;
    }

    ps2mouse_write( basic_init, sizeof( basic_init ) );

    error = ps2mouse_write( basic_init, sizeof( basic_init ) );

    if ( error < 0 ) {
        return error;
    }

    error = ps2mouse_write( imps2_init, sizeof( imps2_init ) );

    if ( error < 0 ) {
        return error;
    }

    id = ps2mouse_read_id();

    if ( id == PS2_AUX_ID_ERROR ) {
        return -EINVAL;
    }

    error = ps2mouse_write( ps2_init, sizeof( ps2_init ) );

    if ( error < 0 ) {
        return error;
    }

    return 0;
}

static int ps2mouse_thread( void* arg ) {
#if 0
    uint8_t data;

    while ( 1 ) {
        read( mouse_device, &data, 1 );
    }
#endif

    return 0;
}

static int ps2mouse_start( void ) {
    int error;

    mouse_thread = create_thread(
        "ps2_input",
        PRIORITY_DISPLAY,
        ps2mouse_thread,
        NULL,
        0
    );

    if ( mouse_thread < 0 ) {
        return mouse_thread;
    }

    error = wake_up_thread( mouse_thread );

    if ( error < 0 ) {
        return error;
    }

    return 0;
}

input_driver_t ps2mouse_driver = {
    .name = "PS/2 mouse",
    .init = ps2mouse_init,
    .start = ps2mouse_start
};

