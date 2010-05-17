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

#include <yutil++/storage/directory.hpp>

namespace yutilpp {
namespace storage {

Directory::Directory( const std::string& path ) : m_dir(NULL), m_path(path) {
}

Directory::~Directory( void ) {
    if ( m_dir != NULL ) {
        closedir(m_dir);
    }
}

bool Directory::init( void ) {
    m_dir = opendir( m_path.c_str() );
    return ( m_dir != NULL );
}

bool Directory::nextEntry( std::string& entry ) {
    struct dirent* dirEntry;

    if ( m_dir == NULL ) {
        return false;
    }

    dirEntry = readdir(m_dir);

    if ( dirEntry == NULL ) {
        return false;
    }

    entry.clear();
    entry = dirEntry->d_name;

    return true;
}

} /* namespace storage */
} /* namespace yutilpp */
