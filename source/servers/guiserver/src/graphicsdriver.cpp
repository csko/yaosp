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

#include <yaosp/debug.h>
#include <guiserver/graphicsdriver.hpp>

int GraphicsDriver::fillRect( Bitmap* bitmap, const yguipp::Rect& clipRect, const yguipp::Rect& rect,
                              const yguipp::Color& color, drawing_mode_t mode ) {
    yguipp::Rect rectToFill = rect;

    rectToFill &= clipRect;
    rectToFill &= bitmap->bounds();

    if ( !rectToFill.isValid() ) {
        return 0;
    }

    switch (mode) {
        case DM_COPY :
            fillRectCopy(bitmap, rectToFill, color);
            break;

        case DM_BLEND :
            break;

        case DM_INVERT :
            break;
    }

    return 0;
}

int GraphicsDriver::blitBitmap( Bitmap* dest, const yguipp::Point& point, Bitmap* src,
                                const yguipp::Rect& rect, drawing_mode_t mode ) {
    return 0;
}

int GraphicsDriver::fillRectCopy( Bitmap* bitmap, const yguipp::Rect& rect, const yguipp::Color& color ) {
    switch ( (int)bitmap->getColorSpace() ) {
        case CS_RGB32 :
            fillRectCopy32(bitmap, rect, color);
            break;
    }

    return 0;
}

int GraphicsDriver::fillRectCopy32( Bitmap* bitmap, const yguipp::Rect& rect, const yguipp::Color& color ) {
    register uint32_t* data;
    uint32_t c = color.toColor32();
    uint32_t padding = bitmap->width() - rect.width();

    data = reinterpret_cast<uint32_t*>( bitmap->getBuffer() + (rect.m_top * bitmap->width() + rect.m_left) );

    for ( int y = 0; y < rect.height(); y++ ) {
        for ( int x = 0; x < rect.width(); x++ ) {
            *data++ = c;
        }

        data += padding;
    }

    return 0;
}
