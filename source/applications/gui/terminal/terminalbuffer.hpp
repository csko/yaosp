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

#include <string>
#include <vector>

#include <yutil++/thread/mutex.hpp>

enum TerminalColor {
    BLACK = 0,
    RED,
    GREEN,
    YELLOW,
    BLUE,
    MAGENTA,
    CYAN,
    WHITE,
    COLOR_COUNT
};

struct TerminalAttribute {
    TerminalAttribute(TerminalColor bgColor = BLACK, TerminalColor fgColor = WHITE, bool bold = false);

    bool operator==(const TerminalAttribute& attr);

    TerminalColor m_bgColor;
    TerminalColor m_fgColor;
    bool m_bold;
};

class TerminalLine {
  public:
    TerminalLine(void);

    void clear(char c, TerminalAttribute attr, int start = 0, int end = -1);
    bool setWidth(int width);

    std::string m_text;
    std::vector<TerminalAttribute> m_attr;
    int m_dirtyWidth;
}; /* class TerminalLine */

class TerminalBuffer {
  public:
    TerminalBuffer(int width, int height);
    ~TerminalBuffer(void);

    int getLineCount(void);
    void getCursorPosition(int& x, int& y);
    inline int getWidth(void) { return m_width; }
    inline int getHeight(void) { return m_height; }

    TerminalLine* lineAt(int index);

    bool setSize(int width, int height);
    void setBgColor(TerminalColor color);
    void setFgColor(TerminalColor color);
    void setBold(bool bold);
    void setScrollRegion(int top, int bottom);

    void insertCr(void);
    void insertLf(void);
    void insertBackSpace(void);
    void insertCharacter(uint8_t c);
    void moveCursorBy(int dx, int dy);
    void moveCursorTo(int x, int y);
    void swapFgBgColor(void);

    void saveCursor(void);
    void restoreCursor(void);
    void saveAttribute(void);
    void restoreAttribute(void);

    void scrollBy(int count);

    void erase(void);
    void eraseLine(void);

    void eraseAbove(void);
    void eraseBelow(void);
    void eraseBefore(void);
    void eraseAfter(void);

    inline void lock(void) { m_mutex.lock(); }
    inline void unLock(void) { m_mutex.unLock(); }

  private:
    int m_width;
    int m_height;
    int m_scrollTop;
    int m_scrollBottom;

    int m_cursorX;
    int m_cursorY;
    int m_savedCursorX;
    int m_savedCursorY;

    TerminalAttribute m_attrib;
    TerminalAttribute m_savedAttrib;

    TerminalLine** m_lines;

    yutilpp::thread::Mutex m_mutex;
}; /* class TerminalBuffer */

#endif /* _TERMINAL_TERMINALBUFFER_HPP_ */
