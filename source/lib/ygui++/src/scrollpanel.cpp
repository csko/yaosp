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
    m_verticalBar->addAdjustmentListener(this);
    m_verticalBar->setIncrement(10);

    m_horizontalBar = new ScrollBar(HORIZONTAL);
    m_horizontalBar->addAdjustmentListener(this);
    m_horizontalBar->setIncrement(10);

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

    m_scrolledWidget->addWidgetListener(this);
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

    Point preferredSize;
    Point visibleSize = size - Point(verticalSize.m_x, horizontalSize.m_y);

    /* The scrolled widget. */
    if (m_scrolledWidget != NULL) {
        m_scrolledWidget->setPosition(Point(0, 0));

        preferredSize = m_scrolledWidget->getPreferredSize();
        updateScrolledWidgetSizes(visibleSize, preferredSize);
    }

    /* Update scrollbar values. */
    updateScrollBarValues(visibleSize, preferredSize);

    return 0;
}

int ScrollPanel::onWidgetResized(Widget* widget) {
    Point size = getSize();
    Point verticalSize = m_verticalBar->getPreferredSize();
    Point horizontalSize = m_horizontalBar->getPreferredSize();
    Point preferredSize = m_scrolledWidget->getPreferredSize();
    Point visibleSize = size - Point(verticalSize.m_x, horizontalSize.m_y);

    updateScrolledWidgetSizes(visibleSize, preferredSize);
    updateScrollBarValues(visibleSize, preferredSize);

    return 0;
}

int ScrollPanel::onAdjustmentValueChanged(Widget* widget) {
    if (widget == m_verticalBar) {
        verticalValueChanged();
    } else if (widget == m_horizontalBar) {
        horizontalValueChanged();
    }

    return 0;
}

int ScrollPanel::verticalValueChanged(void) {
    if (m_scrolledWidget == NULL) {
        return 0;
    }

    Point scrollOffset = m_scrolledWidget->getScrollOffset();
    scrollOffset.m_y = -m_verticalBar->getValue();
    m_scrolledWidget->setScrollOffset(scrollOffset);

    m_scrolledWidget->invalidate();

    return 0;
}

int ScrollPanel::horizontalValueChanged(void) {
    if (m_scrolledWidget == NULL) {
        return 0;
    }

    Point scrollOffset = m_scrolledWidget->getScrollOffset();
    scrollOffset.m_x = -m_horizontalBar->getValue();
    m_scrolledWidget->setScrollOffset(scrollOffset);

    m_scrolledWidget->invalidate();

    return 0;
}

int ScrollPanel::updateScrolledWidgetSizes(const Point& visibleSize, const Point& preferredSize) {
    Point fullSize;

    fullSize.m_x = std::max(preferredSize.m_x, visibleSize.m_x);
    fullSize.m_y = std::max(preferredSize.m_y, visibleSize.m_y);

    m_scrolledWidget->setSizes(visibleSize, fullSize);

    return 0;
}

int ScrollPanel::updateScrollBarValues(const Point& visibleSize, const Point& preferredSize) {
    int value;

    /* Update the vertical bar. */
    value = m_verticalBar->getValue();

    if (value > (preferredSize.m_y - visibleSize.m_y)) {
        value = preferredSize.m_y - visibleSize.m_y;
    }

    m_verticalBar->setValues(value, visibleSize.m_y, 0, preferredSize.m_y);

    /* Update the horizontal bar. */
    value = m_horizontalBar->getValue();

    if (value > (preferredSize.m_x - visibleSize.m_x)) {
        value = preferredSize.m_x - visibleSize.m_x;
    }

    m_horizontalBar->setValues(value, visibleSize.m_x, 0, preferredSize.m_x);

    return 0;
}

} /* namespace yguipp */
