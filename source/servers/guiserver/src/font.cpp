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

FontNode::FontNode(FontStyle* style) : m_style(style) {
}

FontGlyph* FontNode::getGlyph(int c) {
    // todo
    return NULL;
}

FontStyle::FontStyle(FT_Face face) : m_face(face), m_mutex("style_lock") {
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
                    continue;
                }
            }
        }

        FT_Done_Face( face );
    }

    return true;
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
