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

#ifndef _IPCRENDERTABLE_HPP_
#define _IPCRENDERTABLE_HPP_

#include <inttypes.h>

#include <ygui++/rendertable.hpp>

namespace yguipp {

class Window;

class IPCRenderTable : public RenderTable {
  public:
    IPCRenderTable( Window* window );
    virtual ~IPCRenderTable( void );

    void* allocate( size_t size );
    int reset( void );
    int flush( void );

  private:
    uint8_t* m_buffer;
    size_t m_position;

    static const size_t BUFFER_SIZE = 32768;
}; /* class RenderTable */

} /* namespace yguipp */

#endif /* _IPCRENDERTABLE_HPP_ */
