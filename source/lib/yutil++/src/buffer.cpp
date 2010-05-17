/* yutil++ buffer implementation
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

#include <yutil++/buffer.hpp>

namespace yutilpp {

Buffer::Buffer( void ) : m_dataSize(0), m_bufferSize(0), m_buffer(NULL) {
}

Buffer::~Buffer( void ) {
    delete[] m_buffer;
}

int Buffer::read( void* buffer, size_t size ) {
    size_t toRead = std::min(m_dataSize,size);

    if ( toRead == 0 ) {
        return 0;
    }

    memcpy(buffer,m_buffer,toRead);

    if ( m_dataSize > toRead ) {
        memmove(m_buffer, m_buffer + toRead, m_dataSize - toRead );
    }

    m_dataSize -= toRead;

    return toRead;
}

int Buffer::write( void* buffer, size_t size ) {
    if ( ( m_dataSize + size ) > m_bufferSize ) {
        uint8_t* buffer;

        buffer = new uint8_t[m_dataSize + size];
        memcpy(buffer,m_buffer,m_dataSize);
        delete[] m_buffer;
        m_buffer = buffer;
        m_bufferSize = m_dataSize + size;
    }

    memcpy( m_buffer + m_dataSize, buffer, size );
    m_dataSize += size;

    return size;
}

int Buffer::getDataSize( void ) {
    return m_dataSize;
}

} /* namespace yutilpp */
