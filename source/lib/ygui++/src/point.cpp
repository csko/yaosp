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

#include <ygui++/point.hpp>

namespace yguipp {

Point::Point( int x, int y ) : m_x(x), m_y(y) {
}

Point::Point( point_t* point ) : m_x(point->x), m_y(point->y) {
}

Point::~Point( void ) {
}

void Point::toPointT( point_t* p ) const {
    p->x = m_x;
    p->y = m_y;
}

Point Point::operator+( const Point& p ) const {
    return Point( m_x + p.m_x, m_y + p.m_y );
}

Point& Point::operator+=( const Point& p ) {
    m_x += p.m_x;
    m_y += p.m_y;

    return *this;
}

Point& Point::operator-=( const Point& p ) {
    m_x -= p.m_x;
    m_y -= p.m_y;

    return *this;
}

} /* namespace yguipp */
