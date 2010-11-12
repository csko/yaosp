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

#include <assert.h>

#include <guiserver/windowmanager.hpp>
#include <guiserver/guiserver.hpp>

WindowManager::WindowManager(GuiServer* guiServer, Decorator* decorator) : m_mutex("wm_lock"),
                                                                           m_graphicsDriver(guiServer->getGraphicsDriver()),
                                                                           m_screenBitmap(guiServer->getScreenBitmap()),
                                                                           m_windowDecorator(decorator), m_mouseWindow(NULL),
                                                                           m_mouseDownWindow(NULL), m_activeWindow(NULL),
                                                                           m_movingWindow(NULL) {
    guiServer->addListener(this);

    m_mousePointer = new MousePointer();
    m_mousePointer->init();
    m_mousePointer->moveTo(NULL, NULL, m_screenBitmap->size() / 2);
}

WindowManager::~WindowManager( void ) {
    delete m_mousePointer;
}

int WindowManager::enable( void ) {
    m_mousePointer->show(m_graphicsDriver, m_screenBitmap);
    return 0;
}

int WindowManager::registerWindow( Window* window ) {
    size_t i;

    /* Create decorator data if the window has border. */
    if ((window->getFlags() & yguipp::WINDOW_NO_BORDER) == 0) {
        window->setDecoratorData(m_windowDecorator->createWindowData());

        m_windowDecorator->calculateItemPositions(window);
        m_windowDecorator->update(m_graphicsDriver, window);
    }

    lock();

    for ( i = 0; i < m_windowStack.size(); i++ ) {
        if ( window->getOrder() <= m_windowStack[i]->getOrder() ) {
            break;
        }
    }

    m_windowStack.insert( m_windowStack.begin() + i, window );

    for ( ; i < m_windowStack.size(); i++ ) {
        generateVisibleRegions(i);
    }

    doUpdateWindowRegion(window, window->getScreenRect());

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

    /* Update the active window. */

    if (m_activeWindow != NULL) {
        if (!(m_activeWindow->getFlags() & yguipp::WINDOW_MENU) ||
            !(window->getFlags() & yguipp::WINDOW_MENU)) {
            m_activeWindow->deactivated(yguipp::OTHER_WINDOW_OPENED);
        }
    }

    m_activeWindow = window;
    m_activeWindow->activated(yguipp::WINDOW_OPENED);

    unLock();

    return 0;
}

int WindowManager::unregisterWindow( Window* window ) {
    int index;

    lock();

    /* Make sure that the window is visible at the moment. */
    index = getWindowIndex(window);

    if (index == -1) {
        unLock();
        return 0;
    }

    /* Hide the visible regions of the unregistered window. */
    doHideWindowRegion(window, window->getScreenRect());

    /* Update window stack and the visible regions of other windows. */
    m_windowStack.erase(m_windowStack.begin() + index);

    for (size_t i = index; i < m_windowStack.size(); i++) {
        generateVisibleRegions(i);
    }

    /* Update mouse window if it's required. */
    if (m_mouseWindow == window) {
        const yguipp::Point& mousePosition = m_mousePointer->getPosition();
        m_mouseWindow = getWindowAt(mousePosition);

        if (m_mouseWindow != NULL) {
            m_mouseWindow->mouseEntered(mousePosition);
        }
    }

    /* Update active window. */
    if (m_activeWindow == window) {
        m_activeWindow->deactivated(yguipp::WINDOW_CLOSED);

        if (m_windowStack.empty()) {
            m_activeWindow = NULL;
        } else {
            m_activeWindow = m_windowStack[0];
            m_activeWindow->activated(yguipp::OTHER_WINDOW_CLOSED);
        }
    }

    unLock();

    /* Destroy decorator data if the window has border. */
    if ((window->getFlags() & yguipp::WINDOW_NO_BORDER) == 0) {
        delete window->getDecoratorData();
        window->setDecoratorData(NULL);
    }

    return 0;
}

int WindowManager::keyPressed( int key ) {
    lock();

    if (m_activeWindow != NULL) {
        m_activeWindow->keyPressed(key);
    }

    unLock();

    return 0;
}

int WindowManager::keyReleased( int key ) {
    lock();

    if (m_activeWindow != NULL) {
        m_activeWindow->keyReleased(key);
    }

    unLock();

    return 0;
}

