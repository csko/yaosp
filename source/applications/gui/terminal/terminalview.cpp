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

#include "terminalview.hpp"

TerminalView::TerminalView(TerminalBuffer* buffer) : m_buffer(buffer) {
    m_font = new yguipp::Font("DejaVu Sans Mono", "Book", yguipp::FontInfo(8));
    m_font->init();
}

yguipp::Font* TerminalView::getFont(void) {
    return m_font;
}

int TerminalView::paint(yguipp::GraphicsContext* g) {
    g->setPenColor(yguipp::Color(0, 0, 0));
    g->fillRect(yguipp::Rect(getVisibleSize()));

    g->setPenColor(yguipp::Color(255, 255, 255));
    g->setFont(m_font);

    yguipp::Point pos(0, m_font->getAscender());
    for (int i = 0; i < m_buffer->getHeight(); i++) {
        TerminalLine* line = m_buffer->lineAt(i);
        g->drawText(pos, line->m_text, line->m_dirtyWidth);
        pos.m_y += m_font->getHeight();
    }

    return 0;
}
