/* Terminal application
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

#ifndef _TERMINAL_TERMINALVIEW_HPP_
#define _TERMINAL_TERMINALVIEW_HPP_

#include <ygui++/widget.hpp>

#include "terminalbuffer.hpp"

class Terminal;

class TerminalView : public yguipp::Widget, public TerminalBufferListener {
  public:
    TerminalView(Terminal* terminal, TerminalBuffer* buffer);

    yguipp::Font* getFont(void);
    yguipp::Point getPreferredSize(void);

    int paint(yguipp::GraphicsContext* g);

    int onHistoryChanged(TerminalBuffer* buffer);

  private:
    int paintLine(yguipp::GraphicsContext* g, int lineIndex, yguipp::Point position);
    int paintLinePart(yguipp::GraphicsContext* g, TerminalLine* line, yguipp::Point& position,
                      int start, int end, const TerminalAttribute& attr);
    int paintCursor(yguipp::GraphicsContext* g, int cursorX, int cursorY, TerminalLine* line);

  private:
    Terminal* m_terminal;

    yguipp::Font* m_font;
    TerminalBuffer* m_buffer;

    static yguipp::Color m_boldColors[COLOR_COUNT];
    static yguipp::Color m_normalColors[COLOR_COUNT];
}; /* class TerminalView */

#endif /* _TERMINAL_TERMINALVIEW_HPP_ */
