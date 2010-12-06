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
    for (std::string::size_type i = 0; i < text.size(); i++) {
        charInsertedAt(offset + i, text[i]);
    }

    fireTextInsertListeners(this);

    return true;
}

bool PlainDocument::remove(int offset, int length) {
    /* Update our internal element tree. */
    for (int i = 0; i < length; i++) {
        charRemovedAt(offset);
    }

    m_buffer->remove(offset, length);

    fireTextRemoveListeners(this);

    return true;
}

void PlainDocument::charInsertedAt(int position, char c) {
    assert((position >= 0) && (position <= getLength()));

    size_t index = 0;
    PlainElement* e = NULL;

    for (size_t i = 0; i < m_rootElement->getElementCount(); i++) {
        PlainElement* tmp = dynamic_cast<PlainElement*>(m_rootElement->getElement(i));
        int tmpOffset = tmp->getOffset();

        if ((position >= tmpOffset) &&
            (position <= (tmpOffset + tmp->getLength()))) {
            e = tmp;
            index = i;
            break;
        }
    }

    assert(e != NULL);

    size_t shiftFrom;
    bool atElementEnd = position == (e->getOffset() + e->getLength());

    if (atElementEnd) {
        switch (c) {
            case '\n' :
                if (e->m_lineEnd) {
                    m_rootElement->insertChild(index + 1, new PlainElement(e->getOffset() + e->getLength(), 1, true));
                    shiftFrom = index + 2;
                } else {
                    e->m_lineEnd = true;
                    e->incLength(1);
                    shiftFrom = index + 1;
                }
                break;

            default :
                if (e->m_lineEnd) {
                    if (index == m_rootElement->getElementCount() - 1) {
                        m_rootElement->addChild(new PlainElement(e->getOffset() + e->getLength(), 1, false));
                    } else {
                        m_rootElement->getElement(index + 1)->incLength(1);
                    }

                    shiftFrom = index + 2;
                } else {
                    e->incLength(1);
                    shiftFrom = index + 1;
                }
                break;
        }
    } else {
        switch (c) {
            case '\n' : {
                int offsetInElement = position - e->getOffset();
                m_rootElement->insertChild(index + 1, new PlainElement(e->getOffset() + offsetInElement + 1, e->getLength() - offsetInElement, e->m_lineEnd));

                e->m_lineEnd = true;
                e->setLength(offsetInElement + 1);

                shiftFrom = index + 2;

                break;
            }

            default :
                e->incLength(1);
                shiftFrom = index + 1;
                break;
        }
    }

    for (size_t i = shiftFrom; i < m_rootElement->getElementCount(); i++) {
        m_rootElement->getElement(i)->incOffset(1);
    }
}

void PlainDocument::charRemovedAt(int position) {
    assert((position >= 0) && (position < getLength()));

    size_t index = 0;
    PlainElement* e = NULL;

    for (size_t i = 0; i < m_rootElement->getElementCount(); i++) {
        PlainElement* tmp = dynamic_cast<PlainElement*>(m_rootElement->getElement(i));
        int tmpOffset = tmp->getOffset();

        if ((position >= tmpOffset) &&
            (position < (tmpOffset + tmp->getLength()))) {
            e = tmp;
            index = i;
            break;
        }
    }

    assert(e != NULL);

    size_t shiftFrom = index + 1;
    bool atElementEnd = position == (e->getOffset() + e->getLength() - 1);

    if (atElementEnd) {
        if (e->m_lineEnd) {
            if (index == m_rootElement->getElementCount() - 1) {
                e->m_lineEnd = false;
            } else {
                PlainElement* tmp = dynamic_cast<PlainElement*>(m_rootElement->getElement(index + 1));

                e->m_lineEnd = tmp->m_lineEnd;
                e->incLength(tmp->getLength());
                e->decLength(1);

                m_rootElement->removeChild(index + 1);
                delete tmp;
            }
        } else {
            e->decLength(1);
        }
    } else {
        e->decLength(1);
    }

    for (size_t i = shiftFrom; i < m_rootElement->getElementCount(); i++) {
        m_rootElement->getElement(i)->decOffset(1);
    }
}

PlainDocument::PlainElement::PlainElement(int startOffset, int endOffset, bool lineEnd)
    : Element(startOffset, endOffset), m_lineEnd(lineEnd) {
}

} /* namespace text */
} /* namespace yguipp */
