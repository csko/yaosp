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

#ifndef _YGUI_IMAGE_IMAGELOADER_H_
#define _YGUI_IMAGE_IMAGELOADER_H_

#include <sys/types.h>

#include <ygui/yconstants.h>

typedef int imgldr_identify_t( uint8_t* data, size_t size );
typedef int imgldr_create_t( void** private );
typedef int imgldr_destroy_t( void* private );
typedef int imgldr_add_data_t( void* private, uint8_t* data, size_t size, int finalize );
typedef int imgldr_get_available_size_t( void* private );
typedef int imgldr_read_data_t( void* private, uint8_t* data, size_t size );

typedef struct image_info {
    int width;
    int height;
    color_space_t color_space;
} image_info_t;

typedef struct image_loader {
    const char* name;
    imgldr_identify_t* identify;

    imgldr_create_t* create;
    imgldr_destroy_t* destroy;

    imgldr_add_data_t* add_data;
    imgldr_get_available_size_t* get_available_size;
    imgldr_read_data_t* read_data;
} image_loader_t;

int image_loader_find( uint8_t* data, size_t size, image_loader_t** loader, void** private );

#endif /* _YGUI_IMAGE_IMAGELOADER_H_ */
