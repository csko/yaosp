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

#include <assert.h>

#include "terminalbuffer.hpp"

TerminalAttribute::TerminalAttribute(TerminalColor bgColor, TerminalColor fgColor, bool bold) : m_bgColor(bgColor),
                                                                                                m_fgColor(fgColor),
                                                                                                m_bold(bold) {
}

bool TerminalAttribute::operator==(const TerminalAttribute& attr) {
    return ((m_bgColor == attr.m_bgColor) &&
            (m_fgColor == attr.m_fgColor) &&
            (m_bold == attr.m_bold));
}

TerminalLine::TerminalLine(void) : m_dirtyWidth(0) {
}

TerminalLine::TerminalLine(TerminalLine* line) {
    m_text = line->m_text;
    m_attr = line->m_attr;
    m_dirtyWidth = line->m_dirtyWidth;
}

void TerminalLine::clear(char c, TerminalAttribute attr, int start, int end) {
    if (end == -1) {
        end = (int)m_text.size() - 1;
    }

    for (int i = start; i <= end; i++) {
        m_text[i] = ' ';
        m_attr[i] = attr;
    }
}

bool TerminalLine::setWidth(int width) {
    int oldWidth = (int)m_text.size();

    m_text.resize(width);
    m_attr.resize(width);

    if (width > oldWidth) {
        clear(' ', TerminalAttribute(), oldWidth, width - 1);
    }

    m_dirtyWidth = std::min(m_dirtyWidth, width);

    return true;
}

TerminalBuffer::TerminalBuffer(int width, int height) : m_width(0), m_height(0), m_cursorX(0), m_cursorY(0),
                                                        m_mutex("term_buffer") {
    setSize(width, height);

    m_scrollTop = 0;
    m_scrollBottom = height - 1;
}

TerminalBuffer::~TerminalBuffer(void) {
    for (int i = 0; i < m_height; i++) {
        delete m_lines[i];
    }

    delete[] m_lines;
}

void TerminalBuffer::addListener(TerminalBufferListener* listener) {
    m_listeners.push_back(listener);
}

int TerminalBuffer::getLineCount(void) {
    return m_history.size() + m_height;
}

int TerminalBuffer::getHistorySize(void) {
    return m_history.size();
}

void TerminalBuffer::getCursorPosition(int& x, int& y) {
    x = m_cursorX;
    y = m_cursorY;
}

TerminalLine* TerminalBuffer::lineAt(int index) {
    assert((index >= 0) && (index < getLineCount()));

    int historySize = (int)m_history.size();

    if (index < historySize) {
        return m_history[index];
    } else {
        return m_lines[index - historySize];
    }
}

bool TerminalBuffer::setSize(int width, int height) {
    int linesToCopy = std::min(height, m_height);
    TerminalLine** newLines = new TerminalLine*[height];

    /* Copy possible lines from the old buffer. */
    for (int i = 0; i < linesToCopy; i++) {
        newLines[i] = m_lines[i];
    }

    /* Create new lines in the new buffer if it's bigger than the old one. */
    for (int i = linesToCopy; i < height; i++) {
        newLines[i] = new TerminalLine();
    }

    /* Delete lines from the old buffer if it's bigger than the new one. */
    for (int i = linesToCopy; i < m_height; i++) {
        delete m_lines[i];
    }

    m_width = width;
    m_height = height;

    delete[] m_lines;
    m_lines = newLines;

    for (int i = 0; i < m_height; i++) {
        m_lines[i]->setWidth(width);
    }

    return true;
}

void TerminalBuffer::setBgColor(TerminalColor color) {
    m_attrib.m_bgColor = color;
}

void TerminalBuffer::setFgColor(TerminalColor color) {
    m_attrib.m_fgColor = color;
}

void TerminalBuffer::setBold(bool bold) {
    m_attrib.m_bold = bold;
}

void TerminalBuffer::setScrollRegion(int top, int bottom) {
    assert((bottom >= 0) && (bottom < m_height));
    assert((top >= 0) && (top <= bottom));

    m_scrollTop = top;
    m_scrollBottom = bottom;
}

void TerminalBuffer::insertCr(void) {
    m_cursorX = 0;
}

void TerminalBuffer::insertLf(void) {
    if (m_cursorY == m_scrollBottom) {
        scrollBy(1);
    } else {
        m_cursorY++;
    }
}

void TerminalBuffer::insertBackSpace(void) {
    if (m_cursorX == 0) {
        if (m_cursorY > 0) {
            m_cursorY--;
            m_cursorX = m_width - 1;
        }
    } else {
        m_cursorX--;
    }
}

