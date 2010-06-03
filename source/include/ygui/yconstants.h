/* yaosp GUI library
 *
 * Copyright (c) 2009, 2010 Zoltan Kovacs
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

#ifndef _YAOSP_YCONSTANTS_H_
#define _YAOSP_YCONSTANTS_H_


typedef enum scrollbar_policy {
    SCROLLBAR_NEVER,
    SCROLLBAR_AUTO,
    SCROLLBAR_ALWAYS
} scrollbar_policy_t;

typedef enum color_space {
    CS_UNKNOWN,
    CS_RGB16,
    CS_RGB24,
    CS_RGB32
} color_space_t;

typedef enum drawing_mode {
    DM_COPY,
    DM_BLEND,
    DM_INVERT
} drawing_mode_t;

typedef struct screen_mode_info {
    int width;
    int height;
    color_space_t color_space;
} screen_mode_info_t;

#endif /* _YAOSP_YCONSTANTS_H_ */
