/* Bitmap definitions
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

#ifndef _BITMAP_H_
#define _BITMAP_H_

#include <sys/types.h>
#include <yaosp/region.h>
#include <ygui/yconstants.h>
#include <ygui/protocol.h>
#include <yutil/hashtable.h>

#include <application.h>

typedef int bitmap_id;

enum bitmap_flags {
    BITMAP_FREE_BUFFER = ( 1 << 0 )
};

typedef struct bitmap {
    hashitem_t hash;

    bitmap_id id;
    int ref_count;
    uint32_t width;
    uint32_t height;
    int bytes_per_line;
    color_space_t color_space;
    void* buffer;
    uint32_t flags;
    region_id region;
} bitmap_t;

bitmap_t* create_bitmap( uint32_t width, uint32_t height, color_space_t color_space );
bitmap_t* create_bitmap_from_buffer( uint32_t width, uint32_t height, color_space_t color_space, void* buffer );

bitmap_t* bitmap_get( bitmap_id id );
int bitmap_put( bitmap_t* bitmap );

int handle_create_bitmap( application_t* app, msg_create_bitmap_t* request );
int handle_clone_bitmap( application_t* app, msg_clone_bitmap_t* request );
int handle_delete_bitmap( application_t* app, msg_delete_bitmap_t* request );

int init_bitmap( void );

#endif /* _BITMAP_H_ */
