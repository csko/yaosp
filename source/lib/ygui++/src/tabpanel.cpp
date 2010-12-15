/* yaosp GUI library
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

#include <ygui++/tabpanel.hpp>

namespace yguipp {

TabPanel::TabPanel(void) {
    m_font = new Font("DejaVu Sans", "Book", 11);
    m_font->init();
    m_font->incRef();
}

int TabPanel::addTab(const std::string& title, Widget* component) {
    m_tabs.push_back(std::make_pair<std::string, Widget*>(title, component));
    return 0;
}

int TabPanel::validate(void) {
    return 0;
}

int TabPanel::paint(GraphicsContext* g) {
    Rect bounds = getBounds();

    int tabHeight = m_font->getHeight() + 5;

    g->setPenColor(Color(216, 216, 216));
    g->rectangle(Rect(0, 0, bounds.width() - 1, tabHeight - 1));
    g->fill();

    int leftOffset = 10;
    //int rightOffset = 10;

    g->setFont(m_font);
    g->setPenColor(Color(0, 0, 0));
    g->setClipRect(Rect(leftOffset, 0, bounds.width() - (leftOffset + rightOffset) - 1, tabHeight - 1));
    g->setLineWidth(1.0f);

    int x = leftOffset;

    for (std::vector< std::pair<std::string, Widget*> >::const_iterator it = m_tabs.begin();
         it != m_tabs.end();
         ++it) {
        const std::pair<std::string, Widget*>& tabInfo = *it;

        const std::string& title = tabInfo.first;
        int titleWidth = m_font->getWidth(title);

        g->moveTo(Point(x, tabHeight - 1));
        g->lineTo(Point(x, 0));
        g->lineTo(Point(x + titleWidth + 5, 0));
        g->lineTo(Point(x + titleWidth + 5, tabHeight - 1));
        g->stroke();

        g->moveTo(Point(x + 2, m_font->getAscender() - 1));
        g->showText(title);

        x += titleWidth + 5;
    }

    return 0;
}

} /* namespace yguipp */
