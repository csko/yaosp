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

#include <algorithm>

#include <ygui++/rect.hpp>

namespace yguipp {

Rect::Rect( const Point& size ) : m_left(0), m_top(0),
                                  m_right(size.m_x - 1), m_bottom(size.m_y - 1 ) {
}

Rect::Rect( int left, int top, int right, int bottom ) : m_left(left), m_top(top),
                                                         m_right(right), m_bottom(bottom) {
}

int Rect::width( void ) const {
    return ( m_right - m_left + 1 );
}

int Rect::height( void ) const {
    return ( m_bottom - m_top + 1 );
}

Point Rect::bounds( void ) const {
    return Point( m_right - m_left + 1, m_bottom - m_top + 1 );
}

Point Rect::leftTop( void ) const {
    return Point(m_left, m_top);
}

bool Rect::isValid( void ) const {
    return ( ( m_left <= m_right ) &&
             ( m_top <= m_bottom ) );
}

bool Rect::hasPoint( const Point& p ) const {
    return ( ( m_left <= p.m_x ) &&
             ( p.m_x <= m_right ) &&
             ( m_top <= p.m_y ) &&
             ( p.m_y <= m_bottom ) );
}

void Rect::toRectT( rect_t* r ) const {
    r->left = m_left;
    r->top = m_top;
    r->right = m_right;
    r->bottom = m_bottom;
}

Rect Rect::operator+( const Point& p ) const {
    return Rect( m_left + p.m_x, m_top + p.m_y,
                 m_right + p.m_x, m_bottom + p.m_y );
}

Rect Rect::operator&( const Rect& r ) const {
    return Rect( std::max(m_left, r.m_left), std::max(m_top, r.m_top),
                 std::min(m_right, r.m_right), std::min(m_bottom, r.m_bottom) );
}

Rect& Rect::operator=( const Point& p ) {
    m_left = 0;
    m_top = 0;
    m_right = p.m_x - 1;
    m_bottom = p.m_y - 1;

    return *this;
}

Rect& Rect::operator+=( const Point& p ) {
    m_left += p.m_x;
    m_top += p.m_y;
    m_right += p.m_x;
    m_bottom += p.m_y;

    return *this;
}

Rect& Rect::operator&=( const Rect& r ) {
    m_left = std::max(m_left, r.m_left);
    m_top = std::max(m_top, r.m_top);
    m_right = std::min(m_right, r.m_right);
    m_bottom = std::min(m_bottom, r.m_bottom );

    return *this;
}

} /* namespace yguipp */
