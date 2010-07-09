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

#ifndef _GRAPHICSCONTEXT_HPP_
#define _GRAPHICSCONTEXT_HPP_

#include <stack>
#include <string>

#include <ygui++/rect.hpp>
#include <ygui++/point.hpp>
#include <ygui++/color.hpp>
#include <ygui++/font.hpp>
#include <ygui++/bitmap.hpp>

namespace yguipp {

class Window;
class Widget;

class GraphicsContext {
  public:
    friend class Window;
    friend class Widget;

    GraphicsContext( Window* window );
    virtual ~GraphicsContext( void );

    const Point& getLeftTop( void );
    bool needToFlush( void );

    void setPenColor( const Color& pen );
    void setClipRect( const Rect& rect );
    void setFont( Font* font );
    void setDrawingMode( DrawingMode mode );

    void translate( const Point& p );
    void fillRect( const Rect& r );
    void drawRect( const Rect& r );
    void drawBitmap( const Point& p, Bitmap* bitmap );
    void drawText( const Point& p, const std::string& text, int length = -1 );

    void finish( void );

  private:
    enum TranslateType {
        CHECKPOINT,
        TRANSLATE
    };

    struct TranslateItem {
        TranslateItem( void ) : m_type(CHECKPOINT) {}
        TranslateItem( const Point& p ) : m_type(TRANSLATE), m_point(p) {}

        TranslateType m_type;
        Point m_point;
    };

  private:
    void pushRestrictedArea( const Rect& rect );
    void popRestrictedArea( void );
    const Rect& currentRestrictedArea( void );

    void translateCheckPoint( void );
    void rollbackTranslate( void );

    void cleanUp( void );

  private:
    Point m_leftTop;
    Rect m_clipRect;
    bool m_penValid;
    Color m_penColor;

    bool m_needToFlush;

    std::stack<Rect> m_restrictedAreas;
    std::stack<TranslateItem> m_translateStack;

    Window* m_window;
}; /* class GraphicsContext */

} /* namespace yguipp */

#endif /* _GRAPHICSCONTEXT_HPP_ */
