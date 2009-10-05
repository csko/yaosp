/* Image loader handling
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

#include <errno.h>

#include <ygui/image/imageloader.h>

extern image_loader_t png_loader;

static image_loader_t* image_loaders[] = {
    &png_loader,
    NULL
};

int image_loader_find( uint8_t* data, size_t size, image_loader_t** _loader, void** _private ) {
    int i;
    image_loader_t* loader;

    if ( ( _loader == NULL ) ||
         ( _private == NULL ) ) {
        return -EINVAL;
    }

    for ( i = 0; image_loaders[ i ] != NULL; i++ ) {
        loader = image_loaders[ i ];

        if ( loader->identify( data, size ) == 0 ) {
            goto found;
        }
    }

    return -1;

 found:
    *_loader = loader;

    return loader->create( _private );
}
