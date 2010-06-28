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

#ifndef _TERMINAL_TERMINALBUFFER_HPP_
#define _TERMINAL_TERMINALBUFFER_HPP_

class TerminalBuffer {
  public:
    TerminalBuffer(int width, int height);

  private:
    int m_width;
    int m_height;

    int m_cursorX;
    int m_cursorY;
    int m_savedCursorX;
    int m_savedCursorY;
}; /* class TerminalBuffer */

#endif /* _TERMINAL_TERMINALBUFFER_HPP_ */
