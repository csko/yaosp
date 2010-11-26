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

#include <yaosp/input.h>

#include <ygui++/textarea.hpp>
#include <ygui++/text/plaindocument.hpp>

namespace yguipp {

TextArea::TextArea(void) {
    m_caretPosition = 0;

    m_document = new yguipp::text::PlainDocument();
    m_document->addDocumentListener(this);

    m_font = new yguipp::Font("DejaVu Sans", "Book", yguipp::FontInfo(8));
    m_font->init();
}

TextArea::~TextArea(void) {
    delete m_document;
    delete m_font;
}

yguipp::text::Document* TextArea::getDocument(void) {
    return m_document;
}

Point TextArea::getPreferredSize(void) {
    yguipp::text::Element* rootElement = m_document->getRootElement();

    return Point(
        400, /* todo */
        rootElement->getElementCount() * m_font->getHeight()
    );
}

void TextArea::setFont(yguipp::Font* font) {
    if (font == NULL) {
        return;
    }

    m_font->decRef();
    m_font = font;
    m_font->incRef();

    invalidate();
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
        std::string line = m_document->getText(e->getOffset(), e->getLength() - 1);

        g->drawText(p, line);

        p.m_y += m_font->getHeight();
    }

    return 0;
}

int TextArea::keyPressed(int key) {
    switch (key) {
        case KEY_LEFT :
            if (m_caretPosition > 0) {
                m_caretPosition--;
            }

            break;

        case KEY_RIGHT :
            if (m_caretPosition < m_document->getLength()) {
                m_caretPosition++;
            }

            break;

        default : {
            std::string s;
            s.append(reinterpret_cast<char*>(&key), 1);

            m_document->insert(m_caretPosition++, s);
        }
    }

    return 0;
}

int TextArea::onTextInserted(yguipp::text::Document* document) {
    invalidate();
    return 0;
}

int TextArea::onTextRemoved(yguipp::text::Document* document) {
    return 0;
}

} /* namespace yguipp */
