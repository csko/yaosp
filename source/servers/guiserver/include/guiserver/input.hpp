/* GUI server
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

#ifndef _INPUT_HPP_
#define _INPUT_HPP_

#include <yutil++/thread.hpp>

class WindowManager;

class InputThread : public yutilpp::Thread {
  public:
    InputThread( WindowManager* windowManager );
    virtual ~InputThread( void ) {}

    bool init( void );

    int run( void );

  private:
    int m_device;
    WindowManager* m_windowManager;
}; /* class InputThread */

#endif /* _INPUT_HPP_ */
