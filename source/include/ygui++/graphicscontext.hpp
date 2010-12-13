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

#ifndef _YGUIPP_GRAPHICSCONTEXT_HPP_
#define _YGUIPP_GRAPHICSCONTEXT_HPP_

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
class RenderTable;

class GraphicsContext {
  public:
    friend class Window;
    friend class Widget;

    GraphicsContext( Window* window );
    virtual ~GraphicsContext( void );

  private:
    GraphicsContext(const GraphicsContext& gc);
    GraphicsContext& operator=(const GraphicsContext& gc);

  public:
    const Point& getLeftTop( void );

    void setPenColor(const Color& pen);
    void setLineWidth(double width);
    void setFont(Font* font);
    void setClipRect(const Rect& rect);
    void setAntiAlias(AntiAliasMode mode);

    void translate(const Point& p);
    void moveTo(const Point& p);
    void lineTo(const Point& p);
    void rectangle(const Rect& r);
    void arc(const Point& center, double radius, double angle1, double angle2);

    void closePath(void);

    void stroke(void);
    void fill(void);
    void fillPreserve(void);
    void showText(const std::string& text);
    void showBitmap(const Point& p, Bitmap* bitmap);

    void finish( void );

  private:
    enum TranslateType {
        CHECKPOINT,
        TRANSLATE
    }; /* enum TranslateType */

    struct TranslateItem {
        TranslateItem( void ) : m_type(CHECKPOINT) {}
        TranslateItem( const Point& p ) : m_type(TRANSLATE), m_point(p) {}

        TranslateType m_type;
        Point m_point;
    }; /* struct TranslateItem */

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

    std::stack<Rect> m_restrictedAreas;
    std::stack<TranslateItem> m_translateStack;

    Window* m_window;
    RenderTable* m_renderTable;
}; /* class GraphicsContext */

} /* namespace yguipp */

#endif /* _YGUIPP_GRAPHICSCONTEXT_HPP_ */
