/* yaosp GUI library
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

#include <ygui++/textarea.hpp>
#include <ygui++/text/plaindocument.hpp>

namespace yguipp {

TextArea::TextArea(void) {
    m_document = new yguipp::text::PlainDocument();

    m_font = new yguipp::Font("DejaVu Sans Mono", "Book", yguipp::FontInfo(8));
    m_font->init();
}

TextArea::~TextArea(void) {
    delete m_document;
    delete m_font;
}

int TextArea::paint(GraphicsContext* g) {
    yguipp::Point visibleSize = getVisibleSize();
    yguipp::Point scrollOffset = getScrollOffset();

    g->setPenColor(yguipp::Color(255, 255, 255));
    g->fillRect(yguipp::Rect(visibleSize) - scrollOffset);

    g->setFont(m_font);
    g->setPenColor(yguipp::Color(0, 0, 0));

    yguipp::Point p;
    p.m_y = m_font->getAscender();

    yguipp::text::Element* root = m_document->getRootElement();

    for (size_t i = 0; i < root->getElementCount(); i++) {
        yguipp::text::Element* e = root->getElement(i);
        std::string line = m_document->getText(e->getOffset(), e->getLength());

        g->drawText(p, line);

        p.m_y += m_font->getHeight();
    }

    return 0;
}

} /* namespace yguipp */
