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

#include <string.h>

#include <algorithm>

#include <yutil++/buffer/gapbuffer.hpp>

namespace yutilpp {
namespace buffer {

GapBuffer::GapBuffer(int initialSize) {
    m_gapStart = 0;
    m_gapEnd = initialSize;
    m_gapSize = initialSize;

    m_buffer = new char[initialSize];
    m_bufferSize = initialSize;
}

GapBuffer::~GapBuffer(void) {
    delete[] m_buffer;
}

int GapBuffer::getSize(void) {
    return (m_bufferSize - m_gapSize);
}

bool GapBuffer::insert(int position, const char* data, int size) {
    moveGapTo(position);

    if (!ensureGapSize(size)) {
        return false;
    }

    memcpy(&m_buffer[m_gapStart], data, size);

    m_gapStart += size;
    m_gapSize -= size;

    return true;
}

bool GapBuffer::remove(int position, int size) {
    moveGapTo(position + size);
    m_gapStart -= size;

    return true;
}

std::string GapBuffer::asString(int start, int size) {
    std::string s;

    /* If the size is -1, it means that we have to copy the entire buffer. */
    if (size == -1) {
        size = m_bufferSize - m_gapSize;
    }

    if ((start >= m_gapStart) &&
        (start < m_gapEnd)) {
        start = m_gapEnd + (start - m_gapStart);
    } else if (m_gapStart >= start) {
        int toCopy = std::min(size, m_gapStart - start);

        if (toCopy > 0) {
            s.append(&m_buffer[start], toCopy);

            size -= toCopy;
            start += toCopy;
        }

        if (start == m_gapStart) {
            start = m_gapEnd;
        }
    }

    size = std::min(size, m_bufferSize - start);
    s.append(&m_buffer[start], size);

    return s;
}

void GapBuffer::moveGapTo(int position) {
    /* Check if the start of the gap is already located at the requested position. */
    if (m_gapStart == position) {
        return;
    }

    /* Check if we have a zero length gap. It can be moved to anywhere without touching the buffer. */
    if (m_gapSize == 0) {
        m_gapStart = position;
        m_gapEnd = position;
        return;
    }

    if (position < m_gapStart) {
        int delta = m_gapStart - position;
        memmove(&m_buffer[m_gapEnd - delta], &m_buffer[position], delta);
    } else {
        int delta = position - m_gapStart;
        memmove(&m_buffer[m_gapStart], &m_buffer[m_gapEnd], delta);
    }

    /* Update the start and end position of the gap. */
    m_gapStart = position;
    m_gapEnd = m_gapStart + m_gapSize;
}

bool GapBuffer::ensureGapSize(int size) {
    /* Check if we already have enough gap size. */
    if (m_gapSize >= size) {
        return true;
    }

    int newGapSize = (m_gapSize + size) * 2;
    int newBufferSize = m_bufferSize - m_gapSize + newGapSize;
    char* newBuffer = new char[m_bufferSize - m_gapSize + newGapSize];

    /* Copy the data located before the gap. */
    if (m_gapStart > 0) {
        memcpy(newBuffer, m_buffer, m_gapStart);
    }

    /* Copy the data located after the gap. */
    int dataAfterGap = m_bufferSize - m_gapEnd;

    if (dataAfterGap > 0) {
        memcpy(&newBuffer[m_gapStart + newGapSize], &m_buffer[m_gapEnd], dataAfterGap);
    }

    /* Update the internals of GapBuffer. */
    delete[] m_buffer;
    m_buffer = newBuffer;
    m_bufferSize = newBufferSize;

    m_gapEnd = m_gapStart + newGapSize;
    m_gapSize = newGapSize;

    return true;
}

} /* namespace buffer */
} /* namespace yutilpp */
