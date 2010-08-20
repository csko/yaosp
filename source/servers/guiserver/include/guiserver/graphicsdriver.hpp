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
#include <ygui++/rect.hpp>
#include <ygui++/color.hpp>

#include <guiserver/bitmap.hpp>
#include <guiserver/font.hpp>

struct ScreenMode {
    ScreenMode( uint32_t width, uint32_t height, yguipp::ColorSpace colorSpace ) : m_width(width), m_height(height),
                                                                                   m_colorSpace(colorSpace) {}

    bool operator==( const ScreenMode& mode ) {
        return ((m_width == mode.m_width) &&
                (m_height == mode.m_height) &&
                (m_colorSpace == mode.m_colorSpace));
    }

    uint32_t m_width;
    uint32_t m_height;
    yguipp::ColorSpace m_colorSpace;
}; /* struct ScreenMode */

class GraphicsDriver {
  public:
    virtual ~GraphicsDriver(void) {}

    virtual bool detect(void) = 0;
    virtual bool initialize(void) = 0;

    virtual std::string getName( void ) = 0;
    virtual size_t getModeCount( void ) = 0;
    virtual ScreenMode* getModeInfo( size_t index ) = 0;
    virtual void* getFrameBuffer( void ) = 0;

    virtual bool setMode( ScreenMode* info ) = 0;

    virtual int drawRect( Bitmap* bitmap, const yguipp::Rect& clipRect, const yguipp::Rect& rect,
                          const yguipp::Color& color, yguipp::DrawingMode mode );
    virtual int fillRect( Bitmap* bitmap, const yguipp::Rect& clipRect, const yguipp::Rect& rect,
                          const yguipp::Color& color, yguipp::DrawingMode mode );
    virtual int drawText( Bitmap* bitmap, const yguipp::Rect& clipRect, const yguipp::Point& position,
                          const yguipp::Color& color, FontNode* font, const char* text, int length );
    virtual int blitBitmap( Bitmap* dest, const yguipp::Point& point, Bitmap* src,
                            const yguipp::Rect& rect, yguipp::DrawingMode mode );

  private:
    int drawRectCopy( Bitmap* bitmap, const yguipp::Rect& rect, const yguipp::Color& color );
    int drawRectCopy32( Bitmap* bitmap, const yguipp::Rect& rect, const yguipp::Color& color );

    int drawRectInvert( Bitmap* bitmap, const yguipp::Rect& rect );
    int drawRectInvert32( Bitmap* bitmap, const yguipp::Rect& rect );

    int fillRectCopy( Bitmap* bitmap, const yguipp::Rect& rect, const yguipp::Color& color );
    int fillRectCopy32( Bitmap* bitmap, const yguipp::Rect& rect, const yguipp::Color& color );

    int fillRectInvert32( Bitmap* bitmap, const yguipp::Rect& rect );

    int drawText32( Bitmap* bitmap, const yguipp::Rect& clipRect, yguipp::Point position,
                    const yguipp::Color& color, FontNode* font, const char* text, int length );
    int renderGlyph32( Bitmap* bitmap, const yguipp::Rect& clipRect, const yguipp::Point& position,
                       const yguipp::Color& color, FontGlyph* glyph );

    int blitBitmapCopy( Bitmap* dest, const yguipp::Point& point, Bitmap* src, const yguipp::Rect& rect );
    int blitBitmapCopy32( Bitmap* dest, const yguipp::Point& point, Bitmap* src, const yguipp::Rect& rect );
    int blitBitmapBlend( Bitmap* dest, const yguipp::Point& point, Bitmap* src, const yguipp::Rect& rect );
    int blitBitmapBlend32( Bitmap* dest, const yguipp::Point& point, Bitmap* src, const yguipp::Rect& rect );
}; /* class GraphicsDriver */

#endif /* _GRAPHICSDRIVER_HPP_ */
