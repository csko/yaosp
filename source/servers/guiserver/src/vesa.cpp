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

#include <fcntl.h>

#include <guiserver/graphicsdriver.hpp>

VesaDriver::VesaDriver( void ) : m_device(-1) {
}

VesaDriver::~VesaDriver( void ) {
}

std::string VesaDriver::getName( void ) {
    return "VESA";
}

bool VesaDriver::detect( void ) {
    m_device = open( "/device/video/vesa", O_RDONLY );

    if ( m_device < 0 ) {
        return false;
    }

    return true;
}

size_t VesaDriver::getModeCount( void ) {
    return 0;
}

int VesaDriver::getModeInfo( size_t index, ScreenMode* info ) {
    return 0;
}

int VesaDriver::setMode( ScreenMode* info ) {
    return 0;
}

int VesaDriver::fillRect( Bitmap* bitmap, const yguipp::Rect& clipRect, const yguipp::Rect& rect, const yguipp::Color& color, drawing_mode_t mode ) {
    return 0;
}

/*int VesaDriver::drawText( Bitmap* bitmap, const yguipp::Rect& clipRect, const yguipp::Point& position, const yguipp::Color& color, FontNode* font, const char* text, int length ) {
    return 0;
    }*/

int VesaDriver::blitBitmap( Bitmap* dest, const yguipp::Point& point, Bitmap* src, const yguipp::Rect& rect, drawing_mode_t mode ) {
    return 0;
}
