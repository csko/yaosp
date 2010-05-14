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

#ifndef _WIDGET_HPP_
#define _WIDGET_HPP_

#include <vector>
#include <ygui++/object.hpp>
#include <ygui++/graphicscontext.hpp>
#include <ygui++/layout/layout.hpp>

namespace yguipp {

class Window;

class Widget : public Object {
  public:
    friend class Window;

    typedef std::pair<Widget*,layout::LayoutData*> Child;
    typedef std::vector<Child> ChildVector;
    typedef ChildVector::const_iterator ChildVectorCIter;

    Widget( void );
    virtual ~Widget( void );

    void addChild( Widget* child, layout::LayoutData* data = NULL );

    const Point& getPosition( void );
    const Point& getScrollOffset( void );
    const Point& getSize( void );
    const Point& getVisibleSize( void );
    const Rect getBounds( void );
    const ChildVector& getChildren( void );

    void setWindow( Window* window );
    void setPosition( const Point& p );
    void setSize( const Point& p );

    void invalidate( void );

    virtual Point getPreferredSize( void );
    virtual Point getMaximumSize( void );

    virtual int validate( void );
    virtual int paint( GraphicsContext* g );

    virtual int mouseEntered( const Point& p );
    virtual int mouseMoved( const Point& p );
    virtual int mouseExited( void );
    virtual int mousePressed( const Point& p );
    virtual int mouseReleased( void );

  private:
    int doPaint( GraphicsContext* g );
    int doInvalidate( bool notifyWindow );

  private:
    Window* m_window;
    Widget* m_parent;

    bool m_isValid;

    Point m_position;
    Point m_scrollOffset;
    Point m_fullSize;
    Point m_visibleSize;

    ChildVector m_children;
}; /* class Widget */

} /* namespace yguipp */

#endif /* _WIDGET_HPP_ */
