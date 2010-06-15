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

#ifndef _BORDER_HPP_
#define _BORDER_HPP_

#include <ygui++/object.hpp>
#include <ygui++/point.hpp>

namespace yguipp {

class Widget;
class GraphicsContext;

namespace border {

class Border : public yguipp::Object {
  public:
    virtual ~Border(void) {}

    virtual yguipp::Point size(void) = 0;
    virtual yguipp::Point leftTop(void) = 0;

    virtual int paint(Widget* widget, GraphicsContext* gc) = 0;
}; /* class Border */

} /* namespace border */
} /* namespace yguipp */

#endif /* _BORDER_HPP_ */
