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

#include <guiserver/bitmap.hpp>

Bitmap::Bitmap( uint32_t width, uint32_t height, color_space_t colorSpace,
    uint8_t* buffer ) : m_width(width), m_height(height), m_colorSpace(colorSpace), m_flags(0) {
    if ( buffer == NULL ) {
        m_buffer = new uint8_t[m_width * m_height * colorspace_to_bpp(m_colorSpace) * 8];
        m_flags |= FREE_BUFFER;
    } else {
        m_buffer = buffer;
    }
}

Bitmap::~Bitmap( void ) {
    if ( m_flags & FREE_BUFFER ) {
        delete[] m_buffer;
    }
}

yguipp::Rect Bitmap::bounds( void ) {
    return yguipp::Rect(0, 0, m_width - 1, m_height - 1);
}
