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

#include <ygui++/image.hpp>

namespace yguipp {

Image::Image( Bitmap* bitmap ) : m_bitmap(bitmap) {
    if ( m_bitmap != NULL ) {
        m_bitmap->incRef();
    }
}

Image::~Image( void ) {
    if ( m_bitmap != NULL ) {
        m_bitmap->decRef();
    }
}

Point Image::getPreferredSize( void ) {
    if ( m_bitmap != NULL ) {
        return m_bitmap->getSize();
    }

    return Point(0,0);
}

int Image::paint( GraphicsContext* g ) {
    if ( m_bitmap != NULL ) {
        g->drawBitmap( ( getSize() - m_bitmap->getSize() ) / 2, m_bitmap );
    }

    return 0;
}

} /* namespace yguipp */
