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

#ifndef _YUTILPP_BUFFER_GAPBUFFER_HPP_
#define _YUTILPP_BUFFER_GAPBUFFER_HPP_

#include <inttypes.h>
#include <string>

namespace yutilpp {
namespace buffer {

class GapBuffer {
  public:
    GapBuffer(int initialSize = 10);
    ~GapBuffer(void);

    int getSize(void);

    bool insert(int position, const char* data, int size);
    bool remove(int position, int size);

    std::string asString(int start = 0, int size = -1);

  private:
    void moveGapTo(int position);
    bool ensureGapSize(int size);

  private:
    int m_gapStart;
    int m_gapEnd;
    int m_gapSize;
    char* m_buffer;
    int m_bufferSize;
}; /* class GapBuffer */

} /* namespace buffer */
} /* namespace yutilpp */

#endif /* _YUTILPP_BUFFER_GAPBUFFER_HPP_ */
