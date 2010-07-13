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

Label::Label( int hAlign, int vAlign ) : m_hAlign(hAlign), m_vAlign(vAlign) {
    initFont();
}

Label::Label( const std::string& text, int hAlign, int vAlign ) : m_text(text), m_hAlign(hAlign), m_vAlign(vAlign) {
    initFont();
}

Label::~Label( void ) {
    if ( m_font != NULL ) {
        m_font->decRef();
    }
}

Point Label::getPreferredSize( void ) {
    return Point(
        m_font->getWidth(m_text) + 2 * 3,
        m_font->getHeight() + 2 * 3
    );
}

int Label::paint( GraphicsContext* g ) {
    Rect bounds = getBounds();

    g->setPenColor( Color(216, 216, 216) );
    g->fillRect( bounds );

    if ( !m_text.empty() ) {
        Point position;

        switch ( m_hAlign ) {
            case H_ALIGN_LEFT :
                position.m_x = 0;
                break;

            case H_ALIGN_CENTER :
                position.m_x = ( bounds.width() - m_font->getWidth(m_text) ) / 2;
                break;

            case H_ALIGN_RIGHT :
                position.m_x = bounds.m_right - m_font->getWidth(m_text) + 1;
                break;
        }

        switch ( m_vAlign ) {
            case V_ALIGN_TOP :
                position.m_y = m_font->getAscender() - 1;
                break;

            case V_ALIGN_CENTER : {
                int asc = m_font->getAscender();
                position.m_y = ( bounds.height() - ( asc - m_font->getDescender() ) ) / 2 + asc;
                break;
            }

            case V_ALIGN_BOTTOM :
                position.m_y = bounds.m_bottom - m_font->getHeight() + 1;
                break;
        }

        g->setFont( m_font );
        g->setPenColor( Color(0, 0, 0) );
        g->drawText( position, m_text );
    }

    return 0;
}

void Label::initFont( void ) {
    m_font = new Font( "DejaVu Sans", "Book", FontInfo(8) );
    m_font->init();
    m_font->incRef();
}

} /* namespace yguipp */
