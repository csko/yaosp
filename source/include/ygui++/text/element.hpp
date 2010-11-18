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

#ifndef _YGUIPP_TEXT_ELEMENT_HPP_
#define _YGUIPP_TEXT_ELEMENT_HPP_

#include <vector>

namespace yguipp {
namespace text {

class Element {
  public:
    Element(int startOffset, int endOffset);

    int getStartOffset(void);
    int getEndOffset(void);

    Element* getElement(size_t index);
    size_t getElementCount(void);

  private:
    int m_startOffset;
    int m_endOffset;

    std::vector<Element*> m_children;
}; /* class Element */

} /* namespace text */
} /* namespace yguipp */

#endif /* _YGUIPP_TEXT_ELEMENT_HPP_ */
