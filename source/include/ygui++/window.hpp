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

#ifndef _WINDOW_HPP_
#define _WINDOW_HPP_

#include <string>
#include <vector>

#include <ygui++/yconstants.hpp>
#include <ygui++/widget.hpp>
#include <ygui++/rendertable.hpp>

namespace yguipp {

class WindowListener {
  public:
    typedef std::vector<WindowListener*> List;
    typedef List::iterator ListIter;
    typedef List::const_iterator ListCIter;

    virtual ~WindowListener(void) {}

    virtual int windowActivated(Window* window, ActivationReason reason) {}
    virtual int windowDeActivated(Window* window, DeActivationReason reason) {}
}; /* class WindowListener */

class Window : public Object {
  public:
    Window( const std::string& title, const Point& position, const Point& size, int flags = WINDOW_NONE );
    virtual ~Window( void );

    bool init( void );

    void addWindowListener(WindowListener* windowListener);
    void removeWindowListener(WindowListener* windowListener);

    const Point& getSize( void );
    const Point& getPosition( void );
    Widget* getContainer( void );
    RenderTable* getRenderTable( void );
    inline int getId(void) { return m_id; }

    void show( void );
    void hide( void );

    void resize( const Point& size );
    void moveTo( const Point& position );

    int handleMessage( uint32_t code, void* buffer, size_t size );

  private:
    bool registerWindow( void );

    void doRepaint( bool forced = false );

    Widget* findWidgetAt( const Point& p );
    Widget* findWidgetAtHelper( Widget* widget, const Point& position,
                                Point leftTop, const Rect& visibleRect );

    Point getWidgetPosition( Widget* widget, Point p );

    int handleMouseEntered(const yguipp::Point& position);
    int handleMouseMoved(const yguipp::Point& position);
    int handleMouseExited(void);
    int handleMousePressed(const yguipp::Point& position, int button);
    int handleMouseReleased(int button);

  private:
    int m_id;

    std::string m_title;
    Point m_position;
    Point m_size;
    int m_flags;

    Widget* m_container;
    Widget* m_mouseWidget;
    Widget* m_mouseDownWidget;

    RenderTable* m_renderTable;
    GraphicsContext* m_graphicsContext;

    WindowListener::List m_windowListeners;
}; /* class Window */

} /* namespace yguipp */

#endif /* _WINDOW_HPP_ */
