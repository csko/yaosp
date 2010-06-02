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

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <yaosp/input.h>
#include <yaosp/debug.h>

#include <guiserver/input.hpp>

InputThread::InputThread( WindowManager* windowManager ) : Thread("input"), m_device(-1),
                                                           m_windowManager(windowManager) {
}

bool InputThread::init( void ) {
    int ctrl;
    int error;
    char path[128];
    input_cmd_create_node_t cmd;

    ctrl = open("/device/control/input", O_RDONLY);

    if (ctrl < 0) {
        return false;
    }

    cmd.flags = INPUT_KEY_EVENTS | INPUT_MOUSE_EVENTS;

    error = ioctl(ctrl, IOCTL_INPUT_CREATE_DEVICE, &cmd);
    close(ctrl);

    if (error != 0) {
        return false;
    }

    snprintf(path, sizeof(path), "/device/input/node/%u", cmd.node_number);
    m_device = open(path, O_RDONLY);

    return ( m_device >= 0 );
}

int InputThread::run( void ) {
    while (1) {
        input_event_t event;

        if ( read(m_device, &event, sizeof(event)) != 0 ) {
            dbprintf("InputThread::run(): failed to get event.\n");
            break;
        }

        switch (event.event) {
            case E_KEY_PRESSED :
                break;

            case E_KEY_RELEASED :
                break;

            case E_QUALIFIERS_CHANGED :
                break;

            case E_MOUSE_MOVED :
                m_windowManager->mouseMoved(
                    yguipp::Point(event.param1, event.param2)
                );
                break;

            case E_MOUSE_PRESSED :
                m_windowManager->mousePressed(event.param1);
                break;

            case E_MOUSE_RELEASED :
                m_windowManager->mousePressed(event.param1);
                break;

            case E_MOUSE_SCROLLED :
                break;
        }
    }

    return 0;
}
