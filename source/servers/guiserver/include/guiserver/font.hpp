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

#ifndef _FONT_HPP_
#define _FONT_HPP_

#include <map>
#include <string>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_SYNTHESIS_H

#include <yutil++/mutex.hpp>

class FontGlyph {
}; /* class FontGlyph */

class FontStyle;

class FontNode {
  public:
    FontNode(FontStyle* style);

    FontGlyph* getGlyph(int c);
    inline FontStyle* getStyle(void) { return m_style; }

    static inline int utf8CharLength( uint8_t byte ) {
        return (((0xE5000000 >> ( (byte >> 3) & 0x1E) ) & 3) + 1);
    }

    static inline int utf8ToUnicode( const char* text ) {
        if ( (text[0] & 0x80) == 0 ) {
            return *text;
        } else if ( (text[1] & 0xC0) != 0x80 ) {
            return 0xFFFD;
        } else if ( (text[0] & 0x20) == 0 ) {
            return ( ((text[0] & 0x1F) << 6) | (text[1] & 0x3F) );
        } else if ( (text[2] & 0xC0) != 0x80 ) {
            return 0xFFFD;
        } else if ( (text[0] & 0x10) == 0 ) {
            return ( ((text[0] & 0x0F) << 12) | ((text[1] & 0x3F) << 6) | (text[2] & 0x3F) );
        } else if ( (text[3] & 0xC0) != 0x80 ) {
            return 0xFFFD;
        } else {
            int tmp;
            tmp = ((text[0] & 0x07) << 18) | ((text[1] & 0x3F) << 12) | ((text[2] & 0x3F) << 6) | (text[3] & 0x3F);
            return ( ((0xD7C0 + (tmp >> 10)) << 16) | (0xDC00 + (tmp & 0x3FF)) );
        }
    }

  private:
    FontStyle* m_style;
}; /* class FontNode */

class FontStyle {
  public:
    FontStyle(FT_Face face);

    inline void lock(void) { m_mutex.lock(); }
    inline void unLock(void) { m_mutex.unLock(); }

  private:
    FT_Face m_face;
    yutilpp::Mutex m_mutex;
}; /* class FontStyle */

class FontFamily {
  public:
    FontFamily(void);

    void addStyle(const std::string& styleName, FontStyle* style);
    FontStyle* getStyle(const std::string& styleName);

  private:
    std::map<std::string, FontStyle*> m_styles;
}; /* class FontFamily */

class FontStorage {
  public:
    FontStorage(void);

    bool init(void);
    bool loadFonts(void);

  private:
    FontFamily* getFamily(const std::string& familyName);

    bool loadFontFace(FT_Face face);

  private:
    FT_Library m_ftLibrary;
    std::map<std::string, FontFamily*> m_families;
}; /* class FontStorage */

#endif /* _FONT_HPP_ */
