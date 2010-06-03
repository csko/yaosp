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

#ifndef _IMAGELOADER_HPP_
#define _IMAGELOADER_HPP_

#include <vector>
#include <inttypes.h>

#include <ygui++/yconstants.hpp>

namespace yguipp {

struct ImageInfo {
    int m_width;
    int m_height;
    ColorSpace m_colorSpace;
};

class ImageLoader {
  public:
    virtual ~ImageLoader( void ) {}

    virtual int addData( uint8_t* data, size_t size, bool final ) = 0;
    virtual int readData( uint8_t* data, size_t size ) = 0;
    virtual size_t availableData( void ) = 0;
}; /* class ImageLoader */

class ImageLoaderLibrary {
  public:
    virtual ~ImageLoaderLibrary( void ) {}

    virtual bool identify( uint8_t* data, size_t size ) = 0;
    virtual ImageLoader* create( void ) = 0;
}; /* class ImageLoaderLibrary */

class ImageLoaderManager {
  private:
    ImageLoaderManager( void );
    ~ImageLoaderManager( void );

  public:
    static void createInstance( void );
    static ImageLoaderManager* getInstance( void );

    ImageLoader* getLoader( uint8_t* data, size_t size );

    bool loadLibraries( void );

  private:
    typedef int loader_get_count_t( void );
    typedef ImageLoaderLibrary* loader_get_at_t( int index );

    std::vector<ImageLoaderLibrary*> m_libraries;

    static ImageLoaderManager* m_instance;
}; /* class ImageLoader */

} /* namespace yguipp */

#endif /* _IMAGELOADER_HPP_ */
