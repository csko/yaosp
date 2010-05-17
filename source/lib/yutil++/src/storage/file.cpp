/* yutil++ directory implementation
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

#include <unistd.h>

#include <yutil++/storage/file.hpp>

namespace yutilpp {
namespace storage {

File::File( const std::string& path, int mode ) : m_file(-1), m_mode(mode), m_path(path) {
}

File::~File( void ) {
    if ( m_file >= 0 ) {
        ::close(m_file);
    }
}

bool File::init( void ) {
    m_file = ::open( m_path.c_str(), m_mode );
    return ( m_file >= 0 );
}

int File::read( void* buffer, int size ) {
    return ::read( m_file, buffer, size );
}

} /* namespace storage */
} /* namespace yutilpp */
