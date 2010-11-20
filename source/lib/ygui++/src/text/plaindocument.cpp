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

#include <assert.h>

#include <ygui++/text/plaindocument.hpp>

namespace yguipp {
namespace text {

PlainDocument::PlainDocument(void) {
    m_rootElement = new PlainElement(0, 0, false);
    //m_rootElement->addChild(new PlainElement(0, 0, false));
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
    assert(!tokens.empty());

    size_t index = 0;
    PlainElement* e;

    if (m_rootElement->getElementCount() == 0) {
        e = new PlainElement(0, 0, false);
        m_rootElement->addChild(e);
    } else {
        e = dynamic_cast<PlainElement*>(m_rootElement->getElement(index));

        while (e != NULL) {
            if ((e->getOffset() <= offset) &&
                (offset <= (e->getOffset() + e->getLength() - 1))) {
                break;
            }

            index++;
            e = dynamic_cast<PlainElement*>(m_rootElement->getElement(index));
        }
    }

    assert(e != NULL);

    int currOffset = e->getOffset();
    int offsetToAdd = 0;
    int lenAtEnd = e->getLength() - (offset - e->getOffset());
    bool remainingIsEnd = e->m_lineEnd;
    
    const std::pair<int, bool>& firstToken = tokens[0];
    const std::pair<int, bool>& lastToken = tokens[tokens.size() - 1];

    e->decLength(lenAtEnd);
    e->incLength(firstToken.first);
    e->m_lineEnd = firstToken.second;

    currOffset += e->getLength();
    offsetToAdd += firstToken.first;

    if (e->m_lineEnd) {
        index++;
    }

    size_t lastIndex = lastToken.second ? tokens.size() : tokens.size() - 1;

    for (size_t i = 1; i < lastIndex; i++) {
        const std::pair<int, bool>& token = tokens[i];
        assert(token.second);

        e = new PlainElement(currOffset, token.first, true);
        
        m_rootElement->insertChild(index++, e);
        currOffset += token.first;
        offsetToAdd += token.first;
    }

    if ((lenAtEnd > 0) ||
        (!lastToken.second)) {
        if (index == m_rootElement->getElementCount()) {
            e = new PlainElement(currOffset - offsetToAdd, 0, remainingIsEnd);
            m_rootElement->insertChild(index++, e);
        } else {
            e = dynamic_cast<PlainElement*>(m_rootElement->getElement(index++));
        }

        e->incLength(lenAtEnd);

        /* Increase length and offset only if this is not the first element we touched. */
        if ((!lastToken.second) &&
            (tokens.size() > 1)) {
            e->incLength(lastToken.first);
            e->incOffset(offsetToAdd);
        }
    }

    for (; index < m_rootElement->getElementCount(); index++) {
        e = dynamic_cast<PlainElement*>(m_rootElement->getElement(index));
        e->incOffset(offsetToAdd);
    }

    fireTextInsertListeners(this);

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
