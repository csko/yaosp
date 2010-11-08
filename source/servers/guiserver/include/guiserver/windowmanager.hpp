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

#ifndef _WINDOWMANAGER_HPP_
#define _WINDOWMANAGER_HPP_

#include <vector>

#include <ygui++/point.hpp>
#include <yutil++/thread/mutex.hpp>

#include <guiserver/window.hpp>
#include <guiserver/graphicsdriver.hpp>
#include <guiserver/mouse.hpp>
#include <guiserver/decorator.hpp>
#include <guiserver/guiserver.hpp>

class WindowManager : public GuiServerListener {
  public:
    WindowManager(GuiServer* guiServer, Decorator* decorator);
    ~WindowManager( void );

    Decorator* getDecorator( void ) { return m_windowDecorator; }

    inline int lock( void ) { return m_mutex.lock(); }
    inline int unLock( void ) { return m_mutex.unLock(); }

    int enable( void );

    int registerWindow( Window* window );
    int unregisterWindow( Window* window );

    int keyPressed( int key );
    int keyReleased( int key );

    int mouseMoved( const yguipp::Point& delta );
    int mousePressed( int button );
    int mouseReleased( int button );
    int mouseScrolled( int button );

    int setMovingWindow(Window* window);

    int updateWindowRegion( Window* window, const yguipp::Rect& region );
    int hideWindowRegion( Window* window, const yguipp::Rect& region );

    int windowRectChanged(Window* window, const yguipp::Rect& oldRect, const yguipp::Rect& newRect);

    int onScreenModeChanged(GuiServer* guiServer, const yguipp::ScreenModeInfo& modeInfo);

  private:
    int getWindowIndex( Window* window );
    Window* getWindowAt( const yguipp::Point& position );

    int generateVisibleRegions( int index );
    int doUpdateWindowRegion( Window* window, yguipp::Rect region );
    int doHideWindowRegion( Window* window, yguipp::Rect region );

    int invertWindowRect(void);

  private:
    yutilpp::thread::Mutex m_mutex;
    std::vector<Window*> m_windowStack;

    GraphicsDriver* m_graphicsDriver;
    Bitmap* m_screenBitmap;
    MousePointer* m_mousePointer;
    Decorator* m_windowDecorator;

    Window* m_mouseWindow;
    Window* m_mouseDownWindow;
    Window* m_activeWindow;

    Window* m_movingWindow;
    yguipp::Rect m_windowRect;
}; /* class WindowManager */

#endif /* _WINDOWMANAGER_HPP_ */
