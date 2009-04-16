/* yaosp GUI library
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

#ifndef _YAOSP_RENDER_H_
#define _YAOSP_RENDER_H_

#include <sys/types.h>

#include <ygui/rect.h>
#include <ygui/color.h>

#define DEFAULT_RENDER_BUFFER_SIZE 8192

enum {
    R_SET_PEN_COLOR = 1,
    R_FILL_RECT
};

typedef struct render_header {
    uint8_t command;
} __attribute__(( packed )) render_header_t;

typedef struct r_set_pen_color {
    render_header_t header;
    color_t color;
} __attribute__(( packed )) r_set_pen_color_t;

typedef struct r_fill_rect {
    render_header_t header;
    rect_t rect;
} __attribute__(( packed )) r_fill_rect_t;

#endif /* _YAOSP_RENDER_H_ */
