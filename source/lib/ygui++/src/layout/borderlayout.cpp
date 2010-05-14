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

#include <map>

#include <ygui++/panel.hpp>
#include <ygui++/layout/borderlayout.hpp>

namespace yguipp {
namespace layout {

BorderLayout::BorderLayout( void ) {
}

BorderLayout::~BorderLayout( void ) {
}

int BorderLayout::doLayout( yguipp::Panel* panel ) {
    Widget* widget;
    yguipp::Point panelSize = panel->getSize();
    yguipp::Rect centerRect(panelSize);
    const yguipp::Widget::ChildVector& children = panel->getChildren();
    yguipp::Widget* widgetMap[5] = { NULL, };

    buildWidgetMap(children, widgetMap);

    /* Page start widget */

    widget = widgetMap[BorderLayoutData::PAGE_START];
    if ( widget != NULL ) {
        Point size(
            std::min(panelSize.m_x,widget->getMaximumSize().m_x),
            widget->getPreferredSize().m_y
        );

        widget->setPosition( Point( ( panelSize.m_x - size.m_x ) / 2, 0 ) );
        widget->setSize( size );

        centerRect.m_top += size.m_y;
    }

    /* Page end widget */

    widget = widgetMap[BorderLayoutData::PAGE_END];
    if ( widget != NULL ) {
        Point size(
            std::min(panelSize.m_x,widget->getMaximumSize().m_x),
            widget->getPreferredSize().m_y
        );

        widget->setPosition( Point( ( panelSize.m_x - size.m_x ) / 2, centerRect.m_bottom - size.m_y + 1 ) );
        widget->setSize( size );

        centerRect.m_bottom -= size.m_y;
    }

    /* Line start widget */

    widget = widgetMap[BorderLayoutData::LINE_START];
    if ( widget != NULL ) {
        Point size(
            widget->getPreferredSize().m_x,
            std::min(centerRect.height(), widget->getMaximumSize().m_y)
        );

        widget->setPosition(
            Point( 0, centerRect.m_top + ( centerRect.height() - size.m_y ) / 2 )
        );
        widget->setSize( size );

        centerRect.m_left += size.m_x;
    }

    /* Line end widget */

    widget = widgetMap[BorderLayoutData::LINE_END];
    if ( widget != NULL ) {
        Point size(
            widget->getPreferredSize().m_x,
            std::min(centerRect.height(), widget->getMaximumSize().m_y)
        );

        widget->setPosition(
            Point( centerRect.m_right - size.m_x + 1, centerRect.m_top + ( centerRect.height() - size.m_y ) / 2 )
        );
        widget->setSize( size );

        centerRect.m_right -= size.m_x;
    }

    /* Center widget */

    widget = widgetMap[BorderLayoutData::CENTER];
    if ( widget != NULL ) {
        widget->setPosition( centerRect.leftTop() );
        widget->setSize( centerRect.bounds() );
    }

    return 0;
}

void BorderLayout::buildWidgetMap( const yguipp::Widget::ChildVector& children, yguipp::Widget** widgetMap ) {
    for ( yguipp::Widget::ChildVectorCIter it = children.begin();
          it != children.end();
          ++it ) {
        BorderLayoutData* layoutData;

        if ( it->second == NULL ) {
            continue;
        }

        layoutData = dynamic_cast<BorderLayoutData*>(it->second);
        widgetMap[layoutData->m_position] = it->first;
    }
}

} /* namespace layout */
} /* namespace yguipp */
