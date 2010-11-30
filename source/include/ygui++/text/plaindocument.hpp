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

#ifndef _YGUIPP_TEXT_PLAINDOCUMENT_HPP_
#define _YGUIPP_TEXT_PLAINDOCUMENT_HPP_

#include <ygui++/text/document.hpp>

#include <yutil++/buffer/gapbuffer.hpp>

namespace yguipp {
namespace text {

class PlainDocument : public Document {
  public:
    PlainDocument(void);
    ~PlainDocument(void);

    int getLength(void);
    std::string getText(int offset, int length);

    Element* getRootElement(void);

    bool insert(int offset, const std::string& text);
    bool remove(int offset, int length);

  private:
    void charInsertedAt(int position, char c);

    class PlainElement : public Element {
      public:
        friend class PlainDocument;

        PlainElement(int startOffset, int endOffset, bool lineEnd);

      private:
        bool m_lineEnd;
    }; /* class PlainElement */

    Element* m_rootElement;
    yutilpp::buffer::GapBuffer* m_buffer;
}; /* class PlainDocument */

} /* namespace text */
} /* namespace yguipp */

#endif /* _YGUIPP_TEXT_PLAINDOCUMENT_HPP_ */
