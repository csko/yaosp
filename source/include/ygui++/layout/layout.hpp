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

#ifndef _LAYOUT_LAYOUT_HPP_
#define _LAYOUT_LAYOUT_HPP_

#include <ygui++/object.hpp>

namespace yguipp {

class Panel;

namespace layout {

class LayoutData {
}; /* class LayoutData */

class Layout : public Object {
  public:
    virtual ~Layout( void ) {}

    virtual int doLayout( yguipp::Panel* panel ) = 0;
}; /* class Layout */

} /* namespace layout */
} /* namespace yguipp */

#endif /* _LAYOUT_LAYOUT_HPP_ */
