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

#include <ygui++/rect.hpp>

class Window;
class GraphicsDriver;

class Decorator;

class DecoratorData {
  public:
    yguipp::Rect m_minimizeRect;
    yguipp::Rect m_maximizeRect;
    yguipp::Rect m_closeRect;
}; /* class DecoratorData */

enum DecoratorItem {
    ITEM_NONE,
    ITEM_MINIMIZE,
    ITEM_MAXIMIZE,
    ITEM_CLOSE
};

class Decorator {
  public:
    virtual ~Decorator(void) {}

    virtual DecoratorData* createWindowData(void);

    virtual int mouseEntered(Window* window, const yguipp::Point& position);
    virtual int mouseMoved(Window* window, const yguipp::Point& position);
    virtual int mouseExited(Window* window);
    virtual int mousePressed(Window* window, const yguipp::Point& position, int button);
    virtual int mouseReleased(Window* window, int button);

    virtual yguipp::Point leftTop(void) = 0;
    virtual yguipp::Point getSize(void) = 0;

    virtual bool calculateItemPositions(Window* window) = 0;
    virtual DecoratorItem checkHit(Window* window, const yguipp::Point& position);

    virtual bool update(GraphicsDriver* driver, Window* window) = 0;
}; /* class Decorator */

#endif /* _DECORATOR_HPP_ */
