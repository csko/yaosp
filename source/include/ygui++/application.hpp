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

#include <string>

#include <yutil++/ipclistener.hpp>

namespace yguipp {

class Application : public yutilpp::IPCListener {
  private:
    Application( const std::string& name );
    ~Application( void );

    bool init( void );

  public:
    yutilpp::IPCPort* getGuiServerPort( void );
    yutilpp::IPCPort* getApplicationPort( void );

    int ipcDataAvailable( uint32_t code, void* buffer, size_t size );

    static bool createInstance( const std::string& name );
    static Application* getInstance( void );

  private:
    bool registerApplication( void );

  private:
    yutilpp::IPCPort* m_guiServerPort;
    yutilpp::IPCPort* m_serverPort;
    yutilpp::IPCPort* m_replyPort;

    static Application* m_instance;
};

} /* namespace yguipp */

#endif /* _APPLICATION_HPP_ */
