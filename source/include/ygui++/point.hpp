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

#ifndef _POINT_HPP_
#define _POINT_HPP_

namespace yguipp {

class Point {
  public:
    Point( int x = 0, int y = 0 ) : m_x(x), m_y(y) {}

    Point operator+( const Point& p ) const { return Point(m_x + p.m_x, m_y + p.m_y); }
    Point operator-( const Point& p ) const { return Point(m_x - p.m_x, m_y - p.m_y); }
    Point operator*( int n ) const { return Point(m_x * n, m_y * n); }
    Point operator/( int n ) const { return Point(m_x / n, m_y / n); }

    Point& operator+=( const Point& p ) { m_x += p.m_x; m_y += p.m_y; return *this; }
    Point& operator-=( const Point& p ) { m_x -= p.m_x; m_y -= p.m_y; return *this; }

  public:
    int m_x;
    int m_y;
}; /* class Point */

} /* namespace yguipp */

#endif /* _POINT_HPP_ */
