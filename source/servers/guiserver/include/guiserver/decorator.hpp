/* GUI server
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

#ifndef _DECORATOR_HPP_
#define _DECORATOR_HPP_

#include <ygui++/point.hpp>

class Window;
class GraphicsDriver;

class DecoratorData {
};

class Decorator {
  public:
    virtual ~Decorator(void) {}

    virtual yguipp::Point leftTop(void) = 0;
    virtual yguipp::Point getSize(void) = 0;

    virtual DecoratorData* createWindowData(void) = 0;

    virtual int update(GraphicsDriver* driver, Window* window) = 0;
};

#endif /* _DECORATOR_HPP_ */
