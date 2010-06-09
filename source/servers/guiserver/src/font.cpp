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

#include <yaosp/debug.h>
#include <yutil++/storage/directory.hpp>

#include <guiserver/font.hpp>

FontGlyph::FontGlyph(const yguipp::Rect& bounds, const yguipp::Point& advance, int rasterSize) : m_bounds(bounds),
                                                                                                 m_advance(advance) {
    m_raster = new uint8_t[rasterSize];
}

FontGlyph::~FontGlyph(void) {
    delete[] m_raster;
}

FontNode::FontNode(FontStyle* style, const yguipp::FontInfo& info) : m_style(style), m_info(info) {
    m_glyphTable = new FontGlyph*[style->getGlyphCount()];

    for ( int i = 0; i < style->getGlyphCount(); i++ ) {
        m_glyphTable[i] = NULL;
    }

    FT_Size size = setFaceSize();

    if (size->metrics.descender > 0) {
        m_descender = -(size->metrics.descender + 63) / 64;
    } else {
        m_descender = (size->metrics.descender + 63) / 64;
    }

    m_ascender = (size->metrics.ascender + 63) / 64;
    m_lineGap = (size->metrics.height + 63) / 64 - (m_ascender - m_descender);
    m_advance = (size->metrics.max_advance + 63) / 64;
}

int FontNode::getWidth(const char* text, int length) {
    int width = 0;

    m_style->lock();

    while (length > 0) {
        int charLength = FontNode::utf8CharLength(*text);

        if (charLength > length) {
            break;
        }

        FontGlyph* glyph = getGlyph( FontNode::utf8ToUnicode(text) );

        text += charLength;
        length -= charLength;

        width += glyph->getAdvance().m_x;
    }

    m_style->unLock();

    return width;
}

FontGlyph* FontNode::getGlyph(int c) {
    int top;
    int left;
    int index;
    FT_Face face;
    FT_Error error;
    int rasterSize;
    FT_GlyphSlot glyph;
    FontGlyph* fontGlyph;

    face = m_style->getFace();
    index = FT_Get_Char_Index(face, c);

    if ( (index < 0) ||
         (index >= m_style->getGlyphCount()) ) {
        return NULL;
    }

    if ( m_glyphTable[index] != NULL ) {
        return m_glyphTable[index];
    }

    setFaceSize();
    FT_Set_Transform(face, NULL, NULL);

    error = FT_Load_Glyph(face, index, FT_LOAD_DEFAULT);

    if (error != 0) {
        dbprintf( "FontNode::getGlyph(): unable to load glyph char=%u index=%d (error=%d)\n", c, index, error);
        return NULL;
    }

    glyph = face->glyph;

    if (m_style->isScalable()) {
        top = -((glyph->metrics.horiBearingY + 63) & -64) / 64;
        left = (glyph->metrics.horiBearingX & -64) / 64;
    } else {
        top = -glyph->bitmap_top;
        left = glyph->bitmap_left;
    }

    if (m_style->isScalable()) {
        if (m_info.m_flags & yguipp::FONT_SMOOTHED) {
            error = FT_Render_Glyph(glyph, FT_RENDER_MODE_NORMAL);
        } else {
            error = FT_Render_Glyph(glyph, FT_RENDER_MODE_MONO);
        }

        if (error != 0) {
          dbprintf("FontNode::getGlyph(): failed to render glyph: 0x%x\n", error);
          return NULL;
        }
    }

    if ( (glyph->bitmap.width < 0) ||
         (glyph->bitmap.rows < 0) ||
         (glyph->bitmap.pitch < 0) ) {
        dbprintf(
            "FontNode::getGlyph(): glyph got invalid size %dx%d (%d)\n",
            glyph->bitmap.width, glyph->bitmap.rows, glyph->bitmap.pitch
        );
        return NULL;
    }

    switch (glyph->bitmap.pixel_mode) {
        case ft_pixel_mode_grays :
            rasterSize = glyph->bitmap.pitch * glyph->bitmap.rows;
            break;

        case ft_pixel_mode_mono :
            rasterSize = glyph->bitmap.width * glyph->bitmap.rows;
            break;

        default :
            dbprintf("FontNode::getGlyph(): unknown pixel mode: %d\n", glyph->bitmap.pixel_mode);
            return NULL;
    }

    yguipp::Rect bounds;
    yguipp::Point advance;

    bounds = yguipp::Rect(left,top,left + glyph->bitmap.width - 1, top + glyph->bitmap.rows - 1);
    if (m_style->isScalable()) {
        if (m_style->isFixedWidth()) {
            advance = yguipp::Point(m_advance,0);
        } else {
            advance = yguipp::Point(
                (glyph->metrics.horiAdvance + 32) / 64,
                (glyph->metrics.vertAdvance + 32) / 64
            );
        }
    } else {
        advance = yguipp::Point(glyph->bitmap.width,0);
    }

    fontGlyph = new FontGlyph(bounds, advance, rasterSize);
    m_glyphTable[index] = fontGlyph;

    switch (glyph->bitmap.pixel_mode) {
        case ft_pixel_mode_grays :
            fontGlyph->setBytesPerLine(glyph->bitmap.pitch);
            memcpy(fontGlyph->getRaster(), glyph->bitmap.buffer, rasterSize);
            break;

        case ft_pixel_mode_mono : {
            register uint8_t* raster = fontGlyph->getRaster();
            fontGlyph->setBytesPerLine(glyph->bitmap.width);

            for ( int y = 0; y < bounds.height(); ++y ) {
                for ( int x = 0; x < bounds.width(); ++x ) {
                    if ( glyph->bitmap.buffer[x / 8 + y * glyph->bitmap.pitch] & (1 << (7 - (x % 8))) ) {
                        raster[x + y * fontGlyph->getBytesPerLine()] = 255;
                    } else {
                        raster[x + y * fontGlyph->getBytesPerLine()] = 0;
                    }
                }
            }
        }
    }

    return fontGlyph;
}