void TerminalBuffer::insertCharacter(uint8_t c) {
    assert((m_cursorX >= 0) && (m_cursorX < m_width));
    assert((m_cursorY >= 0) && (m_cursorY < m_height));

    TerminalLine* line = m_lines[m_cursorY];
    line->m_text[m_cursorX] = c;
    line->m_attr[m_cursorX] = m_attrib;
    line->m_dirtyWidth = std::max(line->m_dirtyWidth, m_cursorX + 1);

    if (++m_cursorX == m_width) {
        if (m_cursorY == m_scrollBottom) {
            scrollBy(1);
        } else {
            m_cursorY++;
        }

        m_cursorX = 0;
    }
}

void TerminalBuffer::moveCursorBy(int dx, int dy) {
    m_cursorX += dx;
    m_cursorY += dy;

    assert((m_cursorX >= 0) && (m_cursorX < m_width));
    assert((m_cursorY >= 0) && (m_cursorY < m_height));
}

void TerminalBuffer::moveCursorTo(int x, int y) {
    if (x != -1) {
        m_cursorX = x;
        assert((m_cursorX >= 0) && (m_cursorX < m_width));
    }

    if (y != -1) {
        m_cursorY = y;
        assert((m_cursorY >= 0) && (m_cursorY < m_height));
    }
}

void TerminalBuffer::swapFgBgColor(void) {
    TerminalColor tmp;

    tmp = m_attrib.m_bgColor;
    m_attrib.m_bgColor = m_attrib.m_fgColor;
    m_attrib.m_fgColor = tmp;
}

void TerminalBuffer::saveCursor(void) {
    m_savedCursorX = m_cursorX;
    m_savedCursorY = m_cursorY;
}

void TerminalBuffer::restoreCursor(void) {
    m_cursorX = m_savedCursorX;
    m_cursorY = m_savedCursorY;
}

void TerminalBuffer::saveAttribute(void) {
    m_savedAttrib = m_attrib;
}

void TerminalBuffer::restoreAttribute(void) {
    m_attrib = m_savedAttrib;
}

void TerminalBuffer::scrollBy(int count) {
    assert(m_scrollTop >= 0);
    assert(m_scrollBottom < m_height);
    assert(m_scrollTop <= m_scrollBottom);
    assert(count != 0);

    /* Save scrolled lines to the history. */
    if ((m_scrollTop == 0) &&
        (count > 0)) {
        int linesToSave = std::min(count, m_height);

        for (int i = 0; i < linesToSave; i++) {
            m_history.push_back(new TerminalLine(m_lines[i]));
        }

        fireHistoryChangedListeners();
    }

    /* Scroll the requested region. */
    int scrollHeight = m_scrollBottom - m_scrollTop + 1;

    if (abs(count) >= scrollHeight) {
        for (int i = m_scrollTop; i <= m_scrollBottom; i++) {
            TerminalLine* line = m_lines[i];

            line->clear(' ', TerminalAttribute());
            line->m_dirtyWidth = 0;
        }
    } else if (count > 0) {
        std::vector<TerminalLine*> tmp;

        for (int i = 0; i < count; i++) {
            tmp.push_back(m_lines[m_scrollTop + i]);
        }

        for (int i = 0; i < (scrollHeight - count); i++) {
            m_lines[m_scrollTop + i] = m_lines[m_scrollTop + i + count];
        }

        for (int i = 0; i < count; i++) {
            TerminalLine* line = tmp[i];
            m_lines[m_scrollTop + scrollHeight - count + i] = line;
            line->clear(' ', TerminalAttribute());
            line->m_dirtyWidth = 0;
        }
    } else {
        /* todo */
    }
}

void TerminalBuffer::erase(void) {
    for (int i = 0; i < m_height; i++) {
        m_lines[i]->clear(' ', m_attrib);
    }
}

void TerminalBuffer::eraseLine(void) {
    m_lines[m_cursorY]->clear(' ', m_attrib);
}

void TerminalBuffer::eraseAbove(void) {
    for (int i = 0; i <= m_cursorY; i++) {
        m_lines[i]->clear(' ', m_attrib);
    }
}

void TerminalBuffer::eraseBelow(void) {
    for (int i = m_cursorY; i < m_height; i++) {
        m_lines[i]->clear(' ', m_attrib);
    }
}

void TerminalBuffer::eraseBefore(void) {
    m_lines[m_cursorY]->clear(' ', m_attrib, 0, m_cursorX);
}

void TerminalBuffer::eraseAfter(void) {
    m_lines[m_cursorY]->clear(' ', m_attrib, m_cursorX);
}

void TerminalBuffer::fireHistoryChangedListeners(void) {
    for (std::vector<TerminalBufferListener*>::const_iterator it = m_listeners.begin();
         it != m_listeners.end();
         ++it) {
        (*it)->onHistoryChanged(this);
    }
}