int WindowManager::mouseMoved( const yguipp::Point& delta ) {
    lock();

    yguipp::Point realDelta = m_mousePointer->getPosition();

    /* Move the mouse pointer on the screen. */
    m_mousePointer->moveBy(m_graphicsDriver, m_screenBitmap, delta);

    /* Update the current window under the mouse pointer. */
    const yguipp::Point& mousePosition = m_mousePointer->getPosition();
    Window* window = getWindowAt(mousePosition);

    /* Calculate the real mouse movement delta. */
    realDelta = mousePosition - realDelta;

    if (m_movingWindow != NULL) {
        invertWindowRect();
        m_windowRect += realDelta;
        invertWindowRect();
    } else {
        if (window != m_mouseWindow) {
            if (m_mouseWindow != NULL) {
                m_mouseWindow->mouseExited();
            }

            m_mouseWindow = window;

            if (m_mouseWindow != NULL) {
                m_mouseWindow->mouseEntered(mousePosition);
            }
        } else {
            if (m_mouseWindow != NULL) {
                m_mouseWindow->mouseMoved(mousePosition);
            }
        }
    }

    unLock();

    return 0;
}

int WindowManager::mousePressed( int button ) {
    lock();

    if (m_mouseWindow != NULL) {
        if (m_activeWindow != m_mouseWindow) {
            if (m_activeWindow != NULL) {
                if (!(m_activeWindow->getFlags() & yguipp::WINDOW_MENU) ||
                    !(m_mouseWindow->getFlags() & yguipp::WINDOW_MENU)) {
                    m_activeWindow->deactivated(yguipp::OTHER_WINDOW_CLICKED);
                }
            }

            m_activeWindow = m_mouseWindow;
            m_activeWindow->activated(yguipp::WINDOW_CLICKED);

            /* Update order of windows in the window stack if needed. */
            int oldIndex = getWindowIndex(m_activeWindow);
            assert(oldIndex != -1);

            int newIndex = oldIndex - 1;

            while ((newIndex >= 0) &&
                   (m_windowStack[newIndex]->getOrder() >= m_activeWindow->getOrder())) {
                    newIndex--;
            }

            newIndex++;

            if (newIndex < oldIndex) {
                m_windowStack.erase(m_windowStack.begin() + oldIndex);
                m_windowStack.insert(m_windowStack.begin() + newIndex, m_activeWindow);

                for (int i = newIndex; i <= oldIndex; i++) {
                    generateVisibleRegions(i);
                }

                doUpdateWindowRegion(m_activeWindow, m_activeWindow->getScreenRect());
            }
        }

        m_mouseWindow->mousePressed(m_mousePointer->getPosition(), button);
    }

    m_mouseDownWindow = m_mouseWindow;

    unLock();

    return 0;
}

int WindowManager::mouseReleased( int button ) {
    lock();

    if (m_mouseDownWindow != NULL) {
        m_mouseDownWindow->mouseReleased(button);
        m_mouseDownWindow = NULL;
    }

    unLock();

    return 0;
}

int WindowManager::mouseScrolled( int button ) {
    return 0;
}

int WindowManager::setMovingWindow(Window* window) {
    // NOTE: We assume here that the WindowManager lock is owned by the thread.

    if (window != NULL) {
        assert(m_movingWindow == NULL);
        assert(!window->isMoving() && !window->isResizing());

        window->setMoving(true);
        m_windowRect = window->getScreenRect();

        invertWindowRect();
    } else {
        assert(m_movingWindow != NULL);
        assert(m_movingWindow->isMoving() && !m_movingWindow->isResizing());

        m_movingWindow->setMoving(false);
        invertWindowRect();

        /* Update the required parts only if the window really moved. */
        if (m_movingWindow->getScreenRect() != m_windowRect) {
            m_movingWindow->doMoveTo(m_windowRect.leftTop());
        }
    }

    m_movingWindow = window;

    return 0;
}

int WindowManager::updateWindowRegion( Window* window, const yguipp::Rect& region ) {
    lock();
    doUpdateWindowRegion(window, region);
    unLock();

    return 0;
}

int WindowManager::hideWindowRegion( Window* window, const yguipp::Rect& region ) {
    lock();
    doHideWindowRegion(window, region);
    unLock();

    return 0;
}

