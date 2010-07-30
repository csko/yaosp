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

#ifndef _COLOR_HPP_
#define _COLOR_HPP_

#include <inttypes.h>

namespace yguipp {

class Color {
  public:
    Color( uint8_t red = 0, uint8_t green = 0, uint8_t blue = 0, uint8_t alpha = 255 ) : m_red(red), m_green(green),
                                                                                         m_blue(blue), m_alpha(alpha) {}

    inline uint32_t toColor32( void ) const {
        return ( ( m_alpha << 24 ) | ( m_red << 16 ) | ( m_green << 8 ) | ( m_blue ) );
    }

    bool operator==( const Color& c );

    static void invertRgb32(uint32_t* p) {
        uint8_t* data = reinterpret_cast<uint8_t*>(p);

        data[0] = 255 - data[0];
        data[1] = 255 - data[1];
        data[2] = 255 - data[2];
    }

  public:
    uint8_t m_red;
    uint8_t m_green;
    uint8_t m_blue;
    uint8_t m_alpha;
}; /* class Color */

} /* namespace yguipp */

#endif /* _COLOR_HPP_ */
