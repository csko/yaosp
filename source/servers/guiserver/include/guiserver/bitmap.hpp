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

#include <cairo/cairo.h>

#include <yaosp/region.h>

#include <ygui++/rect.hpp>
#include <ygui++/yconstants.hpp>

class Bitmap {
  public:
    enum {
        FREE_BUFFER = (1 << 0),
        SCREEN = (1 << 1),
        VIDEO_MEMORY = (1 << 2)
    };

    Bitmap(uint32_t width, uint32_t height, yguipp::ColorSpace colorSpace,
            uint8_t* buffer = NULL, region_id region = -1);
    ~Bitmap(void);

  public:
    inline cairo_surface_t* getSurface(void) { return m_surface; }

    inline bool hasFlag(uint32_t flag) { return ((m_flags & flag) != 0); }
    inline void addFlag(uint32_t flag) { m_flags |= flag; }
    inline void removeFlag(uint32_t flag) { m_flags &= ~flag; }

    inline uint32_t width( void ) { return m_width; }
    inline uint32_t height( void ) { return m_height; }

    yguipp::Point size( void );
    yguipp::Rect bounds( void );

    uint8_t* getBuffer( void );
    yguipp::ColorSpace getColorSpace( void );

  private:
    Bitmap(const Bitmap& b);
    void operator=(const Bitmap& b);

  private:
    uint32_t m_width;
    uint32_t m_height;
    yguipp::ColorSpace m_colorSpace;
    uint8_t* m_buffer;
    uint32_t m_flags;
    region_id m_region;

    cairo_surface_t* m_surface;
}; /* class Bitmap */

#endif /* _BITMAP_HPP_ */