int WindowManager::windowRectChanged(Window* window, const yguipp::Rect& oldRect, const yguipp::Rect& newRect) {
    bool sizeChanged = (oldRect.size() != newRect.size());

    assert(window->isVisible());

    doHideWindowRegion(window, oldRect);

    int index = getWindowIndex(window);
    assert(index != -1);

    while (index < (int)m_windowStack.size()) {
        generateVisibleRegions(index++);
    }

    m_windowDecorator->calculateItemPositions(window);

    if (sizeChanged) {
        m_windowDecorator->update(m_graphicsDriver, window);
    }

    doUpdateWindowRegion(window, newRect);

    return 0;
}

int WindowManager::onScreenModeChanged(GuiServer* guiServer, const yguipp::ScreenModeInfo& modeInfo) {
    /* Update our pointer to the screen bitmap. */
    m_screenBitmap = guiServer->getScreenBitmap();

    /* Regenerate visible regions of all window according to the new screen size. */
    for (size_t i = 0; i < m_windowStack.size(); i++) {
        generateVisibleRegions(i);
    }

    /* Fill the background. */
    m_graphicsDriver->fillRect(
        m_screenBitmap, m_screenBitmap->bounds(), m_screenBitmap->bounds(), yguipp::Color(75, 100, 125), yguipp::DM_COPY
    );

    /* Redraw all the windows. */
    for (size_t i = 0; i < m_windowStack.size(); i++) {
        Window* window = m_windowStack[i];
        doUpdateWindowRegion(window, window->getScreenRect());
    }

    return 0;
}

int WindowManager::getWindowIndex( Window* window ) {
    for ( int i = 0; i < (int)m_windowStack.size(); i++ ) {
        if (m_windowStack[i] == window) {
            return i;
        }
    }

    return -1;
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

    for (ClipRect* clipRect = visibleRegions.getClipRects(); clipRect != NULL; clipRect = clipRect->m_next) {
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
            yguipp::DM_COPY
        );
    }

    if ( mouseHidden ) {
        m_mousePointer->show(m_graphicsDriver, m_screenBitmap);
    }

    return 0;
}

int WindowManager::doHideWindowRegion( Window* window, yguipp::Rect region ) {
    int index;
    bool mouseHidden = false;
    Region& visibleRegions = window->getVisibleRegions();

    index = getWindowIndex(window);
    assert(index != -1);

    region &= m_screenBitmap->bounds();

    if (region.doIntersect(m_mousePointer->getRect())) {
        m_mousePointer->hide(m_graphicsDriver, m_screenBitmap);
        mouseHidden = true;
    }

    for (size_t i = index + 1; i < m_windowStack.size(); i++) {
        Window* tmp = m_windowStack[i];
        Region origVisibleRegions = window->getVisibleRegions();

        for (ClipRect* clipRect = origVisibleRegions.getClipRects();
             clipRect != NULL;
             clipRect = clipRect->m_next) {

            yguipp::Rect visibleRect = clipRect->m_rect & region & tmp->getScreenRect();

            if (!visibleRect.isValid()) {
                continue;
            }

            visibleRegions.exclude(visibleRect);

            m_graphicsDriver->blitBitmap(
                m_screenBitmap, visibleRect.leftTop(),
                tmp->getBitmap(), visibleRect - tmp->getScreenRect().leftTop(),
                yguipp::DM_COPY
            );
        }
    }

    for (ClipRect* clipRect = visibleRegions.getClipRects(); clipRect != NULL; clipRect = clipRect->m_next) {
        yguipp::Rect visibleRect = clipRect->m_rect & region;

        if (!visibleRect.isValid()) {
            continue;
        }

        m_graphicsDriver->fillRect(
            m_screenBitmap, m_screenBitmap->bounds(), visibleRect, yguipp::Color(75, 100, 125), yguipp::DM_COPY
        );
    }

    if (mouseHidden) {
        m_mousePointer->show(m_graphicsDriver, m_screenBitmap);
    }

    return 0;
}

int WindowManager::invertWindowRect(void) {
    m_graphicsDriver->drawRect(m_screenBitmap, m_screenBitmap->bounds(), m_windowRect, yguipp::Color(), yguipp::DM_INVERT);
    return 0;
}
