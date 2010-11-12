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
#include <ygui++/event/adjustmentlistener.hpp>

namespace yguipp {

class ScrollBar : public Widget, public event::AdjustmentSpeaker {
  public:
    ScrollBar(Orientation orientation, int min = 0, int max = 100, int value = 0, int extent = 10);
    virtual ~ScrollBar(void);

    int getValue(void);
    int setValues(int value, int extent, int min, int max);
    int setIncrement(int increment);

    Point getPreferredSize(void);

    int validate(void);
    int paint(GraphicsContext* g);

    int mousePressed(const Point& p, int button);

  private:
    int validateVertical(void);
    int validateHorizontal(void);

    int paintVertical(GraphicsContext* g, const Point& size);
    int paintHorizontal(GraphicsContext* g, const Point& size);

    int decreaseValue(void);
    int increaseValue(void);

  private:
    Orientation m_orientation;
    int m_min;
    int m_max;
    int m_value;
    int m_extent;
    int m_increment;

    Rect m_decRect;
    Rect m_incRect;

    static const int m_scrollBarSize = 15;
    static const int m_scrollButtonSize = 15;
}; /* class ScrollBar */

} /* namespace yguipp */

#endif /* _SCROLLBAR_HPP_ */
