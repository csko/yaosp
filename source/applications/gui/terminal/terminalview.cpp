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
#include "terminal.hpp"

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

TerminalView::TerminalView(Terminal* terminal, TerminalBuffer* buffer) : m_terminal(terminal), m_buffer(buffer) {
    buffer->addListener(this);

    m_font = new yguipp::Font("DejaVu Sans Mono", "Book", yguipp::FontInfo(11));
    m_font->init();
}

yguipp::Font* TerminalView::getFont(void) {
    return m_font;
}

yguipp::Point TerminalView::getPreferredSize(void) {
    return yguipp::Point(
        m_font->getWidth("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"),
        std::max(25, m_buffer->getLineCount()) * m_font->getHeight()
    );
}

int TerminalView::paint(yguipp::GraphicsContext* g) {
    int cursorX;
    int cursorY;
    int firstLine;
    int lastLine;
    yguipp::Point p;
    yguipp::Point visibleSize = getVisibleSize();
    yguipp::Point scrollOffset = getScrollOffset();

    g->setPenColor(yguipp::Color(0, 0, 0));
    g->rectangle(yguipp::Rect(visibleSize) - scrollOffset);
    g->fill();

    g->setPenColor(yguipp::Color(255, 255, 255));
    g->setFont(m_font);

    m_buffer->lock();

    firstLine = -scrollOffset.m_y / m_font->getHeight();
    lastLine = firstLine + (visibleSize.m_y + m_font->getHeight() - 1) / m_font->getHeight();
    lastLine = std::min(lastLine, m_buffer->getLineCount() - 1);

    p.m_y = firstLine * m_font->getHeight() + m_font->getAscender();

    for (int i = firstLine; i <= lastLine; i++) {
        paintLine(g, i, p);
        p.m_y += m_font->getHeight();
    }

    m_buffer->getCursorPosition(cursorX, cursorY);
    cursorY += m_buffer->getHistorySize();

    if ((cursorY >= firstLine) &&
        (cursorY <= lastLine)) {
        paintCursor(g, cursorX, cursorY, m_buffer->lineAt(cursorY));
    }

    m_buffer->unLock();

    return 0;
}

int TerminalView::onHistoryChanged(TerminalBuffer* buffer) {
    fireWidgetResizedListeners(this);
    m_terminal->scrollToBottom();

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
    int textWidth = m_font->getWidth(text);

    if (attr.m_bgColor != BLACK) {
        yguipp::Rect rect;

        rect.m_left = position.m_x;
        rect.m_right = position.m_x + textWidth - 1;
        rect.m_top = position.m_y - m_font->getAscender() + 1;
        rect.m_bottom = rect.m_top + m_font->getHeight() - 1;

        g->setPenColor(m_normalColors[attr.m_bgColor]);
        g->rectangle(rect);
        g->fill();
    }

    g->setPenColor(attr.m_bold ? m_boldColors[attr.m_fgColor] : m_normalColors[attr.m_fgColor]);
    g->moveTo(position);
    g->showText(text);

    position.m_x += textWidth;

    return 0;
}

int TerminalView::paintCursor(yguipp::GraphicsContext* g, int cursorX, int cursorY, TerminalLine* line) {
    int charWidth;
    yguipp::Rect rect;
    yguipp::Point p;

    charWidth = m_font->getWidth("a");
    p.m_x = charWidth * cursorX;
    p.m_y = m_font->getHeight() * cursorY;

    rect.m_left = p.m_x;
    rect.m_right = p.m_x + charWidth - 1;
    rect.m_top = p.m_y;
    rect.m_bottom = p.m_y + m_font->getHeight() - 1;

    p.m_y += m_font->getAscender();

    const TerminalAttribute& attr = line->m_attr[cursorX];

    g->setPenColor(m_normalColors[attr.m_fgColor]);
    g->rectangle(rect);
    g->fill();

    g->setPenColor(attr.m_bold ? m_boldColors[attr.m_bgColor] : m_normalColors[attr.m_bgColor]);
    g->moveTo(p);
    g->showText(line->m_text.substr(cursorX, 1));

    return 0;
}
