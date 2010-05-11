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
    Point( int x, int y );

    inline int getX( void ) { return m_x; }
    inline int getY( void ) { return m_y; }
    
  private:
    int m_x;
    int m_y;
}; /* class Point */

} /* namespace yguipp */

#endif /* _POINT_HPP_ */
