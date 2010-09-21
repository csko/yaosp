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

#include <assert.h>
#include <yaosp/debug.h>

#include "terminalparser.hpp"

TerminalParser::TerminalParser(TerminalBuffer* buffer) : m_state(NONE), m_buffer(buffer), m_useAlternateCursorKeys(false) {
}

bool TerminalParser::handleData(uint8_t* data, int length) {
    m_buffer->lock();

    for (int i = 0; i < length; i++) {
        switch (m_state) {
            case NONE : handleNone(data[i]); break;
            case ESCAPE : handleEscape(data[i]); break;
            case BRACKET : handleBracket(data[i]); break;
            case SQUARE_BRACKET : handleSquareBracket(data[i]); break;
            case QUESTION : handleQuestion(data[i]); break;
        }
    }

    m_buffer->unLock();

    return true;
}

bool TerminalParser::useAlternateCursorKeys(void) {
    return m_useAlternateCursorKeys;
}

void TerminalParser::handleNone(uint8_t data) {
    switch (data) {
        case 27 :
            m_state = ESCAPE;
            m_params.clear();
            m_firstNumber = true;
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
            if (data >= 32) {
                m_buffer->insertCharacter(data);
            }

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

        case '>' :
        case '=' :
            /* todo: what the hell is this? */
            m_state = NONE;
            break;

        case '7' :
            m_buffer->saveCursor();
            m_buffer->saveAttribute();
            m_state = NONE;
            break;

        case '8' :
            m_buffer->restoreCursor();
            m_buffer->restoreAttribute();
            m_state = NONE;
            break;

        default :
            dbprintf("TerminalParser::handleEscape(): invalid data: %d\n", (int)data);
            m_state = NONE;
            break;
    }
}

void TerminalParser::handleBracket(uint8_t data) {
    switch (data) {
        case 'B' :
            /* North American ASCII set */
            m_state = NONE;
            break;

        default :
            dbprintf("TerminalParser::handleBracket(): invalid data: %d\n", (int)data);
            m_state = NONE;
            break;
    }
}

void TerminalParser::handleSquareBracket(uint8_t data) {
    switch (data) {
        case '0' ... '9' :
            if (m_firstNumber) {
                m_params.push_back(data - '0');
                m_firstNumber = false;
            } else {
                int& n = m_params[m_params.size() - 1];
                n *= 10;
                n += (data - '0');
            }

            break;

        case ';' :
            m_firstNumber = true;
            break;

        case 'A' :
            if (m_params.empty()) {
                m_buffer->moveCursorBy(0, -1);
            } else {
                m_buffer->moveCursorBy(0, -m_params[0]);
            }
            m_state = NONE;
            break;

        case 'J' :
            assert((m_params.empty()) ||
                   (m_params.size() == 1));

            if (m_params.empty()) {
                m_buffer->eraseBelow();
            } else {
                switch (m_params[0]) {
                    case 1 :
                        m_buffer->eraseAbove();
                        break;

                    case 2 :
                        m_buffer->erase();
                        m_buffer->moveCursorTo(0, 0);
                        break;
                }
            }
            m_state = NONE;
            break;

        case 'K' :
            assert((m_params.empty()) ||
                   (m_params.size() == 1));

            if (m_params.empty()) {
                m_buffer->eraseAfter();
            } else {
                switch (m_params[0]) {
                    case 1 :
                        m_buffer->eraseBefore();
                        break;

                    case 2 :
                        m_buffer->eraseLine();
                        break;
                }
            }
            m_state = NONE;
            break;

        case 'S' :
            assert(m_params.size() == 1);
            m_buffer->scrollBy(m_params[0]);
            m_state = NONE;
            break;

        case 'f' :
        case 'H' :
            assert((m_params.empty()) ||
                   (m_params.size() == 2));

            if (m_params.empty()) {
                m_buffer->moveCursorTo(0, 0);
            } else {
                m_buffer->moveCursorTo(m_params[1] - 1, m_params[0] - 1);
            }

            m_state = NONE;
            break;

        case 'd' :
            assert(m_params.size() == 1);
            m_buffer->moveCursorTo(-1, m_params[0] - 1);
            m_state = NONE;
            break;

        case 'G' :
            assert(m_params.size() == 1);
            m_buffer->moveCursorTo(m_params[0] - 1, -1);
            m_state = NONE;
            break;

        case 'r' :
            assert(m_params.size() == 2);
            m_buffer->setScrollRegion(m_params[0] - 1, m_params[1] - 1);
            m_state = NONE;
            break;

        case 'm' :
            updateMode();
            m_state = NONE;
            break;

        case 'l' :
            /* todo */
            m_state = NONE;
            break;

        case '?' :
            m_state = QUESTION;
            break;

        default :
            dbprintf("TerminalParser::handleSquareBracket(): invalid data: %d\n", (int)data);
            m_state = NONE;
            break;
    }
}

void TerminalParser::handleQuestion(uint8_t data) {
    switch (data) {
        case '0' ... '9' :
            if (m_firstNumber) {
                m_params.push_back(data - '0');
                m_firstNumber = false;
            } else {
                int& n = m_params[m_params.size() - 1];
                n *= 10;
                n += (data - '0');
            }

            break;


        case 'h' :
            assert(m_params.size() == 1);

            switch (m_params[0]) {
                case 1 :
                    m_useAlternateCursorKeys = true;
                    break;
            }

            m_state = NONE;
            break;

        case 'l' :
            assert(m_params.size() == 1);

            switch (m_params[0]) {
                case 1 :
                    m_useAlternateCursorKeys = false;
                    break;
            }

            m_state = NONE;
            break;

        default :
            dbprintf("TerminalParser::handleQuestion(): invalid data: %d\n", (int)data);
            m_state = NONE;
            break;
    }
}

void TerminalParser::updateMode(void) {
    if (m_params.empty()) {
        m_buffer->setBgColor(BLACK);
        m_buffer->setFgColor(WHITE);
        m_buffer->setBold(false);
        return;
    }

    for (std::vector<int>::const_iterator it = m_params.begin();
         it != m_params.end();
         ++it) {
        switch (*it) {
            case 0 :
                m_buffer->setBgColor(BLACK);
                m_buffer->setFgColor(WHITE);
                m_buffer->setBold(false);
                break;

            case 1 :
                m_buffer->setBold(true);
                break;

            case 2 :
                m_buffer->setBold(false);
                break;

            case 7 :
                m_buffer->swapFgBgColor();
                break;

            case 30 ... 37 :
                m_buffer->setFgColor(static_cast<TerminalColor>(*it - 30));
                break;

            case 39 :
                m_buffer->setFgColor(WHITE);
                break;

            case 40 ... 47 :
                m_buffer->setBgColor(static_cast<TerminalColor>(*it - 40));
                break;

            case 49 :
                m_buffer->setBgColor(BLACK);
                break;

            default :
                dbprintf("TerminalParser::updateMode(): unknown parameter: %d.\n", *it);
                break;
        }
    }
}
