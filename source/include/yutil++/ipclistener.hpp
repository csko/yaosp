/* yaosp IPC listener implementation
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

#ifndef _IPCLISTENER_HPP_
#define _IPCLISTENER_HPP_

#include <yutil++/thread.hpp>
#include <yutil++/ipcport.hpp>

namespace yutilpp {

class IPCListener : public Thread {
  public:
    IPCListener( const std::string& name, size_t bufferSize = 1024 );
    virtual ~IPCListener( void );

    bool init( void );
    IPCPort* getPort( void );

    int run( void );

    virtual int ipcDataAvailable( uint32_t code, void* data, size_t size ) = 0;

  private:
    IPCPort* m_port;
    uint8_t* m_buffer;
    size_t m_bufferSize;
}; /* class IPCListener */

} /* namespace yutilpp */

#endif /* _IPCLISTENER_HPP_ */
