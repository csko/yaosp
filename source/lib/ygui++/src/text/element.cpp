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

#include <ygui++/text/element.hpp>

namespace yguipp {
namespace text {

Element::Element(int offset, int length)
    : m_offset(offset), m_length(length) {
}

Element::~Element(void) {
    for (std::vector<Element*>::const_iterator it = m_children.begin();
         it != m_children.end();
         ++it) {
        delete (*it);
    }
}

void Element::addChild(Element* element) {
    m_children.push_back(element);
}

void Element::insertChild(size_t index, Element* element) {
    m_children.insert(m_children.begin() + index, element);
}

void Element::incOffset(int amount) {
    m_offset += amount;
}

void Element::incLength(int amount) {
    m_length += amount;
}

void Element::decLength(int amount) {
    m_length -= amount;
}

int Element::getOffset(void) {
    return m_offset;
}

int Element::getLength(void) {
    return m_length;
}

void Element::setLength(int length) {
    m_length = length;
}

Element* Element::getElement(size_t index) {
    return m_children[index];
}

size_t Element::getElementCount(void) {
    return m_children.size();
}

} /* namespace text */
} /* namespace yguipp */
