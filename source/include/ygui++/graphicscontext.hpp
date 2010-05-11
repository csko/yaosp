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

#include <ygui++/rect.hpp>
#include <ygui++/point.hpp>
#include <ygui++/color.hpp>

namespace yguipp {

class Window;

class GraphicsContext {
  public:
    GraphicsContext( Window* window );
    virtual ~GraphicsContext( void );

    void setPenColor( const Color& pen );
    void setClipRect( const Rect& rect );

    void fillRect( const Rect& r );

    void flush( void );

  private:
    Point m_leftTop;
    Rect m_clipRect;
    bool m_penValid;
    Color m_penColor;

    bool m_needToFlush;

    Window* m_window;
}; /* class GraphicsContext */

} /* namespace yguipp */

#endif /* _GRAPHICSCONTEXT_HPP_ */
