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

#ifndef _STORAGE_FILE_HPP_
#define _STORAGE_FILE_HPP_

#include <string>
#include <fcntl.h>

namespace yutilpp {
namespace storage {

class File {
  public:
    File( const std::string& path, int mode = O_RDONLY );
    ~File( void );

    bool init( void );

    int read( void* buffer, int size );
    bool seek(off_t offset);

  private:
    int m_file;
    int m_mode;
    std::string m_path;
}; /* class File */

} /* namespace storage */
} /* namespace yutilpp */

#endif /* _STORAGE_FILE_HPP_ */
