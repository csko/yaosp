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

#ifndef _RECT_HPP_
#define _RECT_HPP_

#include <algorithm>
#include <ygui/rect.h>

#include <ygui++/point.hpp>

namespace yguipp {

class Rect {
  public:
    Rect( const Point& size ) : m_left(0), m_top(0), m_right(size.m_x - 1), m_bottom(size.m_y - 1) {}
    Rect( rect_t* r ) : m_left(r->left), m_top(r->top), m_right(r->right), m_bottom(r->bottom) {}
    Rect( int left = 0, int top = 0, int right = 0, int bottom = 0 ) : m_left(left), m_top(top), m_right(right), m_bottom(bottom) {}

    inline int width( void ) const { return ( m_right - m_left + 1 ); }
    inline int height( void ) const { return ( m_bottom - m_top + 1 ); }

    Point size( void ) const { return Point(m_right - m_left + 1, m_bottom - m_top + 1); }
    Point leftTop( void ) const { return Point(m_left, m_top); }
    Rect bounds( void ) const { return Rect(0, 0, width() - 1, height() - 1); }

    bool isValid( void ) const { return ( ( m_left <= m_right ) && ( m_top <= m_bottom ) ); }
    bool hasPoint( const Point& p ) const {
        return ( ( m_left <= p.m_x ) &&
                 ( p.m_x <= m_right ) &&
                 ( m_top <= p.m_y ) &&
                 ( p.m_y <= m_bottom ) );
    }
    bool doIntersect( const Rect& r ) const {
        return !( r.m_right < m_left || r.m_left > m_right || r.m_bottom < m_top || r.m_top > m_bottom );
    }

    void toRectT( rect_t* r ) const {
        r->left = m_left;
        r->top = m_top;
        r->right = m_right;
        r->bottom = m_bottom;
    }

    Rect operator+( const Point& p ) const {
        return Rect( m_left + p.m_x, m_top + p.m_y,
                     m_right + p.m_x, m_bottom + p.m_y );
    }

    Rect operator-( const Point& p ) const {
        return Rect( m_left - p.m_x, m_top - p.m_y,
                     m_right - p.m_x, m_bottom - p.m_y );
    }

    Rect operator&( const Rect& r ) const {
        return Rect( std::max(m_left, r.m_left), std::max(m_top, r.m_top),
                     std::min(m_right, r.m_right), std::min(m_bottom, r.m_bottom) );
    }

    Rect& operator=( const Point& p ) {
        m_left = 0;
        m_top = 0;
        m_right = p.m_x - 1;
        m_bottom = p.m_y - 1;

        return *this;
    }

    Rect& operator+=( const Point& p ) {
        m_left += p.m_x;
        m_top += p.m_y;
        m_right += p.m_x;
        m_bottom += p.m_y;

        return *this;
    }

    Rect& operator-=( const Point& p ) {
        m_left -= p.m_x;
        m_top -= p.m_y;
        m_right -= p.m_x;
        m_bottom -= p.m_y;

        return *this;
    }

    Rect& operator&=( const Rect& r ) {
        m_left = std::max(m_left, r.m_left);
        m_top = std::max(m_top, r.m_top);
        m_right = std::min(m_right, r.m_right);
        m_bottom = std::min(m_bottom, r.m_bottom );

        return *this;
    }

  public:
    int m_left;
    int m_top;
    int m_right;
    int m_bottom;
}; /* class Rect */

} /* namespace yguipp */

#endif /* _RECT_HPP_ */
