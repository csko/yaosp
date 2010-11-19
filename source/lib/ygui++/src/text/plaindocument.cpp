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

#include <ygui++/text/plaindocument.hpp>

namespace yguipp {
namespace text {

PlainDocument::PlainDocument(void) {
    m_rootElement = new PlainElement(0, 0, false);
    m_rootElement->addChild(new PlainElement(0, 0, false));
    m_buffer = new yutilpp::buffer::GapBuffer();
}

PlainDocument::~PlainDocument(void) {
    delete m_rootElement;
    delete m_buffer;
}

int PlainDocument::getLength(void) {
    return m_buffer->getSize();
}

std::string PlainDocument::getText(int offset, int length) {
    return m_buffer->asString(offset, length);
}

Element* PlainDocument::getRootElement(void) {
    return m_rootElement;
}

bool PlainDocument::insert(int offset, const std::string& text) {
    if (text.empty()) {
        return true;
    }

    /* Insert the next text into the buffer. */
    m_buffer->insert(offset, text.data(), text.size());

    /* Update our internal element tree. */
    std::vector< std::pair<int, bool> > tokens;
    tokenize(text, tokens, "\n");

    PlainElement* lastElement = dynamic_cast<PlainElement*>(m_rootElement->getElement(m_rootElement->getElementCount() - 1));

    /* Text is inserted to the end of the buffer? */
    if (offset == (lastElement->getOffset() + lastElement->getLength())) {
        size_t currToken = 0;

        if (!lastElement->m_lineEnd) {
            lastElement->incLength(tokens[0].first);
            lastElement->m_lineEnd = tokens[0].second;
            currToken++;
        }

        int offset = lastElement->getOffset() + lastElement->getLength();

        while (currToken < tokens.size()) {
            const std::pair<int, bool>& t = tokens[currToken];

            m_rootElement->addChild(new PlainElement(offset, t.first, t.second));
            offset += t.first;

            currToken++;
        }
    }

    // todo: handle other cases :)

    return true;
}

bool PlainDocument::remove(int offset, int length) {
    m_buffer->remove(offset, length);
    // todo: update Element tree
    return true;
}

void PlainDocument::tokenize(const std::string& s, std::vector< std::pair<int, bool> >& tokens, const std::string& delim) {
    std::string::size_type i;
    std::string::size_type j = 0;
    std::string::size_type l = delim.size();

    while ((i = s.find(delim, j)) != std::string::npos) {
        tokens.push_back(std::make_pair<int, bool>(i - j + l, true));
        j = i + l;
    }

    if (j < s.size()) {
        tokens.push_back(std::make_pair<int, bool>(s.size() - j, false));
    }
}

PlainDocument::PlainElement::PlainElement(int startOffset, int endOffset, bool lineEnd)
    : Element(startOffset, endOffset), m_lineEnd(lineEnd) {
}

} /* namespace text */
} /* namespace yguipp */
