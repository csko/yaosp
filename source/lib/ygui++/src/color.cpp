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

#include <ygui++/color.hpp>

namespace yguipp {

Color::Color( uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha ) : m_red(red), m_green(green),
                                                                          m_blue(blue), m_alpha(alpha) {
}

void Color::toColorT( color_t* c ) const {
    c->red = m_red;
    c->green = m_green;
    c->blue = m_blue;
    c->alpha = m_alpha;
}

bool Color::operator==( const Color& c ) {
    return ( ( m_red == c.m_red ) &&
             ( m_green == c.m_green ) &&
             ( m_blue == c.m_blue ) &&
             ( m_alpha == c.m_alpha ) );
}

} /* namespace yguipp */
