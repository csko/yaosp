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

#include <yaosp/debug.h>

#include "terminalparser.hpp"

TerminalParser::TerminalParser(TerminalBuffer* buffer) : m_state(NONE), m_buffer(buffer) {
}

bool TerminalParser::handleData(uint8_t* data, int length) {
    for (int i = 0; i < length; i++) {
        switch (m_state) {
            case NONE : handleNone(data[i]); break;
            case ESCAPE : handleEscape(data[i]); break;
            case BRACKET : handleBracket(data[i]); break;
            case SQUARE_BRACKET : handleSquareBracket(data[i]); break;
            case QUESTION : handleQuestion(data[i]); break;
        }
    }

    return true;
}

void TerminalParser::handleNone(uint8_t data) {
    switch (data) {
        case 27 :
            m_state = ESCAPE;
            break;

        case '\r' :
            m_buffer->insertCr();
            break;

        case '\n' :
            m_buffer->insertLf();
            break;

        case '\b' :
            m_buffer->insertBackSpace();
            break;

        case '\t' :
            break;

        default :
            if (data < 32) {
                break;
            }

            m_buffer->insertCharacter(data);

            break;
    }
}

void TerminalParser::handleEscape(uint8_t data) {
    switch (data) {
        case '(' :
            m_state = BRACKET;
            break;

        case '[' :
            m_state = SQUARE_BRACKET;
            break;

        default :
            dbprintf("TerminalParser::handleEscape(): invalid data: %d\n", (int)data);
            m_state = NONE;
            break;
    }
}

void TerminalParser::handleBracket(uint8_t data) {
    switch (data) {
        default :
            dbprintf("TerminalParser::handleBracket(): invalid data: %d\n", (int)data);
            m_state = NONE;
            break;
    }
}

void TerminalParser::handleSquareBracket(uint8_t data) {
    switch (data) {
        default :
            dbprintf("TerminalParser::handleSquareBracket(): invalid data: %d\n", (int)data);
            m_state = NONE;
            break;
    }
}

void TerminalParser::handleQuestion(uint8_t data) {
    switch (data) {
        default :
            dbprintf("TerminalParser::handleQuestion(): invalid data: %d\n", (int)data);
            m_state = NONE;
            break;
    }
}
