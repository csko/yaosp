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

#ifndef _STORAGE_DIRECTORY_HPP_
#define _STORAGE_DIRECTORY_HPP_

#include <string>
#include <dirent.h>

namespace yutilpp {
namespace storage {

class Directory {
  public:
    Directory( const std::string& path );
    ~Directory( void );

    bool init( void );
    bool nextEntry( std::string& entry );

  private:
    DIR* m_dir;
    std::string m_path;
}; /* class Directory */

} /* namespace storage */
} /* namespace yutilpp */

#endif /* _STORAGE_DIRECTORY_HPP_ */
