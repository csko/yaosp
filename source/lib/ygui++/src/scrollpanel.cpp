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

#include <ygui++/scrollpanel.hpp>

namespace yguipp {

ScrollPanel::ScrollPanel(void) : m_scrolledWidget(NULL) {
    m_verticalBar = new ScrollBar(VERTICAL);
    m_horizontalBar = new ScrollBar(HORIZONTAL);

    Widget::add(m_verticalBar);
    Widget::add(m_horizontalBar);
}

ScrollPanel::~ScrollPanel(void) {
    delete m_verticalBar;
    delete m_horizontalBar;
}

void ScrollPanel::add(Widget* child, layout::LayoutData* data) {
    if (m_scrolledWidget != NULL) {
        return;
    }

    m_scrolledWidget = child;
    Widget::add(m_scrolledWidget);
}

int ScrollPanel::validate(void) {
    Point size = getSize();
    Point verticalSize = m_verticalBar->getPreferredSize();
    Point horizontalSize = m_horizontalBar->getPreferredSize();

    /* Vertical bar. */
    m_verticalBar->setSize(Point(verticalSize.m_x, size.m_y - horizontalSize.m_y));
    m_verticalBar->setPosition(Point(size.m_x - verticalSize.m_x, 0));

    /* Horizontal bar. */
    m_horizontalBar->setSize(Point(size.m_x - verticalSize.m_x, horizontalSize.m_y));
    m_horizontalBar->setPosition(Point(0, size.m_y - horizontalSize.m_y));

    /* The scrolled widget. */
    if (m_scrolledWidget != NULL) {
        Point fullSize;
        Point visibleSize = size - Point(verticalSize.m_x, horizontalSize.m_y);
        Point preferredSize = m_scrolledWidget->getPreferredSize();

        fullSize.m_x = std::max(preferredSize.m_x, visibleSize.m_x);
        fullSize.m_y = std::max(preferredSize.m_y, visibleSize.m_y);

        m_scrolledWidget->setPosition(Point(0, 0));
        m_scrolledWidget->setSizes(visibleSize, fullSize);
    }

    return 0;
}

} /* namespace yguipp */
