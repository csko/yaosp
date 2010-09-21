/* Terminal application
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

#include <unistd.h>
#include <errno.h>
#include <sys/select.h>
#include <yaosp/debug.h>
#include <yaosp/input.h>

#include "ptyhandler.hpp"

PtyHandler::PtyHandler(int masterPty, yguipp::Widget* view, TerminalParser* parser) : Thread("pty_reader"),
                                                                                      m_masterPty(masterPty),
                                                                                      m_parser(parser),
                                                                                      m_view(view) {
    m_view->addKeyListener(this);
}

int PtyHandler::keyPressed(yguipp::Widget* widget, int key) {
    switch (key) {
        case KEY_UP :
            if (m_parser->useAlternateCursorKeys()) {
                write(m_masterPty, "\x1bOA", 3);
            } else {
                write(m_masterPty, "\x1b[A", 3);
            }
            break;

        case KEY_DOWN :
            if (m_parser->useAlternateCursorKeys()) {
                write(m_masterPty, "\x1bOB", 3);
            } else {
                write(m_masterPty, "\x1b[B", 3);
            }
            break;

        case KEY_LEFT :
            if (m_parser->useAlternateCursorKeys()) {
                write(m_masterPty, "\x1bOD", 3);
            } else {
                write(m_masterPty, "\x1b[D", 3);
            }
            break;

        case KEY_RIGHT :
            if (m_parser->useAlternateCursorKeys()) {
                write(m_masterPty, "\x1bOC", 3);
            } else {
                write(m_masterPty, "\x1b[C", 3);
            }
            break;

        case KEY_HOME :
            write(m_masterPty, "\x1b[H", 3);
            break;

        case KEY_END :
            write(m_masterPty, "\x1b[F", 3);
            break;

        case KEY_DELETE :
            write(m_masterPty, "\x1b[3~", 4);
            break;

        case KEY_PAGE_UP :
            write(m_masterPty, "\x1b[5~", 4);
            break;

        case KEY_PAGE_DOWN :
            write(m_masterPty, "\x1b[6~", 4);
            break;

        default :
            if (key < 256) {
                write(m_masterPty, &key, 1);
            }
            break;
    }

    return 0;
}

int PtyHandler::run(void) {
    fd_set readSet;
    uint8_t buffer[512];
    struct timeval timeOut;

    while (1) {
        FD_ZERO(&readSet);
        FD_SET(m_masterPty, &readSet);

        timeOut.tv_sec = 0;
        timeOut.tv_usec = 250 * 1000;

        int ret = select(m_masterPty + 1, &readSet, NULL, NULL, &timeOut);

        if (ret < 0) {
            dbprintf("PtyReader::run(): select failed: %d.\n", errno);
            break;
        } else if (ret > 0) {
            int s = read(m_masterPty, buffer, sizeof(buffer));

            if (s < 0) {
                dbprintf("PtyReader::run(): failed to read from master pty!\n");
                break;
            } else if (s > 0) {
                m_parser->handleData(buffer, s);
                m_view->invalidate();
            }
        }
    }

    return 0;
}
