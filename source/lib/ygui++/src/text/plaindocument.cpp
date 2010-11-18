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
    m_rootElement = new Element(0, 0);
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
    m_buffer->insert(offset, text.data(), text.size());
    // todo: update Element tree
    return true;
}

bool PlainDocument::remove(int offset, int length) {
    m_buffer->remove(offset, length);
    // todo: update Element tree
    return true;
}

} /* namespace text */
} /* namespace yguipp */
