/* yaosp GUI library
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

#include "pngloader.hpp"

namespace yguipp {
namespace imageloader {

PNGLoaderLibrary::PNGLoaderLibrary( void ) {
}

PNGLoaderLibrary::~PNGLoaderLibrary( void ) {
}

bool PNGLoaderLibrary::identify( uint8_t* data, size_t size ) {
    return false;
}

yguipp::ImageLoader* PNGLoaderLibrary::create( void ) {
    return NULL;
}

} /* namespace imageloader */
} /* namespace yguipp */

extern "C"
int loader_get_count( void ) {
    return 1;
}

extern "C"
yguipp::ImageLoaderLibrary* loader_get_at( int index ) {
    return new yguipp::imageloader::PNGLoaderLibrary();
}
