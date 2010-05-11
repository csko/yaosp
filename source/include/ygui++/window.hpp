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

#include <ygui++/widget.hpp>
#include <ygui++/rendertable.hpp>
#include <yutil++/ipclistener.hpp>

namespace yguipp {

class Window : public Object, public yutilpp::IPCListener {
  public:
    Window( const std::string& title, const Point& position, const Point& size );
    virtual ~Window( void );

    bool init( void );

    Widget* getContainer( void );
    RenderTable* getRenderTable( void );
    yutilpp::IPCPort* getServerPort( void );
    yutilpp::IPCPort* getReplyPort( void );

    void show( void );

    int ipcDataAvailable( uint32_t code, void* buffer, size_t size );

  private:
    bool registerWindow( void );

  private:
    std::string m_title;
    Point m_position;
    Point m_size;

    yutilpp::IPCPort* m_serverPort;
    yutilpp::IPCPort* m_replyPort;

    Widget* m_container;
    RenderTable* m_renderTable;
    GraphicsContext* m_graphicsContext;
}; /* class Window */

} /* namespace yguipp */

#endif /* _WINDOW_HPP_ */
