/* Image loader definitions
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

#ifndef _IMGLOADER_H_
#define _IMGLOADER_H_

#include <bitmap.h>
#include <inttypes.h>

typedef struct image_loader {
    int ( *identify )( int fd );
    bitmap_t* ( *load )( int fd );
} image_loader_t;

extern image_loader_t png_loader;

#endif /* _IMGLOADER_H_ */
