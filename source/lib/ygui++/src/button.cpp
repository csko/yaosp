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

#include <ygui++/button.hpp>

namespace yguipp {

Button::Button( int hAlign, int vAlign ) : m_pressed(false), m_hAlign(hAlign), m_vAlign(vAlign) {
    initFont();
}

Button::Button( const std::string& text, int hAlign, int vAlign ) : m_text(text), m_pressed(false),
                                                                    m_hAlign(hAlign), m_vAlign(vAlign) {
    initFont();
}

Button::~Button( void ) {
    if ( m_font != NULL ) {
        m_font->decRef();
    }
}

Point Button::getPreferredSize( void ) {
    return Point(
        m_font->getWidth(m_text) + 2 * 3,
        m_font->getHeight() + 2 * 3
    );
}

int Button::paint( GraphicsContext* g ) {
    Rect bounds = getBounds();

    if ( m_pressed ) {
        g->setPenColor( Color(156, 156, 156) );
    } else {
        g->setPenColor( Color(216, 216, 216) );
    }

    g->fillRect( bounds );
    g->setPenColor( Color(0, 0, 0) );
    g->drawRect( bounds );

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
        g->drawText( position, m_text );
    }

    return 0;
}

int Button::mousePressed( const Point& p, int button ) {
    m_pressed = true;
    invalidate();
    return 0;
}

int Button::mouseReleased( int button ) {
    m_pressed = false;
    invalidate();
    return 0;
}

void Button::initFont( void ) {
    m_font = new Font( "DejaVu Sans", "Book", FontInfo(8) );
    m_font->init();
    m_font->incRef();
}

} /* namespace yguipp */
