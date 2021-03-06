/* yaOSp mutex implementation
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

#ifndef _MUTEX_HPP_
#define _MUTEX_HPP_

#include <string>

namespace yutilpp {
namespace thread {

class Mutex;

class ScopedMutex {
  public:
    ScopedMutex(Mutex* mutex);
    ~ScopedMutex(void);

  private:
    Mutex* m_mutex;
}; /* class ScopedMutex */

class Mutex {
  public:
    Mutex( const std::string& name, bool recursive = false );
    ~Mutex( void );

    int lock( void );
    int unLock( void );

  private:
    int m_id;
}; /* class Mutex */

} /* namespace thread */
} /* namespace yutilpp */

#endif /* _MUTEX_HPP_ */
