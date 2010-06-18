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

#ifndef _APPLICATION_HPP_
#define _APPLICATION_HPP_

#include <map>
#include <string>

#include <ygui++/imageloader.hpp>
#include <ygui++/point.hpp>
#include <yutil++/mutex.hpp>
#include <yutil++/ipcport.hpp>

namespace yguipp {

class Window;

class Application {
  private:
    friend class Window;

    Application( const std::string& name );
    ~Application( void );

    bool init( void );

  public:
    void lock( void );
    void unLock( void );

    Point getDesktopSize(int desktopIndex = 0);

    yutilpp::IPCPort* getGuiServerPort( void );
    yutilpp::IPCPort* getClientPort( void );
    yutilpp::IPCPort* getReplyPort( void );
    inline yutilpp::IPCPort* getServerPort( void ) { return m_serverPort; }

    int ipcDataAvailable( uint32_t code, void* buffer, size_t size );

    int mainLoop( void );

    static bool createInstance( const std::string& name );
    static inline Application* getInstance(void) { return m_instance; }

  private:
    bool registerApplication( void );
    int registerWindow( int id, Window* window );

  private:
    static const size_t IPC_BUF_SIZE = 512;

    yutilpp::Mutex* m_lock;

    yutilpp::IPCPort* m_guiServerPort;
    yutilpp::IPCPort* m_clientPort;
    yutilpp::IPCPort* m_serverPort;
    yutilpp::IPCPort* m_replyPort;

    uint8_t m_ipcBuffer[IPC_BUF_SIZE];

    typedef std::map<int, Window*> WindowMap;
    typedef WindowMap::const_iterator WindowMapCIter;

    WindowMap m_windowMap;

    static Application* m_instance;
}; /* class Application */

} /* namespace yguipp */

#endif /* _APPLICATION_HPP_ */
