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

#include <ygui++/panel.hpp>

namespace yguipp {

Panel::Panel( void ) {
}

Panel::~Panel( void ) {
}

int Panel::paint( GraphicsContext* g ) {
    g->setPenColor( Color(127, 127, 127) );
    g->fillRect( Rect(0, 0, 99, 99) );

    return 0;
}

} /* namespace yguipp */
