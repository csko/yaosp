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

#ifndef _BITMAP_HPP_
#define _BITMAP_HPP_

#include <ygui/yconstants.h>
#include <ygui++/rect.hpp>

class Bitmap {
  public:
    Bitmap( uint32_t width, uint32_t height, color_space_t colorSpace, void* buffer = NULL );

    yguipp::Rect bounds( void );

  private:
    uint32_t m_width;
    uint32_t m_height;
    color_space_t m_colorSpace;
    void* m_buffer;
    uint32_t m_flags;
}; /* class Bitmap */

#endif /* _BITMAP_HPP_ */
