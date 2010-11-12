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

#ifndef _SCROLLPANEL_HPP_
#define _SCROLLPANEL_HPP_

#include <ygui++/scrollbar.hpp>
#include <ygui++/event/widgetlistener.hpp>
#include <ygui++/event/adjustmentlistener.hpp>

namespace yguipp {

class ScrollPanel : public Widget, public event::WidgetListener, public event::AdjustmentListener {
  public:
    enum ScrollBarPolicy {
        SCROLLBAR_NEVER,
        SCROLLBAR_ALWAYS
    }; /* enum ScrollBarPolicy */

    ScrollPanel(ScrollBarPolicy vertPolicy = SCROLLBAR_ALWAYS, ScrollBarPolicy horizPolicy = SCROLLBAR_ALWAYS);
    virtual ~ScrollPanel(void);

    ScrollBar* getVerticalScrollBar(void);
    ScrollBar* getHorizontalScrollBar(void);

    void add(Widget* child, layout::LayoutData* data = NULL);

    Point getPreferredSize(void);

    int validate(void);

    int onWidgetResized(Widget* widget);
    int onAdjustmentValueChanged(Widget* widget);

  private:
    Point getVerticalSize(void);
    Point getHorizontalSize(void);

    int verticalValueChanged(void);
    int horizontalValueChanged(void);

    int updateScrolledWidgetSizes(const Point& visibleSize, const Point& preferredSize);
    int updateScrollBarValues(const Point& visibleSize, const Point& preferredSize);

  private:
    ScrollBarPolicy m_verticalPolicy;
    ScrollBarPolicy m_horizontalPolicy;

    ScrollBar* m_verticalBar;
    ScrollBar* m_horizontalBar;
    Widget* m_scrolledWidget;
}; /* class ScrollPanel */

} /* namespace yguipp */

#endif /* _SCROLLPANEL_HPP_ */
