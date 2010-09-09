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

ScrollBar::ScrollBar(Orientation orientation, int min, int max, int value) : m_orientation(orientation), m_min(min), m_max(max), m_value(value) {
}

ScrollBar::~ScrollBar(void) {
}

Point ScrollBar::getPreferredSize(void) {
    switch (m_orientation) {
        case VERTICAL : return Point(m_scrollBarSize, 0);
        case HORIZONTAL : return Point(0, m_scrollBarSize);
        default : return Point();
    }
}

int ScrollBar::paint(GraphicsContext* g) {
    Point size = getSize();

    g->setPenColor(Color(0, 0, 0));
    g->drawRect(Rect(size));

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

int ScrollBar::paintVertical(GraphicsContext* g, const Point& size) {
    Rect r;

    /* Top button */
    r = Rect(0, 0, m_scrollBarSize - 1, m_scrollButtonSize - 1);
    g->setPenColor(Color(0, 0, 0));
    g->drawRect(r);
    r.resize(1, 1, -1, -1);
    g->setPenColor(Color(216, 216, 216));
    g->fillRect(r);

    /* Bottom button */
    r = Rect(0, size.m_y - m_scrollButtonSize, m_scrollBarSize - 1, size.m_y - 1);
    g->setPenColor(Color(0, 0, 0));
    g->drawRect(r);
    r.resize(1, 1, -1, -1);
    g->setPenColor(Color(216, 216, 216));
    g->fillRect(r);

    return 0;
}

int ScrollBar::paintHorizontal(GraphicsContext* g, const Point& size) {
    Rect r;

    /* Left button */
    r = Rect(0, 0, m_scrollButtonSize - 1, m_scrollBarSize - 1);
    g->setPenColor(Color(0, 0, 0));
    g->drawRect(r);
    r.resize(1, 1, -1, -1);
    g->setPenColor(Color(216, 216, 216));
    g->fillRect(r);

    /* Right button */
    r = Rect(size.m_x - m_scrollButtonSize, 0, size.m_x - 1, m_scrollBarSize - 1);
    g->setPenColor(Color(0, 0, 0));
    g->drawRect(r);
    r.resize(1, 1, -1, -1);
    g->setPenColor(Color(216, 216, 216));
    g->fillRect(r);

    return 0;
}

} /* namespace yguipp */
