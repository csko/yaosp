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

#include <vector>
#include <cairo/cairo-ft.h>

#include <yaosp/debug.h>
#include <yutil++/storage/directory.hpp>

#include <guiserver/font.hpp>

ScaledFont::ScaledFont(cairo_scaled_font_t* font) : m_font(font) {
    cairo_scaled_font_extents(m_font, &m_fontExtents);
}

ScaledFont::~ScaledFont(void) {
    cairo_scaled_font_destroy(m_font);
}

cairo_scaled_font_t* ScaledFont::getCairoFont(void) {
    return m_font;
}

int ScaledFont::getAscent(void) {
    return m_fontExtents.ascent;
}

int ScaledFont::getDescent(void) {
    return m_fontExtents.descent;
}

int ScaledFont::getHeight(void) {
    return m_fontExtents.height;
}

int ScaledFont::getWidth(const std::string& s) {
    cairo_text_extents_t extents;
    cairo_scaled_font_text_extents(m_font, s.c_str(), &extents);
    return extents.x_advance;
}

FontStorage::FontStorage(void) {
}

FontStorage::~FontStorage(void) {
}

bool FontStorage::init(void) {
    FT_Error error;
    error = FT_Init_FreeType(&m_ftLibrary);
    return (error == 0);
}

bool FontStorage::loadFonts(void) {
    std::string entry;
    std::vector<std::string> fontFiles;
    yutilpp::storage::Directory* fontDir;

    /* Create a list of available font files. */
    fontDir = new yutilpp::storage::Directory("/yaosp/system/fonts");
    fontDir->init();

    while ( fontDir->nextEntry(entry) ) {
        if ( (entry == ".") ||
             (entry == "..") ) {
            continue;
        }

        fontFiles.push_back("/yaosp/system/fonts/" + entry);
    }

    delete fontDir;

    /* Load fonts one-by-one. */
    for ( std::vector<std::string>::const_iterator it = fontFiles.begin();
          it != fontFiles.end();
          ++it ) {
        FT_Face face;
        FT_Error error;

        error = FT_New_Face(m_ftLibrary, (*it).c_str(), 0, &face);

        if (error != 0) {
            dbprintf("FontStorage::loadFonts(): failed to load %s\n", (*it).c_str());
            continue;
        }

        for ( int i = 0; i < face->num_charmaps; i++ ) {
            FT_CharMap charMap = face->charmaps[i];

            if ( (charMap->platform_id == 3) &&
                 (charMap->encoding_id == 1) ) {
                face->charmap = charMap;

                if (loadFontFace(face)) {
                    goto next;
                }
            }
        }

        FT_Done_Face( face );

    next:
        ;
    }

    return true;
}

cairo_font_face_t* FontStorage::getCairoFontFace(const std::string& family, const std::string& style) {
    std::map<FontInfo, FontData*>::const_iterator it = m_fonts.find(FontInfo(family, style));

    if (it == m_fonts.end()) {
        return NULL;
    }

    return it->second->m_cairoFace;
}

ScaledFont* FontStorage::getScaledFont(const std::string& family, const std::string& style, int pointSize) {
    std::map<FontInfo, FontData*>::const_iterator it = m_fonts.find(FontInfo(family, style));

    if (it == m_fonts.end()) {
        return NULL;
    }

    cairo_matrix_t fm;
    cairo_matrix_t ctm;
    cairo_matrix_init_scale(&fm, pointSize, pointSize);
    cairo_matrix_init_identity(&ctm);

    cairo_font_options_t* options = cairo_font_options_create();
    cairo_font_options_set_antialias(options, CAIRO_ANTIALIAS_SUBPIXEL);
    cairo_font_options_set_hint_style(options, CAIRO_HINT_STYLE_FULL);

    cairo_scaled_font_t* scaledFont = cairo_scaled_font_create(it->second->m_cairoFace, &fm, &ctm, options);
    cairo_font_options_destroy(options);

    return new ScaledFont(scaledFont);
}

bool FontStorage::loadFontFace(FT_Face face) {
    cairo_font_face_t* cairoFace = cairo_ft_font_face_create_for_ft_face(face, 0);
    m_fonts[FontInfo(face->family_name, face->style_name)] = new FontData(face, cairoFace);

    return true;
}
