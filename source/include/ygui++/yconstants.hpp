/* yaOSp GUI library
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

#ifndef _YCONSTANTS_HPP_
#define _YCONSTANTS_HPP_

enum DrawingMode {
    DM_COPY,
    DM_BLEND,
    DM_INVERT
};

enum ColorSpace {
    CS_UNKNOWN,
    CS_RGB16,
    CS_RGB24,
    CS_RGB32
};

enum {
    WINDOW_NONE = 0,
    WINDOW_NO_BORDER = (1 << 0),
    WINDOW_FIXED_SIZE = (1 << 1)
};

enum WindowOrder {
    WINDOW_ORDER_ALWAYS_ON_BOTTOM,
    WINDOW_ORDER_BOTTOM,
    WINDOW_ORDER_NORMAL,
    WINDOW_ORDER_TOP,
    WINDOW_ORDER_ALWAYS_ON_TOP
};

enum HAlignment {
    H_ALIGN_LEFT,
    H_ALIGN_CENTER,
    H_ALIGN_RIGHT
};

enum VAlignment {
    V_ALIGN_TOP,
    V_ALIGN_CENTER,
    V_ALIGN_BOTTOM
};

enum FontFlags {
    FONT_SMOOTHED = (1 << 0)
};

struct FontInfo {
    FontInfo(int pointSize, int flags) : m_pointSize(pointSize * 64), m_flags(flags) {}

    int m_pointSize;
    int m_flags;

    bool operator<(const FontInfo& info) const {
        return ( (m_pointSize < info.m_pointSize) &&
                 (m_flags < info.m_flags) );
    }
};

static inline int colorspace_to_bpp( ColorSpace colorSpace ) {
    switch (colorSpace) {
        case CS_RGB16 : return 2;
        case CS_RGB24 : return 3;
        case CS_RGB32 : return 4;
        case CS_UNKNOWN :
        default :
            return 0;
    }
}

static inline ColorSpace bpp_to_colorspace( int bitsPerPixel ) {
    switch (bitsPerPixel) {
        case 16 : return CS_RGB16;
        case 24 : return CS_RGB24;
        case 32 : return CS_RGB32;
        default : return CS_UNKNOWN;
    }
}

#endif /* _YCONSTANTS_HPP_ */
