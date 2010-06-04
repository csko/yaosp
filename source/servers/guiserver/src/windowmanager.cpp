/* GUI server
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

#include <guiserver/windowmanager.hpp>
#include <guiserver/guiserver.hpp>

#include "../driver/decorator/default/default.hpp"

WindowManager::WindowManager(GuiServer* guiServer) : m_mutex("wm_lock"),
                                                     m_graphicsDriver(guiServer->getGraphicsDriver()),
                                                     m_screenBitmap(guiServer->getScreenBitmap()),
                                                     m_mouseWindow(NULL) {
    m_mousePointer = new MousePointer();
    m_mousePointer->init();
    m_mousePointer->moveTo(NULL, NULL, m_screenBitmap->size() / 2);
    m_windowDecorator = new DefaultDecorator(guiServer);
}

WindowManager::~WindowManager( void ) {
}

int WindowManager::enable( void ) {
    m_mousePointer->show(m_graphicsDriver, m_screenBitmap);
    return 0;
}

int WindowManager::registerWindow( Window* window ) {
    size_t i;

    window->setDecoratorData(m_windowDecorator->createWindowData());

    if ((window->getFlags() & WINDOW_NO_BORDER) == 0) {
        m_windowDecorator->update(m_graphicsDriver, window);
    }

    lock();

    for ( i = 0; i < m_windowStack.size(); i++ ) {
        if ( window->getOrder() >= m_windowStack[i]->getOrder() ) {
            break;
        }
    }

    m_windowStack.insert( m_windowStack.begin() + i, window );

    for ( ; i < m_windowStack.size(); i++ ) {
        generateVisibleRegions(i);
    }

    doUpdateWindowRegion(window,window->getScreenRect());

    /* Update mouse window if needed. */

    const yguipp::Point& mousePosition = m_mousePointer->getPosition();
    Window* tmp = getWindowAt(mousePosition);

    if (tmp != m_mouseWindow){
        if (m_mouseWindow != NULL){
            m_mouseWindow->mouseExited();
        }

        m_mouseWindow = tmp;
        m_mouseWindow->mouseEntered(mousePosition);
    }

    unLock();

    return 0;
}

int WindowManager::unregisterWindow( Window* window ) {
    // todo
    return 0;
}

int WindowManager::keyPressed( int key ) {
    return 0;
}

int WindowManager::keyReleased( int key ) {
    return 0;
}

int WindowManager::mouseMoved( const yguipp::Point& delta ) {
    m_mousePointer->moveBy(m_graphicsDriver, m_screenBitmap, delta);
    return 0;
}

int WindowManager::mousePressed( int button ) {
    return 0;
}

int WindowManager::mouseReleased( int button ) {
    return 0;
}

int WindowManager::mouseScrolled( int button ) {
    return 0;
}

int WindowManager::updateWindowRegion( Window* window, const yguipp::Rect& region ) {
    lock();
    doUpdateWindowRegion(window, region);
    unLock();

    return 0;
}

Window* WindowManager::getWindowAt( const yguipp::Point& position ) {
    for ( std::vector<Window*>::const_iterator it = m_windowStack.begin();
          it != m_windowStack.end();
          ++it ) {
        Window* window = *it;

        if (window->getScreenRect().hasPoint(position)) {
            return window;
        }
    }

    return NULL;
}

int WindowManager::generateVisibleRegions( int index ) {
    Window* window = m_windowStack[index];
    Region& visibleRegions = window->getVisibleRegions();
    yguipp::Rect realScreenRect = window->getScreenRect() & m_screenBitmap->bounds();

    visibleRegions.clear();

    if ( !realScreenRect.isValid() ) {
        return 0;
    }

    visibleRegions.add(realScreenRect);

    for ( int i = index - 1; i >= 0; i-- ) {
        Window* tmp = m_windowStack[i];
        visibleRegions.exclude(tmp->getScreenRect());
    }

    return 0;
}

int WindowManager::doUpdateWindowRegion( Window* window, yguipp::Rect region ) {
    bool mouseHidden = false;
    const yguipp::Rect& mouseRect = m_mousePointer->getRect();
    Region& visibleRegions = window->getVisibleRegions();
    yguipp::Point winLeftTop = window->getScreenRect().leftTop();

    region &= m_screenBitmap->bounds();

    for ( ClipRect* clipRect = visibleRegions.getClipRects(); clipRect != NULL; clipRect = clipRect->m_next ) {
        yguipp::Rect visibleRect = clipRect->m_rect & region;

        if ( !visibleRect.isValid() ) {
            continue;
        }

        if ( ( !mouseHidden ) &&
             ( visibleRect.doIntersect(mouseRect) ) ) {
            m_mousePointer->hide(m_graphicsDriver, m_screenBitmap);
            mouseHidden = true;
        }

        m_graphicsDriver->blitBitmap(
            m_screenBitmap, visibleRect.leftTop(),
            window->getBitmap(), visibleRect - winLeftTop,
            DM_COPY
        );
    }

    if ( mouseHidden ) {
        m_mousePointer->show(m_graphicsDriver, m_screenBitmap);
    }

    return 0;
}

