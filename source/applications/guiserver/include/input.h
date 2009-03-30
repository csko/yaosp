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

#ifndef _GUISERVER_INPUT_H_
#define _GUISERVER_INPUT_H_

#define INIT_FREE_EVENT_COUNT 64
#define MAX_FREE_EVENT_COUNT  64

typedef enum input_event_type {
    MOUSE_MOVED
} input_event_type_t;

typedef struct input_event {
    input_event_type_t type;
    int param1;
    int param2;
    struct input_event* next;
} input_event_t;

typedef struct input_driver {
    const char* name;
    int ( *init )( void );
    int ( *start )( void );
} input_driver_t;

extern input_driver_t ps2mouse_driver;

input_event_t* get_input_event( input_event_type_t type, int param1, int param2 );
int put_input_event( input_event_t* event );

int insert_input_event( input_event_t* event );

int init_input_system( void );

#endif /* _GUISERVER_INPUT_H_ */

