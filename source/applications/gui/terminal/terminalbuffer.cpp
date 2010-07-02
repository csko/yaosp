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

TerminalBuffer::TerminalBuffer(int width, int height) : m_width(width), m_height(height) {
    m_lines = new TerminalLine[height];
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
        return &m_lines[index - historySize];
    }
}
