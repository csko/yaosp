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
#include <ygui/color.h>

namespace yguipp {

class Color {
  public:
    Color( uint8_t red = 0, uint8_t green = 0, uint8_t blue = 0, uint8_t alpha = 255 );

    void toColorT( color_t* c ) const;

    bool operator==( const Color& c );

  private:
    uint8_t m_red;
    uint8_t m_green;
    uint8_t m_blue;
    uint8_t m_alpha;
}; /* class Color */

} /* namespace yguipp */

#endif /* _COLOR_HPP_ */
