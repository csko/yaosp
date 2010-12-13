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
#include <cairo/cairo.h>

#include <ygui++/yconstants.hpp>
#include <ygui++/rect.hpp>
#include <ygui++/point.hpp>
#include <yutil++/thread/mutex.hpp>

class FontGlyph {
  public:
    FontGlyph(const yguipp::Rect& bounds, const yguipp::Point& advance, int rasterSize);
    ~FontGlyph(void);

    inline const yguipp::Rect& bounds(void) { return m_bounds; }
    inline uint8_t* getRaster(void) { return m_raster; }
    inline int getBytesPerLine(void) { return m_bytesPerLine; }
    inline const yguipp::Point& getAdvance(void) { return m_advance; }

    inline void setBytesPerLine(int bpl) { m_bytesPerLine = bpl; }

  private:
    yguipp::Rect m_bounds;
    yguipp::Point m_advance;
    uint8_t* m_raster;
    int m_bytesPerLine;
}; /* class FontGlyph */

class FontStyle;
class FontStorage;

class FontNode {
  public:
    FontNode(FontStorage* storage, FontStyle* style, const yguipp::FontInfo& info);

    int getWidth(const char* text, int length);
    FontGlyph* getGlyph(int c);

    inline FontStyle* getStyle(void) { return m_style; }
    inline int getAscender(void) { return m_ascender; }
    inline int getDescender(void) { return m_descender; }
    inline int getLineGap(void) { return m_lineGap; }

  public:
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
    FT_Size setFaceSize(void);

  private:
    FontStorage* m_storage;
    FontStyle* m_style;

    yguipp::FontInfo m_info;
    FontGlyph** m_glyphTable;

    int m_ascender;
    int m_descender;
    int m_lineGap;
    int m_advance;
}; /* class FontNode */

class FontStyle {
  public:
    FontStyle(FontStorage* storage, FT_Face face);

    FontNode* getNode(const yguipp::FontInfo& info);
    inline int getGlyphCount(void) { return m_glyphCount; }
    inline FT_Face getFace(void) { return m_face; }

    inline bool isScalable(void) { return m_scalable; }
    inline bool isFixedWidth(void) { return m_fixedWidth; }

    inline void lock(void) { m_mutex.lock(); }
    inline void unLock(void) { m_mutex.unLock(); }

  private:
    FontStorage* m_storage;

    FT_Face m_face;
    int m_glyphCount;
    bool m_scalable;
    bool m_fixedWidth;

    yutilpp::thread::Mutex m_mutex;
    std::map<yguipp::FontInfo, FontNode*> m_nodes;
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
    ~FontStorage(void);

    bool init(void);
    bool loadFonts(void);

    FontNode* getFontNode(const std::string& family, const std::string& style, const yguipp::FontInfo& info);

    void lockFT(void);
    void unLockFT(void);

    cairo_font_face_t* getCairoFontFace(const std::string& family, const std::string& style);

  private:
    FontFamily* getFamily(const std::string& familyName);

    bool loadFontFace(FT_Face face);

  private:
    struct FontInfo {
        FontInfo(const std::string& family, const std::string& style)
            : m_family(family), m_style(style) {}

        bool operator<(const FontInfo& i) const {
            return ((m_family < i.m_family) ||
                    (m_family == i.m_family && m_style < i.m_style));
        }

        std::string m_family;
        std::string m_style;
    }; /* struct FontInfo */

    struct FontData {
        FontData(FT_Face face, cairo_font_face_t* cairoFace)
            : m_face(face), m_cairoFace(cairoFace) {}

        FT_Face m_face;
        cairo_font_face_t* m_cairoFace;
    }; /* struct FontData */

    FT_Library m_ftLibrary;
    yutilpp::thread::Mutex m_ftLock;
    std::map<std::string, FontFamily*> m_families;

    std::map<FontInfo, FontData*> m_fonts;
}; /* class FontStorage */

#endif /* _FONT_HPP_ */
