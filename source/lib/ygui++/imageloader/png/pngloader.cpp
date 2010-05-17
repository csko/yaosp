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

PNGLoader::PNGLoader( void ) : m_pngStruct(NULL), m_pngInfo(NULL) {
}

PNGLoader::~PNGLoader( void ) {
}

bool PNGLoader::init( void ) {
    m_pngStruct = png_create_read_struct( PNG_LIBPNG_VER_STRING, NULL, NULL, NULL );

    if ( m_pngStruct == NULL ) {
        return false;
    }

    m_pngInfo = png_create_info_struct(m_pngStruct);

    if ( m_pngInfo == NULL ) {
        png_destroy_read_struct( &m_pngStruct, NULL, NULL );
        return false;
    }

    if ( setjmp( png_jmpbuf(m_pngStruct) ) ) {
        png_destroy_read_struct( &m_pngStruct, NULL, NULL );
        /* todo */
        return false;
    }

    png_set_progressive_read_fn(
        m_pngStruct, reinterpret_cast<void*>(this),
        pngInfoCallback, pngRowCallback, pngEndCallback
    );

    return true;
}

int PNGLoader::addData( uint8_t* data, size_t size, bool final ) {
    if ( setjmp( png_jmpbuf(m_pngStruct) ) ) {
        return -1;
    }

    png_process_data(
        m_pngStruct, m_pngInfo,
        reinterpret_cast<png_bytep>(data), size
    );

    return 0;
}

int PNGLoader::readData( uint8_t* data, size_t size ) {
    return 0;
}

size_t PNGLoader::availableData( void ) {
    return 0;
}

void PNGLoader::infoCallback( png_infop info ) {
    int depth;
    int colorType;
    int interlaceType;
    png_uint_32 width;
    png_uint_32 height;
    double imageGamma;

    png_get_IHDR(
        m_pngStruct, m_pngInfo,
        &width, &height, &depth,
        &colorType, &interlaceType,
        NULL, NULL
    );

    if ( ( colorType == PNG_COLOR_TYPE_PALETTE ) ||
         ( png_get_valid( m_pngStruct, m_pngInfo, PNG_INFO_tRNS ) ) ) {
        png_set_expand(m_pngStruct);
    }

    if ( png_get_gAMA( m_pngStruct, m_pngInfo, &imageGamma ) ) {
        png_set_gamma( m_pngStruct, 2.2, imageGamma );
    } else {
        png_set_gamma( m_pngStruct, 2.2, 0.45 );
    }

    png_set_bgr( m_pngStruct );
    png_set_filler( m_pngStruct, 0xFF, PNG_FILLER_AFTER );
    png_set_gray_to_rgb( m_pngStruct );
    png_set_interlace_handling( m_pngStruct );
    png_read_update_info( m_pngStruct, m_pngInfo );
}

void PNGLoader::rowCallback( png_bytep row, png_uint_32 num, int pass ) {
}

void PNGLoader::pngInfoCallback( png_structp png, png_infop info ) {
    PNGLoader* loader;

    loader = reinterpret_cast<PNGLoader*>( png_get_progressive_ptr(png) );
    loader->infoCallback(info);
}

void PNGLoader::pngRowCallback( png_structp png, png_bytep row, png_uint_32 num, int pass ) {
    PNGLoader* loader;

    loader = reinterpret_cast<PNGLoader*>( png_get_progressive_ptr(png) );
    loader->rowCallback(row,num,pass);
}

void PNGLoader::pngEndCallback( png_structp png, png_infop info ) {
}

PNGLoaderLibrary::PNGLoaderLibrary( void ) {
}

PNGLoaderLibrary::~PNGLoaderLibrary( void ) {
}

bool PNGLoaderLibrary::identify( uint8_t* data, size_t size ) {
    return ( ( size >= 8 ) &&
             ( png_check_sig( reinterpret_cast<png_bytep>(data), 8 ) != 0 ) );
}

yguipp::ImageLoader* PNGLoaderLibrary::create( void ) {
    PNGLoader* loader = new PNGLoader();

    if ( !loader->init() ) {
        delete loader;
        return NULL;
    }

    return loader;
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
