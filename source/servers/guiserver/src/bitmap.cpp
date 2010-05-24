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

Bitmap::Bitmap( uint32_t width, uint32_t height, color_space_t colorSpace, void* buffer ) : m_width(width), m_height(height), m_colorSpace(colorSpace) {
}

yguipp::Rect Bitmap::bounds( void ) {
    return yguipp::Rect(0, 0, m_width - 1, m_height - 1);
}
