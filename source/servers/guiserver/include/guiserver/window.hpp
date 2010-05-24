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

#ifndef _WINDOW_HPP_
#define _WINDOW_HPP_

#include <ygui++/protocol.hpp>
#include <ygui++/rect.hpp>

#include <guiserver/region.hpp>
#include <guiserver/bitmap.hpp>

class Window {
  private:
    Window( void );
    ~Window( void );

    bool init( WinCreate* request );

  public:
    inline int getOrder( void ) { return m_order; }
    inline const yguipp::Rect& getScreenRect( void ) { return m_screenRect; }
    inline Region& getVisibleRegions( void ) { return m_visibleRegions; }
    inline Bitmap* getBitmap( void ) { return m_bitmap; }

    int handleMessage( uint32_t code, void* data, size_t size );

    static Window* createFrom( WinCreate* request );

  private:
    int m_order;
    yguipp::Rect m_screenRect;
    yguipp::Rect m_clientRect;
    Region m_visibleRegions;
    Bitmap* m_bitmap;
}; /* class Window */

#endif /* _WINDOW_HPP_ */
