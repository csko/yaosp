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

bool TerminalLine::setWidth(int width) {
    int oldWidth = (int)m_text.size();

    m_text.resize(width);
    m_attr.resize(width);

    for (int i = oldWidth; i < width; i++) {
        m_text[i] = ' ';

        TerminalAttribute& attr = m_attr[i];
        attr.m_bgColor = 0;
        attr.m_fgColor = 7;
    }

    return true;
}

TerminalBuffer::TerminalBuffer(int width, int height) : m_width(0), m_height(0), m_cursorX(0), m_cursorY(0) {
    setSize(width, height);
}

TerminalBuffer::~TerminalBuffer(void) {
    delete[] m_lines;
}

int TerminalBuffer::getLineCount(void) {
    return m_height /* + history size */;
}

TerminalLine* TerminalBuffer::lineAt(int index) {
    assert((index >= 0) && (index < getLineCount()));

    int historySize = 0;

    if (index < historySize) {
        // todo
        return NULL;
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
        newLines[i] = new TerminalLine;
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

void TerminalBuffer::insertCr(void) {
}

void TerminalBuffer::insertLf(void) {
}

void TerminalBuffer::insertBackSpace(void) {
}

void TerminalBuffer::insertCharacter(uint8_t c) {
    assert((m_cursorX >= 0) && (m_cursorX < m_width));
    assert((m_cursorY >= 0) && (m_cursorY < m_height));

    //TerminalLine* line = m_lines[m_cursorY];
}
