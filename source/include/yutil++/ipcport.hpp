/* yaosp IPC port implementation
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

#ifndef _IPCPORT_H_
#define _IPCPORT_H_

#include <string>

#include <yaosp/ipc.h>

namespace yutilpp {

class IPCPort {
  public:
    IPCPort( void );
    ~IPCPort( void );

    bool createNew( void );
    bool createFromExisting( ipc_port_id id );
    bool createFromNamed( const std::string& name );

    ipc_port_id getId( void );

    int send( uint32_t code, void* data = NULL, size_t size = 0 );
    int receive( uint32_t& code, void* data = NULL, size_t maxSize = 0, uint64_t timeOut = INFINITE_TIMEOUT );

    bool registerAsNamed( const std::string& name );

    static int sendTo( ipc_port_id id, uint32_t code, void* data = NULL, size_t size = 0 );

  private:
    bool m_canSend;
    bool m_canReceive;
    ipc_port_id m_id;
}; /* class IPCPort */

} /* namespace yutilpp */

#endif /* _IPCPORT_H_ */
