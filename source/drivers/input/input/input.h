/* Input driver
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

#ifndef _INPUT_INPUT_H_
#define _INPUT_INPUT_H_

#include <types.h>
#include <input.h>
#include <semaphore.h>

#define INPUT_DRV_COUNT 1

typedef enum input_driver_type {
    T_KEYBOARD,
    T_MOUSE,
    T_JOYSTICK
} input_driver_type_t;

enum {
    INPUT_KEY_EVENTS = ( 1 << 0 ),
    INPUT_MOUSE_EVENTS = ( 1 << 1 )
};

typedef struct input_state {
    void* driver_states[ INPUT_DRV_COUNT ];
} input_state_t;

typedef struct input_event_wrapper {
    input_event_t e;
    struct input_event_wrapper* next;
} input_event_wrapper_t;

typedef struct input_device {
    int flags;

    semaphore_id lock;
    semaphore_id sync;

    input_state_t input_state;

    input_event_wrapper_t* first_event;
    input_event_wrapper_t* last_event;
} input_device_t;

typedef struct input_driver {
    const char* name;
    input_driver_type_t type;
    bool initialized;

    int ( *init )( void );
    void ( *destroy )( void );
    int ( *start )( void );
    int ( *create_state )( void** state );
    void ( *destroy_state )( void* state );
    int ( *set_state )( void* state );
} input_driver_t;

/* Input driver state management */

int set_input_driver_states( input_state_t* state );
int init_input_driver_states( input_state_t* state );

/* Input device node handling */

input_device_t* create_input_device( int flags );
void destroy_input_device( input_device_t* device );

int insert_input_device( input_device_t* device );

int insert_input_event( input_event_t* event );

/* Init functions */

int init_node_manager( void );
int init_input_controller( void );
int init_input_drivers( void );

#endif /* _INPUT_H_ */
