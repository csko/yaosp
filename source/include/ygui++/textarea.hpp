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

#ifndef _YGUIPP_TEXTAREA_HPP_
#define _YGUIPP_TEXTAREA_HPP_

#include <ygui++/widget.hpp>
#include <ygui++/text/document.hpp>

namespace yguipp {

class TextArea : public Widget, public yguipp::event::DocumentListener {
  public:
    TextArea(void);
    ~TextArea(void);

    yguipp::text::Document* getDocument(void);

    Point getPreferredSize(void);
    void setFont(yguipp::Font* font);

    int paint(GraphicsContext* g);

    int keyPressed(int key);

    int onTextInserted(yguipp::text::Document* document);
    int onTextRemoved(yguipp::text::Document* document);

  private:
    int paintCursor(GraphicsContext* g);

    void calcXYCaretPosition(int& x, int& y);
    yguipp::text::Element* currentLineElement(void);

  private:
    yguipp::Font* m_font;

    int m_caretPosition;
    yguipp::text::Document* m_document;
}; /* class TextArea */

} /* namespace yguipp */

#endif /* _YGUIPP_TEXTAREA_HPP_ */
