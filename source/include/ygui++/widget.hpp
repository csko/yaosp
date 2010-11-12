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
#include <ygui++/border/border.hpp>
#include <ygui++/event/keylistener.hpp>

namespace yguipp {

class Window;

class Widget : public Object, public event::KeySpeaker {
  public:
    friend class Window;

    typedef std::pair<Widget*, const layout::LayoutData*> Child;
    typedef std::vector<Child> ChildVector;
    typedef ChildVector::const_iterator ChildVectorCIter;

    Widget( void );
    virtual ~Widget( void );

    virtual void add( Widget* child, const layout::LayoutData* data = NULL );

    Widget* getParent( void );
    const Point& getPosition( void );
    const Point& getScrollOffset( void );
    const Point& getSize( void );
    const Point& getVisibleSize( void );
    const Rect getBounds( void );
    const ChildVector& getChildren( void );
    Window* getWindow( void );

    void setWindow( Window* window );
    void setBorder( border::Border* border );
    void setPosition(const Point& p);
    void setScrollOffset(const Point& p);
    void setSize(const Point& s);
    void setSizes(const Point& visibleSize, const Point& fullSize);

    void invalidate( void );

    virtual Point getPreferredSize( void );
    virtual Point getMaximumSize( void );

    virtual int validate( void );
    virtual int paint( GraphicsContext* g );

    virtual int keyPressed(int key);
    virtual int keyReleased(int key);
    virtual int mouseEntered( const Point& p );
    virtual int mouseMoved( const Point& p );
    virtual int mouseExited( void );
    virtual int mousePressed( const Point& p, int button );
    virtual int mouseReleased( int button );

  private:
    int doPaint( GraphicsContext* g, bool forced );
    int doInvalidate( bool notifyWindow );

  private:
    Window* m_window;
    Widget* m_parent;
    border::Border* m_border;

    bool m_isValid;

    Point m_position;
    Point m_scrollOffset;
    Point m_fullSize;
    Point m_visibleSize;

  protected:
    ChildVector m_children;
}; /* class Widget */

} /* namespace yguipp */

#endif /* _WIDGET_HPP_ */
