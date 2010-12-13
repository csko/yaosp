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

#include <ygui++/scrollbar.hpp>

namespace yguipp {

ScrollBar::ScrollBar(Orientation orientation, int min, int max, int value, int extent)
    : m_orientation(orientation), m_min(min), m_max(max),
      m_value(value), m_extent(extent), m_increment(1) {
}

ScrollBar::~ScrollBar(void) {
}

int ScrollBar::getValue(void) {
    return m_value;
}

int ScrollBar::getMinimum(void) {
    return m_min;
}

int ScrollBar::getMaximum(void) {
    return m_max;
}

int ScrollBar::getVisibleAmount(void) {
    return m_extent;
}

int ScrollBar::setValues(int value, int extent, int min, int max) {
    if ((value < min) ||
        (extent < 0) ||
        ((value + extent) > max)) {
        return -1;
    }

    m_min = min;
    m_max = max;
    m_value = value;
    m_extent = extent;

    return 0;
}

int ScrollBar::setIncrement(int increment) {
    if (increment == 0) {
        return -1;
    }

    m_increment = increment;

    return 0;
}

int ScrollBar::setValue(int value) {
    if ((value < m_min) ||
        ((value + m_extent) > m_max)) {
        return -1;
    }

    m_value = value;
    fireAdjustmentListeners(this);

    return 0;
}

Point ScrollBar::getPreferredSize(void) {
    switch (m_orientation) {
        case VERTICAL : return Point(m_scrollBarSize, 0);
        case HORIZONTAL : return Point(0, m_scrollBarSize);
        default : return Point();
    }
}

int ScrollBar::validate(void) {
    switch (m_orientation) {
        case VERTICAL :
            validateVertical();
            break;

        case HORIZONTAL :
            validateHorizontal();
            break;
    }

    return 0;
}

int ScrollBar::paint(GraphicsContext* g) {
    Point size = getSize();

    switch (m_orientation) {
        case VERTICAL :
            paintVertical(g, size);
            break;

        case HORIZONTAL :
            paintHorizontal(g, size);
            break;
    }

    return 0;
}

int ScrollBar::mousePressed(const Point& p, int button) {
    if (m_decRect.hasPoint(p)) {
        decreaseValue();
    } else if (m_incRect.hasPoint(p)) {
        increaseValue();
    }

    return 0;
}

int ScrollBar::validateVertical(void) {
    Point size = getSize();

    m_decRect = Rect(0, 0, m_scrollBarSize - 1, m_scrollButtonSize - 1);
    m_incRect = Rect(0, size.m_y - m_scrollButtonSize, m_scrollBarSize - 1, size.m_y - 1);

    return 0;
}

int ScrollBar::validateHorizontal(void) {
    Point size = getSize();

    m_decRect = Rect(0, 0, m_scrollBarSize - 1, m_scrollButtonSize - 1);
    m_incRect = Rect(size.m_x - m_scrollButtonSize, 0, size.m_x - 1, m_scrollBarSize - 1);

    return 0;
}

int ScrollBar::paintVertical(GraphicsContext* g, const Point& size) {
    Rect r;
    Point p1;
    Point p2;

    /* Top button */
    r = m_decRect;
    g->setPenColor(Color(0, 0, 0));
    g->rectangle(r);
    g->stroke();
    r.resize(1, 1, -1, -1);
    g->setPenColor(Color(216, 216, 216));
    g->rectangle(r);
    g->fill();

    /* Top ^ */
    g->setPenColor(Color(0, 0, 0));
#if 0
    p1 = Point((r.m_left + r.m_right) / 2, r.m_top + 4);
    p2 = Point(r.m_left + 3, r.m_bottom - 4);
    g->drawLine(p1, p2);
    p2 = Point(r.m_right - 3, r.m_bottom - 4);
    g->drawLine(p1, p2);
#endif

    /* Bottom button */
    r = m_incRect;
    g->rectangle(r);
    g->stroke();
    r.resize(1, 1, -1, -1);
    g->setPenColor(Color(216, 216, 216));
    g->rectangle(r);
    g->fill();

    /* Bottom V */
    g->setPenColor(Color(0, 0, 0));
#if 0
    p1 = Point((r.m_left + r.m_right) / 2, r.m_bottom - 4);
    p2 = Point(r.m_left + 3, r.m_top + 4);
    g->drawLine(p1, p2);
    p2 = Point(r.m_right - 3, r.m_top + 4);
    g->drawLine(p1, p2);
#endif

    /* Scrollbar lines */
    g->rectangle(Rect(0, m_scrollButtonSize, 0, size.m_y - (m_scrollButtonSize + 1)));
    g->fill();
    g->rectangle(Rect(m_scrollBarSize - 1, 0, m_scrollBarSize - 1, size.m_y - (m_scrollButtonSize + 1)));
    g->fill();

    g->setPenColor(Color(180, 180, 180));
    g->rectangle(Rect(1, m_scrollButtonSize, m_scrollBarSize - 2, size.m_y - (m_scrollButtonSize + 1)));
    g->fill();

    return 0;
}

int ScrollBar::paintHorizontal(GraphicsContext* g, const Point& size) {
    Rect r;
    Point p1;
    Point p2;

    /* Left button */
    r = m_decRect;
    g->setPenColor(Color(0, 0, 0));
    g->rectangle(r);
    g->stroke();
    r.resize(1, 1, -1, -1);
    g->setPenColor(Color(216, 216, 216));
    g->rectangle(r);
    g->fill();

    /* Left < */
    g->setPenColor(Color(0, 0, 0));
#if 0
    p1 = Point(r.m_left + 4, (r.m_top + r.m_bottom) / 2);
    p2 = Point(r.m_right - 4, r.m_top + 3);
    g->drawLine(p1, p2);
    p2 = Point(r.m_right - 4, r.m_bottom - 3);
    g->drawLine(p1, p2);
#endif

    /* Right button */
    r = m_incRect;
    g->rectangle(r);
    g->stroke();
    r.resize(1, 1, -1, -1);
    g->setPenColor(Color(216, 216, 216));
    g->rectangle(r);
    g->fill();

    /* Right > */
    g->setPenColor(Color(0, 0, 0));
#if 0
    p1 = Point(r.m_right - 4, (r.m_top + r.m_bottom) / 2);
    p2 = Point(r.m_left + 4, r.m_top + 3);
    g->drawLine(p1, p2);
    p2 = Point(r.m_left + 4, r.m_bottom - 3);
    g->drawLine(p1, p2);
#endif

    /* Scrollbar lines */
    g->rectangle(Rect(m_scrollButtonSize, 0, size.m_x - (m_scrollButtonSize + 1), 0));
    g->fill();
    g->rectangle(Rect(m_scrollButtonSize, m_scrollBarSize - 1, size.m_x - (m_scrollButtonSize + 1), m_scrollBarSize - 1));
    g->fill();

    g->setPenColor(Color(180, 180, 180));
    g->rectangle(Rect(m_scrollButtonSize, 1, size.m_x - (m_scrollButtonSize + 1), m_scrollBarSize - 2));
    g->fill();

    return 0;
}

int ScrollBar::decreaseValue(void) {
    if (m_value == m_min) {
        return 0;
    }

    m_value -= m_increment;

    if (m_value < m_min) {
        m_value = m_min;
    }

    fireAdjustmentListeners(this);

    return 0;
}

int ScrollBar::increaseValue(void) {
    if (m_value == m_max) {
        return 0;
    }

    m_value += m_increment;

    if ((m_value + m_extent) > m_max) {
        m_value = m_max - m_extent;
    }

    fireAdjustmentListeners(this);

    return 0;
}

} /* namespace yguipp */
