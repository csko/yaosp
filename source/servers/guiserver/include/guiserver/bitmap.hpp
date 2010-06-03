/* GUI server
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

#include <ygui++/rect.hpp>
#include <ygui++/yconstants.hpp>

class Bitmap {
  public:
    Bitmap( uint32_t width, uint32_t height, ColorSpace colorSpace, uint8_t* buffer = NULL );
    ~Bitmap( void );

    inline uint32_t width( void ) { return m_width; }
    inline uint32_t height( void ) { return m_height; }

    yguipp::Point size( void );
    yguipp::Rect bounds( void );

    uint8_t* getBuffer( void );
    ColorSpace getColorSpace( void );

  private:
    enum {
        FREE_BUFFER = (1<<0)
    };

  private:
    uint32_t m_width;
    uint32_t m_height;
    ColorSpace m_colorSpace;
    uint8_t* m_buffer;
    uint32_t m_flags;
}; /* class Bitmap */

#endif /* _BITMAP_HPP_ */
