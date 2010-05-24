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
#include <yutil++/mutex.hpp>

#include <guiserver/window.hpp>
#include <guiserver/graphicsdriver.hpp>

class WindowManager {
  private:
    WindowManager( GraphicsDriver* graphicsDriver, Bitmap* screenBitmap );
    ~WindowManager( void );

  public:
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

  private:
    int generateVisibleRegions( int index );
    int updateWindowRegion( Window* window, yguipp::Rect region );

  private:
    yutilpp::Mutex m_mutex;
    std::vector<Window*> m_windowStack;

    GraphicsDriver* m_graphicsDriver;
    Bitmap* m_screenBitmap;
}; /* class WindowManager */

#endif /* _WINDOWMANAGER_HPP_ */
