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

#include <ygui/point.h>

namespace yguipp {

class Point {
  public:
    Point( int x = 0, int y = 0 );

    void toPointT( point_t* p ) const;

    Point operator+( const Point& p ) const;
    Point& operator+=( const Point& p );
    Point& operator-=( const Point& p );

  public:
    int m_x;
    int m_y;
}; /* class Point */

} /* namespace yguipp */

#endif /* _POINT_HPP_ */
