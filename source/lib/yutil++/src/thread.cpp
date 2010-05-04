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

#include <time.h>
#include <yaosp/syscall.h>
#include <yaosp/syscall_table.h>

#include <yutil++/thread.hpp>

namespace yutilpp {

Thread::Thread( const std::string& name ) : m_id(-1), m_name(name) {
}

Thread::~Thread( void ) {
}

bool Thread::start( void ) {
    m_id = syscall5(
        SYS_create_thread, ( int )m_name.c_str(), PRIORITY_NORMAL,
        ( int )starter, ( int )this, 0
    );

    if ( m_id < 0 ) {
        return false;
    }

    syscall1( SYS_wake_up_thread, m_id );

    return true;
}

void Thread::starter( void* arg ) {
    Thread* t;

    t = reinterpret_cast<Thread*>(arg);
    t->run();
}

bool Thread::uSleep( uint64_t usecs ) {
    struct timespec req;

    req.tv_sec = usecs / 1000000;
    req.tv_nsec = ( usecs % 1000000 ) * 1000;

    return ( nanosleep(&req, NULL) == 0 );
}

} /* namespace yutilpp */
