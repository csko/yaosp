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

#ifndef _TERMINAL_PTYREADER_HPP_
#define _TERMINAL_PTYREADER_HPP_

#include <yutil++/thread.hpp>

#include "terminalparser.hpp"

class PtyReader : public yutilpp::Thread {
  public:
    PtyReader(int masterPty, TerminalParser* parser);

    int run(void);

  private:
    int m_masterPty;
    TerminalParser* m_parser;
}; /* class PtyReader */

#endif /* _TERMINAL_PTYREADER_HPP_ */
