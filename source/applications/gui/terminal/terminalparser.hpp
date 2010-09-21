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

#ifndef _TERMINAL_TERMINALPARSER_HPP_
#define _TERMINAL_TERMINALPARSER_HPP_

#include "terminalbuffer.hpp"

class TerminalParser {
  public:
    TerminalParser(TerminalBuffer* buffer);

    bool handleData(uint8_t* data, int length);

    bool useAlternateCursorKeys(void);

  private:
    enum ParserState {
        NONE,
        ESCAPE,
        BRACKET,
        SQUARE_BRACKET,
        QUESTION
    };

    void handleNone(uint8_t data);
    void handleEscape(uint8_t data);
    void handleBracket(uint8_t data);
    void handleSquareBracket(uint8_t data);
    void handleQuestion(uint8_t data);

    void updateMode(void);

  private:
    ParserState m_state;
    TerminalBuffer* m_buffer;
    std::vector<int> m_params;
    bool m_firstNumber;
    bool m_useAlternateCursorKeys;
}; /* class TerminalParser */

#endif /* _TERMINAL_TERMINALPARSER_HPP_ */
