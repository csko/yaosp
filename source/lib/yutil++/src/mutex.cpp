/* yaosp IPC port implementation
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

#include <yaosp/yaosp.h>
#include <yaosp/syscall.h>
#include <yaosp/syscall_table.h>

#include <yutil++/mutex.hpp>

namespace yutilpp {

ScopedMutex::ScopedMutex(Mutex* mutex) : m_mutex(mutex) {
    m_mutex->lock();
}

ScopedMutex::~ScopedMutex(void) {
    m_mutex->unLock();
}

Mutex::Mutex( const std::string& name, bool recursive ) {
    m_id = syscall2(
        SYS_mutex_create, reinterpret_cast<uint32_t>( name.c_str() ), recursive ? MUTEX_RECURSIVE : MUTEX_NONE
    );
}

Mutex::~Mutex( void ) {
    syscall1( SYS_mutex_destroy, m_id );
}

int Mutex::lock( void ) {
    return syscall1( SYS_mutex_lock, m_id );
}

int Mutex::unLock( void ) {
    return syscall1( SYS_mutex_unlock, m_id );
}

} /* namespace yutilpp */
