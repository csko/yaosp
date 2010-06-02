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
#include <ygui/protocol.h>
#include <ygui/yconstants.h>

#include <ygui++/widget.hpp>
#include <ygui++/rendertable.hpp>

namespace yguipp {

class Window : public Object {
  public:
    Window( const std::string& title, const Point& position, const Point& size, int flags = WINDOW_NONE );
    virtual ~Window( void );

    bool init( void );

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

    void handleResized( msg_win_resized_t* cmd );
    void handleKeyPressed( msg_key_pressed_t* cmd );
    void handleKeyReleased( msg_key_released_t* cmd );
    void handleMouseEntered( msg_mouse_entered_t* cmd );
    void handleMouseMoved( msg_mouse_moved_t* cmd );
    void handleMouseExited( void );
    void handleMousePressed( msg_mouse_pressed_t* cmd );
    void handleMouseReleased( msg_mouse_released_t* cmd );
    void handleMouseScrolled( msg_mouse_scrolled_t* cmd );
    void handleDoResize( msg_win_do_resize_t* cmd );
    void handleDoMove( msg_win_do_move_t* cmd );
    void handleMoved( msg_win_moved_t* cmd );

    Widget* findWidgetAt( const Point& p );
    Widget* findWidgetAtHelper( Widget* widget, const Point& position,
                                Point leftTop, const Rect& visibleRect );

    Point getWidgetPosition( Widget* widget, Point p );

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
}; /* class Window */

} /* namespace yguipp */

#endif /* _WINDOW_HPP_ */
