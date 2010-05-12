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

#include <ygui++/panel.hpp>

namespace yguipp {

Panel::Panel( void ) : m_layout(NULL) {
    m_backgroundColor = Color(216, 216, 216);
}

Panel::~Panel( void ) {
}

void Panel::setLayout( layout::Layout* layout ) {
    if ( m_layout != NULL ) {
        m_layout->decRef();
    }

    m_layout = layout;

    if ( m_layout != NULL ) {
        m_layout->incRef();
    }
}

void Panel::setBackgroundColor( const Color& c ) {
    m_backgroundColor = c;
}

int Panel::validate( void ) {
    if ( m_layout != NULL ) {
        m_layout->doLayout(this);
    }

    return 0;
}

int Panel::paint( GraphicsContext* g ) {
    g->setPenColor( m_backgroundColor );
    g->fillRect( getBounds() );

    return 0;
}

} /* namespace yguipp */
