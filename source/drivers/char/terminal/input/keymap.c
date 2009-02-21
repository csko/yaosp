/* Terminal driver
 *
 * Copyright (c) 2009 Zoltan Kovacs, Kornel Csernai
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

#include <types.h>

#include "../input.h"

#define CTRL(c) ((c)&0x1F)

uint16_t keyboard_normal_map[ 128 ] = {
    /* 0-9 */ 0, KEY_ESCAPE, '1', '2', '3', '4', '5', '6', '7', '8',
    /* 10-19 */ '9', '0', '-', '=', KEY_BACKSPACE, KEY_TAB, 'q', 'w', 'e', 'r',
    /* 20-29 */ 't', 'y', 'u', 'i', 'o', 'p', '[', ']', KEY_ENTER, KEY_L_CTRL,
    /* 30-39 */ 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',
    /* 40-49 */ '\'', '`', KEY_L_SHIFT, '\\', 'z', 'x', 'c', 'v', 'b', 'n',
    /* 50-59 */ 'm', ',', '.', '/', KEY_R_SHIFT, '*', KEY_L_ALT, ' ', KEY_CAPSLOCK, KEY_F1,
    /* 60-69 */ KEY_F2, KEY_F3, KEY_F4, KEY_F5, KEY_F6, KEY_F7, KEY_F8, KEY_F9, KEY_F10, KEY_NUMLOCK,
    /* 70-79 */ KEY_SCRLOCK, KEY_HOME, KEY_UP, KEY_PAGE_UP, '-', KEY_LEFT, 0, KEY_RIGHT, '+', KEY_END,
    /* 80-89 */ KEY_DOWN, KEY_PAGE_DOWN, KEY_INSERT, KEY_DELETE, 0, 0, 0, KEY_F11, KEY_F12, 0,
    /* 90-99 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    /* 100-109 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    /* 110-119 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    /* 120-127 */ 0, 0, 0, 0, 0, 0, 0, 0
};

uint16_t keyboard_shifted_map[ 128 ] = {
    /* 0-9 */ 0, KEY_ESCAPE, '!', '@', '#', '$', '%', '^', '&', '*',
    /* 10-19 */ '(', ')', '_', '+', KEY_BACKSPACE, KEY_TAB, 'Q', 'W', 'E', 'R',
    /* 20-29 */ 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', KEY_ENTER, KEY_L_CTRL,
    /* 30-39 */ 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':',
    /* 40-49 */ '"', '~', KEY_L_SHIFT, '|', 'Z', 'X', 'C', 'V', 'B', 'N',
    /* 50-59 */ 'M', '<', '>', '?', KEY_R_SHIFT, '*', KEY_L_ALT, ' ', KEY_CAPSLOCK, KEY_F1,
    /* 60-69 */ KEY_F2, KEY_F3, KEY_F4, KEY_F5, KEY_F6, KEY_F7, KEY_F8, KEY_F9, KEY_F10, KEY_NUMLOCK,
    /* 70-79 */ KEY_SCRLOCK, '7', '8', '9', '-', '4', '5', '6', '+', '1',
    /* 80-89 */ '2', '3', '0', '.', 0, 0, '|', KEY_F11, KEY_F12, 0,
    /* 90-99 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    /* 100-109 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    /* 110-119 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    /* 120-127 */ 0, 0, 0, 0, 0, 0, 0, 0
};

uint16_t keyboard_ctrl_map[ 128 ] = {
    /* 0-9 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    /* 10-19 */ 0, 0, 0, 0, 0, 0, CTRL('q'), CTRL('w'), CTRL('e'), CTRL('r'),
    /* 20-29 */ CTRL('t'), CTRL('y'), CTRL('u'), CTRL('i'), CTRL('o'), CTRL('p'), 0, 0, 0, KEY_L_CTRL,
    /* 30-39 */ CTRL('a'), CTRL('s'), CTRL('d'), CTRL('f'), CTRL('g'), CTRL('h'), CTRL('j'), CTRL('k'), CTRL('l'), 0,
    /* 40-49 */ 0, 0, 0, 0, CTRL('z'), CTRL('x'), CTRL('c'), CTRL('v'), CTRL('b'), CTRL('n'),
    /* 50-59 */ CTRL('m'), 0, 0, 0, 0, 0 /* Ctrl+PrintScr */, 0, 0, 0, 0,
    /* 60-69 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    /* 70-79 */ 0 /* Ctrl+Break */, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    /* 80-89 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    /* 90-99 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    /* 100-109 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    /* 110-119 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    /* 120-127 */ 0, 0, 0, 0, 0, 0, 0, 0
};

uint16_t keyboard_escaped_map[ 128 ] = {
    /* 0-9 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    /* 10-19 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    /* 20-29 */ 0, 0, 0, 0, 0, 0, 0, 0, KEY_ENTER, KEY_R_CTRL,
    /* 30-39 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    /* 40-49 */ 0, 0, KEY_L_SHIFT, 0, 0, 0, 0, 0, 0, 0,
    /* 50-59 */ 0, 0, 0, '/', KEY_R_SHIFT, 0 /* Ctrl+PrintScr */, KEY_R_ALT, 0, 0, 0,
    /* 60-69 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    /* 70-79 */ 0 /* Ctrl+Break */, KEY_HOME, KEY_UP, KEY_PAGE_UP, 0, KEY_LEFT, 0, KEY_RIGHT, 0, KEY_END,
    /* 80-89 */ KEY_DOWN, KEY_PAGE_DOWN, KEY_INSERT, KEY_DELETE, 0, 0, 0, 0, 0, 0,
    /* 90-99 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    /* 100-109 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    /* 110-119 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    /* 120-127 */ 0, 0, 0, 0, 0, 0, 0, 0
};
