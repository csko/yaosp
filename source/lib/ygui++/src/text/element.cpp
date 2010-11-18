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

Element::Element(int startOffset, int endOffset)
    : m_startOffset(startOffset), m_endOffset(endOffset) {
}

int Element::getStartOffset(void) {
    return m_startOffset;
}

int Element::getEndOffset(void) {
    return m_endOffset;
}

Element* Element::getElement(size_t index) {
    return m_children[index];
}

size_t Element::getElementCount(void) {
    return m_children.size();
}

} /* namespace text */
} /* namespace yguipp */
