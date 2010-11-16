/* yutil++ gap buffer implementation
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

#include <algorithm>

#include <yutil++/buffer/gapbuffer.hpp>

namespace yutilpp {

GapBuffer::GapBuffer(int initialSize) {
    m_gapStart = 0;
    m_gapEnd = initialSize - 1;
    m_gapSize = initialSize;
    m_buffer = new char[initialSize];
}

GapBuffer::~GapBuffer(void) {
    delete[] m_buffer;
}

bool GapBuffer::insert(int position, const char* data, int size) {
    return true;
}

bool GapBuffer::moveGapTo(int position) {
    /* Check if the start of the gap is already located at the requested position. */
    if (m_gapStart == position) {
        return true;
    }

    /* Check if we have a zero length gap. It can be moved to anywhere without touching the buffer. */
    if (m_gapSize == 0) {
        m_gapStart = position;
        m_gapEnd = position;
        return true;
    }

    if (position < m_gapStart) {
    } else {
    }

    /* Update the start and end position of the gap. */
    m_gapStart = position;
    m_gapEnd = m_gapStart + m_gapSize - 1;

    return true;
}

} /* namespace yutilpp */
