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

#ifndef _PNGLOADER_HPP_
#define _PNGLOADER_HPP_

#include <png.h>

#include <ygui++/imageloader.hpp>

namespace yguipp {
namespace imageloader {

class PNGLoader : public ImageLoader {
  public:
    PNGLoader( void );
    virtual ~PNGLoader( void );

    bool init( void );

    int addData( uint8_t* data, size_t size, bool final );
    int readData( uint8_t* data, size_t size );
    size_t availableData( void );

  private:
    void infoCallback( png_infop info );
    void rowCallback( png_bytep row, png_uint_32 num, int pass );

    static void pngInfoCallback( png_structp png, png_infop info );
    static void pngRowCallback( png_structp png, png_bytep row, png_uint_32 num, int pass );
    static void pngEndCallback( png_structp png, png_infop info );

  private:
    png_structp m_pngStruct;
    png_infop m_pngInfo;
}; /* class PNGLoader */

class PNGLoaderLibrary : public ImageLoaderLibrary {
  public:
    PNGLoaderLibrary( void );
    virtual ~PNGLoaderLibrary( void );

    bool identify( uint8_t* data, size_t size );
    ImageLoader* create( void );
}; /* class PNGLoaderLibrary */

} /* namespace imageloader */
} /* namespace yguipp */

#endif /* _PNGLOADER_HPP_ */
