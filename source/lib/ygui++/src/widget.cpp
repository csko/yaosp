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

#include <ygui++/widget.hpp>

namespace yguipp {

Widget::Widget( void ) : m_window(NULL), m_parent(NULL) {
}

Widget::~Widget( void ) {
}

void Widget::setWindow( Window* window ) {
    m_window = window;

    for ( std::vector<Widget*>::const_iterator it = m_children.begin();
          it != m_children.end();
          ++it ) {
        (*it)->setWindow(window);
    }
}

int Widget::paint( GraphicsContext* g ) {
    return 0;
}

int Widget::doPaint( GraphicsContext* g ) {
    g->setClipRect( Rect(0, 0, 99, 99) );
    paint(g);

    for ( std::vector<Widget*>::const_iterator it = m_children.begin();
          it != m_children.end();
          ++it ) {
        (*it)->doPaint(g);
    }

    return 0;
}

} /* namespace yguipp */
