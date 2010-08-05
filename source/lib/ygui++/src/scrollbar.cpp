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
        case VERTICAL : return Point(15, 0);
        case HORIZONTAL : return Point(0, 15);
        default : return Point();
    }
}

int ScrollBar::paint(GraphicsContext* g) {
    Point size = getSize();

    g->setPenColor(Color(0, 0, 0));
    g->drawRect(Rect(size));

    return 0;
}

} /* namespace yguipp */
