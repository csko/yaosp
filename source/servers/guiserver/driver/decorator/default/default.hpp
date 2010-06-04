/* Default window decorator
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

#ifndef _DECORATOR_DEFAULT_HPP_
#define _DECORATOR_DEFAULT_HPP_

#include <guiserver/decorator.hpp>

class DefaultDecoratorData : public DecoratorData {
};

class DefaultDecorator : public Decorator {
  public:
    DefaultDecorator(GuiServer* guiServer);
    virtual ~DefaultDecorator(void) {}

    yguipp::Point leftTop(void);
    yguipp::Point getSize(void);

    DecoratorData* createWindowData(void);

    int update(GraphicsDriver* driver, Window* window);

  private:
    FontNode* m_titleFont;

  private:
    static const size_t BORDER_LEFT = 3;
    static const size_t BORDER_TOP = 21;
    static const size_t BORDER_RIGHT = 3;
    static const size_t BORDER_BOTTOM = 3;

    static const yguipp::Color TOP_BORDER_COLORS[BORDER_TOP];
}; /* class DefaultDecorator */

#endif /* _DECORATOR_DEFAULT_HPP_ */
