/* Input definitions
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

#ifndef _INPUT_H_
#define _INPUT_H_

#include <types.h>

enum {
    INPUT_KEY_EVENTS = ( 1 << 0 ),
    INPUT_MOUSE_EVENTS = ( 1 << 1 )
};

typedef enum input_event_type {
    E_KEY_PRESSED,
    E_KEY_RELEASED,
    E_MOUSE_PRESSED,
    E_MOUSE_RELEASED,
    E_MOUSE_MOVED,
    E_MOUSE_SCROLLED,
    E_QUALIFIERS_CHANGED
} input_event_type_t;

typedef struct input_event {
    input_event_type_t event;

    int param1;
    int param2;
} input_event_t;

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

enum {
    Q_LEFT_SHIFT = ( 1 << 0 ),
    Q_RIGHT_SHIFT = ( 1 << 1 ),
    Q_LEFT_CTRL = ( 1 << 2 ),
    Q_RIGHT_CTRL = ( 1 << 3 ),
    Q_LEFT_ALT = ( 1 << 4 ),
    Q_RIGHT_ALT = ( 1 << 5),
    Q_CAPSLOCK = ( 1 << 6 ),
    Q_SCRLOCK = ( 1 << 7 ),
    Q_NUMLOCK = ( 1 << 8 )
};

#define Q_SHIFT ( Q_LEFT_SHIFT | Q_RIGHT_SHIFT )
#define Q_CTRL  ( Q_LEFT_CTRL | Q_RIGHT_CTRL )
#define Q_ALT   ( Q_LEFT_ALT | Q_RIGHT_ALT )

enum {
    MOUSE_BTN_LEFT = 1,
    MOUSE_BTN_RIGHT,
    MOUSE_BTN_CENTER
};

typedef struct input_cmd_create_node {
    int flags;
    uint32_t node_number;
} input_cmd_create_node_t;

#endif /* _INPUT_H_ */
