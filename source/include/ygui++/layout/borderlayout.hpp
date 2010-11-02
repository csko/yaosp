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

#ifndef _YGUIPP_LAYOUT_BORDERLAYOUT_HPP_
#define _YGUIPP_LAYOUT_BORDERLAYOUT_HPP_

#include <map>

#include <ygui++/widget.hpp>
#include <ygui++/layout/layout.hpp>

namespace yguipp {
namespace layout {

class BorderLayoutData : public LayoutData {
  public:
    enum Position {
        PAGE_START,
        PAGE_END,
        LINE_START,
        LINE_END,
        CENTER
    };

    BorderLayoutData( Position position ) : m_position(position) {}
    virtual ~BorderLayoutData( void ) {}

  public:
    Position m_position;
}; /* class BorderLayoutData */

class BorderLayout : public Layout {
  public:
    BorderLayout( void );
    virtual ~BorderLayout( void );

    int doLayout( yguipp::Panel* panel );

  private:
    void buildWidgetMap( const yguipp::Widget::ChildVector& children, yguipp::Widget** widgetMap );
}; /* class BorderLayout */

} /* namespace layout */
} /* namespace yguipp */

#endif /* _YGUIPP_LAYOUT_BORDERLAYOUT_HPP_ */
