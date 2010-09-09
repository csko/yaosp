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

#ifndef _SCROLLBAR_HPP_
#define _SCROLLBAR_HPP_

#include <ygui++/widget.hpp>
#include <ygui++/yconstants.hpp>

namespace yguipp {

class ScrollBar : public Widget {
  public:
    ScrollBar(Orientation orientation, int min = 0, int max = 100, int value = 0);
    virtual ~ScrollBar(void);

    Point getPreferredSize(void);

    int paint(GraphicsContext* g);

  private:
    int paintVertical(GraphicsContext* g, const Point& size);
    int paintHorizontal(GraphicsContext* g, const Point& size);

  private:
    Orientation m_orientation;
    int m_min;
    int m_max;
    int m_value;

    static const int m_scrollBarSize = 15;
    static const int m_scrollButtonSize = 15;
}; /* class ScrollBar */

} /* namespace yguipp */

#endif /* _SCROLLBAR_HPP_ */
