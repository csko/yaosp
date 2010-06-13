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

#include <string>

#include <ygui++/protocol.hpp>
#include <ygui++/rect.hpp>
#include <ygui++/color.hpp>

#include <guiserver/region.hpp>
#include <guiserver/bitmap.hpp>
#include <guiserver/font.hpp>

class Application;
class GuiServer;
class DecoratorData;

class Window {
  private:
    Window( GuiServer* guiServer, Application* application );
    ~Window( void );

    bool init( WinCreate* request );

  public:
    inline int getFlags(void) { return m_flags; }
    inline int getOrder(void) { return m_order; }
    inline const yguipp::Rect& getScreenRect(void) { return m_screenRect; }
    inline Region& getVisibleRegions(void) { return m_visibleRegions; }
    inline Bitmap* getBitmap(void) { return m_bitmap; }
    inline DecoratorData* getDecoratorData(void) { return m_decoratorData; }
    inline const std::string& getTitle(void) { return m_title; }

    inline void setId(int id) { m_id = id; }
    inline void setDecoratorData(DecoratorData* data) { m_decoratorData = data; }

    int handleMessage( uint32_t code, void* data, size_t size );

    int mouseEntered(const yguipp::Point& position);
    int mouseMoved(const yguipp::Point& position);
    int mouseExited(void);
    int mousePressed(const yguipp::Point& position, int button);
    int mouseReleased(int button);

    static Window* createFrom( GuiServer* guiServer, Application* application, WinCreate* request );

  private:
    void handleDoResize( WinResize* request );
    void handleDoMoveTo( WinMoveTo* request );
    void handleRender( uint8_t* data, size_t size );
    
    void calculateWindowRects( const yguipp::Point& position, const yguipp::Point& size,
                               yguipp::Rect& screenRect, yguipp::Rect& clientRect );
  private:
    int m_id;
    int m_order;
    int m_flags;
    bool m_visible;
    std::string m_title;
    yguipp::Rect m_screenRect;
    yguipp::Rect m_clientRect;
    Region m_visibleRegions;
    Bitmap* m_bitmap;
    DecoratorData* m_decoratorData;
    bool m_mouseOnDecorator;

    yguipp::Rect m_clipRect;
    yguipp::Color m_penColor;
    yguipp::DrawingMode m_drawingMode;
    FontNode* m_font;

    GuiServer* m_guiServer;
    Application* m_application;
}; /* class Window */

#endif /* _WINDOW_HPP_ */
