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

#include <ygui++/label.hpp>

namespace yguipp {

Label::Label( void ) {
    initFont();
}

Label::Label( const std::string& text ) : m_text(text) {
    initFont();
}

Label::~Label( void ) {
    if ( m_font != NULL ) {
        m_font->decRef();
    }
}

int Label::paint( GraphicsContext* g ) {
    Rect bounds = getBounds();

    g->setPenColor( Color(216, 216, 216) );
    g->fillRect( bounds );

    if ( !m_text.empty() ) {
        g->setFont( m_font );
        g->setPenColor( Color(0, 0, 0) );
        g->drawText( Point(0, m_font->getAscender()), m_text );
    }

    return 0;
}

void Label::initFont( void ) {
    m_font = new Font( "DejaVu Sans", "Book", 8 * 64 );
    m_font->init();
    m_font->incRef();
}

} /* namespace yguipp */
