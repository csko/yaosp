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

#include <iostream>
#include <yaosp/debug.h>

#include <ygui++/menu.hpp>
#include <ygui++/menuitem.hpp>

namespace yguipp {

MenuItem::MenuItem( const std::string& text, Bitmap* image ) : m_text(text), m_image(image), m_subMenu(NULL),
                                                               m_menuParent(NULL), m_active(false) {
    initFont();

    if (m_image != NULL) {
        m_image->incRef();
    }
}

MenuItem::~MenuItem( void ) {
    if (m_image) {
        m_image->decRef();
    }

    if ( m_font != NULL ) {
        m_font->decRef();
    }

    if ( m_subMenu != NULL ) {
        m_subMenu->decRef();
    }
}

Point MenuItem::getPreferredSize( void ) {
    return Point( m_font->getWidth(m_text) + 6, m_font->getHeight() + 6 );
}

Menu* MenuItem::getSubMenu( void ) {
    Menu* subMenu;

    subMenu = m_subMenu;

    if ( subMenu != NULL ) {
        subMenu->incRef();
    }

    return subMenu;
}

int MenuItem::setActive( bool active ) {
    m_active = active;
    invalidate();
    return 0;
}

int MenuItem::setSubMenu( Menu* menu ) {
    m_subMenu = menu;
    m_subMenu->incRef();
    return 0;
}

int MenuItem::setMenuParent( MenuItemParent* menuParent ) {
    m_menuParent = menuParent;
    return 0;
}

int MenuItem::mouseEntered( const Point& p ) {
    m_menuParent->itemActivated(this);
    return 0;
}

int MenuItem::mouseExited( void ) {
    m_menuParent->itemDeActivated(this);
    return 0;
}

int MenuItem::mousePressed( const Point& p, int button ) {
    m_menuParent->itemPressed(this);

    if (m_subMenu == NULL) {
        fireActionListeners(this);
    }

    return 0;
}

int MenuItem::paint( GraphicsContext* g ) {
    Point position;
    Rect bounds = getBounds();

    if ( m_active ) {
        g->setPenColor( Color(156, 156, 156) );
    } else {
        g->setPenColor( Color(216, 216, 216) );
    }

    g->fillRect( bounds );

    position.m_x = 3;
    int asc = m_font->getAscender();
    position.m_y = ( bounds.height() - ( asc - m_font->getDescender() ) ) / 2 + asc;

    g->setFont( m_font );
    g->setPenColor( Color(0, 0, 0) );
    g->drawText( position, m_text );

    return 0;
}

void MenuItem::initFont( void ) {
    m_font = new Font( "DejaVu Sans", "Book", FontInfo(8) );
    m_font->init();
    m_font->incRef();
}

} /* namespace yguipp */
