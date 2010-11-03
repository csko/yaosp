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

#ifndef _TERMINAL_PTYHANDLER_HPP_
#define _TERMINAL_PTYHANDLER_HPP_

#include <ygui++/widget.hpp>
#include <yutil++/thread.hpp>

#include "terminalparser.hpp"

class PtyHandler : public yutilpp::Thread, public yguipp::event::KeyListener {
  public:
    PtyHandler(int masterPty, yguipp::Widget* view, TerminalParser* parser);

    int onKeyPressed(yguipp::Widget* widget, int key);
    int onKeyReleased(yguipp::Widget* widget, int key) { return 0; }

    int run(void);

  private:
    int m_masterPty;
    TerminalParser* m_parser;
    yguipp::Widget* m_view;
}; /* class PtyReader */

#endif /* _TERMINAL_PTYREADER_HPP_ */
