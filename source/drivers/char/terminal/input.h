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

#ifndef _TERMINAL_INPUT_H_
#define _TERMINAL_INPUT_H_

typedef enum event_type {
    E_KEY_PRESSED,
    E_KEY_RELEASED,
    E_MOUSE_MOVED,
} event_type_t;

enum {
    KEY_TAB = 7,
    KEY_BACKSPACE = 8,
    KEY_ENTER = 10,
    KEY_ESCAPE = 27,
    KEY_F1 = 0x0100,
    KEY_F2 = 0x0200,
    KEY_F3 = 0x0300,
    KEY_F4 = 0x0400,
    KEY_F5 = 0x0500,
    KEY_F6 = 0x0600,
    KEY_F7 = 0x0700,
    KEY_F8 = 0x0800,
    KEY_F9 = 0x0900,
    KEY_F10 = 0x0A00,
    KEY_F11 = 0x0B00,
    KEY_F12 = 0x0C00,
    KEY_L_CTRL = 0x0D00,
    KEY_R_CTRL = 0x0E00,
    KEY_L_ALT = 0x0F00,
    KEY_R_ALT = 0x1000,
    KEY_CAPSLOCK = 0x1100,
    KEY_L_SHIFT = 0x1200,
    KEY_R_SHIFT = 0x1300,
    KEY_NUMLOCK = 0x1400,
    KEY_SCRLOCK = 0x1500,
    KEY_HOME = 0x1600,
    KEY_PAGE_UP = 0x1700,
    KEY_PAGE_DOWN = 0x1800,
    KEY_INSERT = 0x1900,
    KEY_DELETE = 0x1A00,
    KEY_END = 0x1B00,
    KEY_UP = 0x1C00,
    KEY_DOWN = 0x1D00,
    KEY_LEFT = 0x1E00,
    KEY_RIGHT = 0x1F00
};

typedef int init_input_t( void );
typedef int start_input_t( void );

typedef struct terminal_input {
    const char* name;
    init_input_t* init;
    start_input_t* start;
} terminal_input_t;

extern terminal_input_t ps2_keyboard;

int terminal_handle_event( event_type_t event, int param1, int param2 );

#endif /* _TERMINAL_INPUT_H_ */
