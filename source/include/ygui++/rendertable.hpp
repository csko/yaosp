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

#ifndef _RENDERTABLE_HPP_
#define _RENDERTABLE_HPP_

#include <stddef.h>

namespace yguipp {

class Window;

class RenderTable {
  public:
    RenderTable( Window* window );
    virtual ~RenderTable( void );

    virtual void* allocate( size_t size ) = 0;
    virtual int reset( void ) = 0;
    virtual int flush( void ) = 0;
    virtual int waitForFlush( void ) = 0;

  protected:
    Window* m_window;
}; /* class RenderTable */

} /* namespace yguipp */

#endif /* _RENDERTABLE_HPP_ */
