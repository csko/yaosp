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

#ifndef _APPLICATION_HPP_
#define _APPLICATION_HPP_

#include <map>

#include <ygui++/protocol.hpp>
#include <yutil++/ipclistener.hpp>

class Window;

class Application : public yutilpp::IPCListener {
  private:
    Application( void );
    ~Application( void );

    bool init( AppCreate* request );

  public:
    int ipcDataAvailable( uint32_t code, void* data, size_t size );

    static Application* createFrom( AppCreate* request );

  private:
    int handleWindowCreate( WinCreate* request );
    int getWindowId( void );

  private:
    yutilpp::IPCPort* m_clientPort;

    typedef std::map<int, Window*> WindowMap;
    typedef WindowMap::const_iterator WindowMapCIter;

    int m_nextWinId;
    WindowMap m_windowMap;
}; /* class Application */

#endif /* _APPLICATION_HPP_ */
