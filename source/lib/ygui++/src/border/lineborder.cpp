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

#include <ygui++/widget.hpp>
#include <ygui++/graphicscontext.hpp>
#include <ygui++/border/lineborder.hpp>

namespace yguipp {
namespace border {

LineBorder::LineBorder(int lineSize, int spacing) : m_lineSize(lineSize), m_spacing(spacing) {
}

yguipp::Point LineBorder::size(void) {
    return yguipp::Point((m_lineSize + m_spacing) * 2, (m_lineSize + m_spacing) * 2);
}

yguipp::Point LineBorder::leftTop(void) {
    return yguipp::Point(m_lineSize + m_spacing, m_lineSize + m_spacing);
}

int LineBorder::paint(Widget* widget, GraphicsContext* gc) {
    const Point& size = widget->getSize();

    gc->setPenColor(Color(0,0,0));
    gc->fillRect(Rect(0,0,size.m_x - 1, m_lineSize - 1));
    gc->fillRect(Rect(0,size.m_y - m_lineSize + 1, size.m_x - 1, size.m_y - 1));
    gc->fillRect(Rect(0,0,m_lineSize - 1,size.m_y - 1));
    gc->fillRect(Rect(size.m_x - m_lineSize + 1, 0, size.m_x - 1, size.m_y - 1));

    gc->setPenColor(Color(255,255,255));
    gc->fillRect(Rect(m_lineSize, m_lineSize, size.m_x - (m_lineSize + 1), m_lineSize + m_spacing - 1));
    gc->fillRect(Rect(m_lineSize, m_lineSize, m_lineSize + m_spacing - 1, size.m_y - m_lineSize));

    return 0;
}

} /* namespace border */
} /* namespace yguipp */
