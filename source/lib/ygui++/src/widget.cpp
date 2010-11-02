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

#include <limits.h>

#include <ygui++/widget.hpp>
#include <ygui++/window.hpp>
#include <ygui++/protocol.hpp>
#include <ygui++/application.hpp>

namespace yguipp {

Widget::Widget( void ) : m_window(NULL), m_parent(NULL), m_border(NULL), m_isValid(false),
                         m_position(0,0), m_scrollOffset(0,0), m_fullSize(0,0),
                         m_visibleSize(0,0) {
}

Widget::~Widget( void ) {
}

void Widget::add( Widget* child, const layout::LayoutData* data ) {
    child->incRef();
    child->m_parent = this;

    m_children.push_back(std::make_pair<Widget*, const layout::LayoutData*>(child, data));

    if ( m_window != NULL ) {
        child->setWindow(m_window);
    }
}

Widget* Widget::getParent( void ) {
    return m_parent;
}

const Point& Widget::getPosition( void ) {
    return m_position;
}

const Point& Widget::getScrollOffset( void ) {
    return m_scrollOffset;
}

const Point& Widget::getSize( void ) {
    return m_fullSize;
}

const Point& Widget::getVisibleSize( void ) {
    return m_visibleSize;
}

const Rect Widget::getBounds( void ) {
    return Rect(m_fullSize);
}

const Widget::ChildVector& Widget::getChildren( void ) {
    return m_children;
}

Window* Widget::getWindow( void ) {
    return m_window;
}

void Widget::setWindow( Window* window ) {
    m_window = window;

    for ( ChildVectorCIter it = m_children.begin(); it != m_children.end(); ++it ) {
        it->first->setWindow(window);
    }
}

void Widget::setBorder( border::Border* border ) {
    if (m_border != NULL) {
        m_border->decRef();
    }

    m_border = border;

    if (m_border != NULL) {
        m_border->incRef();
    }
}

void Widget::setPosition( const Point& p ) {
    m_position = p;
}

void Widget::setSize( const Point& s ) {
    m_fullSize = s;
    m_visibleSize = s;
}

void Widget::setSizes(const Point& visibleSize, const Point& fullSize) {
    m_fullSize = fullSize;
    m_visibleSize = visibleSize;
}

void Widget::invalidate( void ) {
    doInvalidate(true);
}

Point Widget::getPreferredSize( void ) {
    return Point(0,0);
}

Point Widget::getMaximumSize( void ) {
    return Point(INT_MAX, INT_MAX);
}

int Widget::validate( void ) {
    return 0;
}

int Widget::paint( GraphicsContext* g ) {
    return 0;
}

int Widget::keyPressed(int key) {
    fireKeyPressedListeners(this, key);
    return 0;
}

int Widget::keyReleased(int key) {
    fireKeyReleasedListeners(this, key);
    return 0;
}

int Widget::mouseEntered( const Point& p ) {
    return 0;
}

int Widget::mouseMoved( const Point& p ) {
    return 0;
}

int Widget::mouseExited( void ) {
    return 0;
}

int Widget::mousePressed( const Point& p, int button ) {
    return 0;
}

int Widget::mouseReleased( int button ) {
    return 0;
}

int Widget::doPaint( GraphicsContext* g, bool forced ) {
    Rect visibleRect;

    /* Calculate the visible rectangle of this widget. */
    visibleRect = m_visibleSize;
    visibleRect += g->getLeftTop();
    visibleRect &= g->currentRestrictedArea();

    /* If there is no visible part of the widget, we don't have to paint. */
    if (!visibleRect.isValid()) {
        return 0;
    }

    /* Update the restricted area according to this widget. */
    g->pushRestrictedArea(visibleRect);

    /* Paint the border. */
    if (m_border != NULL) {
        g->translateCheckPoint();
        g->translate(m_scrollOffset);

        m_border->paint(this, g);

        g->rollbackTranslate();
        g->translate(m_border->leftTop());
    }

    /* Paint the widget. */
    if ( ( forced ) ||
         ( !m_isValid ) ) {
        m_isValid = true;

        // todo: call validate only if the size of the widget changed
        validate();

        g->translateCheckPoint();
        g->translate(m_scrollOffset);

        paint(g);

        g->rollbackTranslate();
    }

    /* Paint the children of this widget as well. */
    for ( ChildVectorCIter it = m_children.begin(); it != m_children.end(); ++it ) {
        Widget* widget = it->first;

        g->translateCheckPoint();
        g->translate(widget->m_position);

        widget->doPaint(g, forced);

        g->rollbackTranslate();
    }

    if (m_border != NULL) {
        g->translate(m_border->leftTop() * -1);
    }

    /* Remove the restricted area of this widget. */
    g->popRestrictedArea();

    return 0;
}

int Widget::doInvalidate( bool notifyWindow ) {
    m_isValid = false;

    for ( ChildVectorCIter it = m_children.begin(); it != m_children.end(); ++it ) {
        it->first->doInvalidate(false);
    }

    if ( ( notifyWindow ) &&
         ( m_window != NULL ) ) {
        WinHeader header;
        header.m_windowId = m_window->getId();

        Application::getInstance()->getClientPort()->send(Y_WINDOW_WIDGET_INVALIDATED,
            reinterpret_cast<void*>(&header), sizeof(header));
    }

    return 0;
}

} /* namespace yguipp */
