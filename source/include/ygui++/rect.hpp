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

#include <ygui/rect.h>

namespace yguipp {

class Rect {
  public:
    Rect( int left, int top, int right, int bottom );

    void toRectT( rect_t* r ) const;

  public:
    int m_left;
    int m_top;
    int m_right;
    int m_bottom;
}; /* class Rect */

} /* namespace yguipp */

#endif /* _RECT_HPP_ */
