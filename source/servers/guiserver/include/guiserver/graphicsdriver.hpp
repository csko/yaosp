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

#ifndef _GRAPHICSDRIVER_HPP_
#define _GRAPHICSDRIVER_HPP_

#include <string>
#include <ygui/yconstants.h>
#include <ygui++/rect.hpp>
#include <ygui++/color.hpp>

#include <guiserver/bitmap.hpp>

struct ScreenMode {
    ScreenMode( uint32_t width, uint32_t height, color_space_t colorSpace ) : m_width(width), m_height(height),
                                                                              m_colorSpace(colorSpace) {}

    bool operator==( const ScreenMode& mode ) {
        return ( ( m_width == mode.m_width ) &&
                 ( m_height == mode.m_height ) &&
                 ( m_colorSpace == mode.m_colorSpace ) );
    }

    uint32_t m_width;
    uint32_t m_height;
    color_space_t m_colorSpace;
};

class GraphicsDriver {
  public:
    virtual ~GraphicsDriver( void ) {}

    virtual std::string getName( void ) = 0;
    virtual bool detect( void ) = 0;
    virtual size_t getModeCount( void ) = 0;
    virtual ScreenMode* getModeInfo( size_t index ) = 0;
    virtual bool setMode( ScreenMode* info ) = 0;
    virtual void* getFrameBuffer( void ) = 0;
    virtual int fillRect( Bitmap* bitmap, const yguipp::Rect& clipRect, const yguipp::Rect& rect,
                          const yguipp::Color& color, drawing_mode_t mode );
    //virtual int drawText( Bitmap* bitmap, const yguipp::Rect& clipRect, const yguipp::Point& position, const yguipp::Color& color, FontNode* font, const char* text, int length );
    virtual int blitBitmap( Bitmap* dest, const yguipp::Point& point, Bitmap* src,
                            const yguipp::Rect& rect, drawing_mode_t mode );
}; /* class GraphicsDriver */

#endif /* _GRAPHICSDRIVER_HPP_ */
