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

yguipp::Color TerminalView::m_boldColors[COLOR_COUNT] = {
    yguipp::Color(0x55, 0x55, 0x55),
    yguipp::Color(0xFF, 0x55, 0x55),
    yguipp::Color(0x55, 0xFF, 0x55),
    yguipp::Color(0xFF, 0xFF, 0x55),
    yguipp::Color(0x55, 0x55, 0xFF),
    yguipp::Color(0xFF, 0x55, 0xFF),
    yguipp::Color(0x55, 0xFF, 0xFF),
    yguipp::Color(0xFF, 0xFF, 0xFF)
};

yguipp::Color TerminalView::m_normalColors[COLOR_COUNT] = {
    yguipp::Color(0x00, 0x00, 0x00),
    yguipp::Color(0xAA, 0x00, 0x00),
    yguipp::Color(0x00, 0xAA, 0x00),
    yguipp::Color(0xAA, 0x55, 0x00),
    yguipp::Color(0x00, 0x00, 0xAA),
    yguipp::Color(0xAA, 0x00, 0xAA),
    yguipp::Color(0x00, 0xAA, 0xAA),
    yguipp::Color(0xAA, 0xAA, 0xAA)
};

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
        paintLine(g, i, pos);
        pos.m_y += m_font->getHeight();
    }

    return 0;
}

int TerminalView::paintLine(yguipp::GraphicsContext* g, int lineIndex, yguipp::Point position) {
    int index = 0;
    TerminalLine* line = m_buffer->lineAt(lineIndex);

    while (index < line->m_dirtyWidth) {
        int i;
        const TerminalAttribute& attr = line->m_attr[index];

        for (i = index + 1;
             (i < line->m_dirtyWidth) && (line->m_attr[i] == attr);
             i++) {
            /* do nothing */
        }

        paintLinePart(g, line, position, index, i - 1, attr);
        index = i;
    }

    return 0;
}

int TerminalView::paintLinePart(yguipp::GraphicsContext* g, TerminalLine* line, yguipp::Point& position,
                                int start, int end, const TerminalAttribute& attr) {
    std::string text = line->m_text.substr(start, end - start + 1);

    g->setPenColor(attr.m_bold ? m_boldColors[attr.m_fgColor] : m_normalColors[attr.m_fgColor]);
    g->drawText(position, text);

    position.m_x += m_font->getStringWidth(text);

    return 0;
}
