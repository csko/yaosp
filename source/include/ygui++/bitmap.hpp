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

#ifndef _BITMAP_HPP_
#define _BITMAP_HPP_

#include <string>
#include <yaosp/region.h>

#include <ygui++/yconstants.hpp>
#include <ygui++/object.hpp>
#include <ygui++/rect.hpp>

namespace yguipp {

class Bitmap : public Object {
  public:
    Bitmap( const Point& size, ColorSpace colorSpace = CS_RGB32 );
    virtual ~Bitmap( void );

    bool init( void );

    Rect bounds(void);
    int getHandle( void );
    uint8_t* getData( void );
    const Point& getSize( void );

    static Bitmap* loadFromFile( const std::string& path );

  private:
    int m_handle;
    uint8_t* m_data;
    region_id m_region;

    Point m_size;
    ColorSpace m_colorSpace;

    static const int LOAD_BUFFER_SIZE = 32768;
}; /* class Bitmap */

} /* namespace yguipp */

#endif /* _BITMAP_HPP_ */
