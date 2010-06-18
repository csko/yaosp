/* Image viewer application
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

#ifndef _WORKER_H_
#define _WORKER_H_

#include <ygui/bitmap.h>
#include <ygui/point.h>

typedef enum {
    OPEN_IMAGE,
    RESIZE_IMAGE,
    WORK_COUNT
} work_type_t;

typedef struct work_header {
    work_type_t type;

    int ( *done )( struct work_header* );
    int ( *failed )( struct work_header* );
} work_header_t;

typedef struct {
    work_header_t header;

    char* path;
    bitmap_t* bitmap;
} open_work_t;

typedef struct {
    work_header_t header;

    point_t size;
    bitmap_t* input;
    bitmap_t* output;
} resize_work_t;

typedef int worker_function_t( work_header_t* work );

int worker_put( work_header_t* work );

int worker_init( void );
int worker_start( void );
int worker_shutdown( void );

#endif /* _WORKER_H_ */
