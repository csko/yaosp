/* yaosp thread implementation
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

#ifndef _THREAD_HPP_
#define _THREAD_HPP_

#include <string>

namespace yutilpp {

class Thread {
  public:
    typedef int Id;

    Thread( const std::string& name );
    virtual ~Thread( void );

    bool start( void );

    virtual int run( void ) = 0;

    static bool uSleep(uint64_t usecs);
    static Id currentThread(void);

  private:
    enum {
        PRIORITY_HIGH = 31,
        PRIORITY_DISPLAY = 23,
        PRIORITY_NORMAL = 15,
        PRIORITY_LOW = 7,
        PRIORITY_IDLE = 0
    };

    static void starter( void* arg );

  private:
    Id m_id;
    std::string m_name;
}; /* class Thread */

} /* namespace yutilpp */

#endif /* _THREAD_HPP_ */
