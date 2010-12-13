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
#include <cairo/cairo.h>

class ScaledFont {
  public:
    ScaledFont(cairo_scaled_font_t* font);
    ~ScaledFont(void);

    cairo_scaled_font_t* getCairoFont(void);

    int getAscent(void);
    int getDescent(void);
    int getHeight(void);

    int getWidth(const std::string& s);

  private:
    cairo_scaled_font_t* m_font;
    cairo_font_extents_t m_fontExtents;
}; /* class ScaledFont */

class FontStorage {
  public:
    FontStorage(void);
    ~FontStorage(void);

    bool init(void);
    bool loadFonts(void);

    cairo_font_face_t* getCairoFontFace(const std::string& family, const std::string& style);
    ScaledFont* getScaledFont(const std::string& family, const std::string& style, int pointSize);

  private:
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
    std::map<FontInfo, FontData*> m_fonts;
}; /* class FontStorage */

#endif /* _FONT_HPP_ */