FT_Size FontNode::setFaceSize(void) {
    FT_Face face = m_style->getFace();

    if (m_style->isScalable()) {
        FT_Set_Char_Size(face, m_info.m_pointSize, m_info.m_pointSize, 96, 96);
    } else {
        FT_Set_Pixel_Sizes(face, 0, (m_info.m_pointSize * 96 / 72) / 64);
    }

    return face->size;
}

FontStyle::FontStyle(FT_Face face) : m_face(face), m_mutex("style_lock") {
    m_glyphCount = face->num_glyphs;
    m_scalable = ((face->face_flags & FT_FACE_FLAG_SCALABLE) != 0);
    m_fixedWidth = ((face->face_flags & FT_FACE_FLAG_FIXED_WIDTH) != 0);
}

FontNode* FontStyle::getNode(const yguipp::FontInfo& info) {
    FontNode* node;
    std::map<yguipp::FontInfo, FontNode*>::const_iterator it = m_nodes.find(info);

    if (it == m_nodes.end()) {
        node = new FontNode(this, info);
        m_nodes[info] = node;
    } else {
        node = it->second;
    }

    return node;
}

FontFamily::FontFamily(void) {
}

void FontFamily::addStyle(const std::string& styleName, FontStyle* style) {
    m_styles[styleName] = style;
}

FontStyle* FontFamily::getStyle(const std::string& styleName) {
    std::map<std::string, FontStyle*>::const_iterator it = m_styles.find(styleName);

    if (it == m_styles.end()) {
        return NULL;
    }

    return it->second;
}

FontStorage::FontStorage(void) {
}

bool FontStorage::init(void) {
    FT_Error error;

    error = FT_Init_FreeType(&m_ftLibrary);

    return ( error == 0 );
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

FontNode* FontStorage::getFontNode(const std::string& family, const std::string& style, const yguipp::FontInfo& info) {
    std::map<std::string, FontFamily*>::const_iterator it = m_families.find(family);

    if (it == m_families.end()) {
        return NULL;
    }

    FontStyle* fontStyle = it->second->getStyle(style);

    if (fontStyle == NULL) {
        return NULL;
    }

    return fontStyle->getNode(info);
}

FontFamily* FontStorage::getFamily(const std::string& familyName) {
    std::map<std::string, FontFamily*>::const_iterator it = m_families.find(familyName);

    if (it == m_families.end()) {
        return NULL;
    }

    return it->second;
}

bool FontStorage::loadFontFace(FT_Face face) {
    FontFamily* family = getFamily(face->family_name);

    if (family == NULL) {
        family = new FontFamily();
        m_families[face->family_name] = family;
    }

    FontStyle* style = family->getStyle(face->style_name);

    if (style != NULL) {
        return false;
    }

    style = new FontStyle(face);
    family->addStyle(face->style_name, style);

    return true;
}
