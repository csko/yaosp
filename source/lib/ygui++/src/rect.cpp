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

#include <ygui++/rect.hpp>

namespace yguipp {

Rect::Rect( int left, int top, int right, int bottom ) : m_left(left), m_top(top),
                                                         m_right(right), m_bottom(bottom) {
}

void Rect::toRectT( rect_t* r ) const {
    r->left = m_left;
    r->top = m_top;
    r->right = m_right;
    r->bottom = m_bottom;
}

} /* namespace yguipp */
