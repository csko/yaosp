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

#ifndef _TERMINAL_TERMINAL_H_
#define _TERMINAL_TERMINAL_H_

#include <ygui++/scrollpanel.hpp>

#include "ptyhandler.hpp"
#include "terminalparser.hpp"

class Terminal {
  public:
    Terminal(void);

    int run(void);

    int scrollToBottom(void);

  private:
    bool startShell(void);

  private:
    int m_masterPty;
    int m_slaveTty;

    PtyHandler* m_ptyHandler;
    TerminalBuffer* m_buffer;
    TerminalParser* m_parser;

    yguipp::ScrollPanel* m_scrollPanel;
}; /* class Terminal */

#endif /* _TERMINAL_TERMINAL_H_ */
