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

#ifndef _MOUSE_HPP_
#define _MOUSE_HPP_

#include <ygui++/point.hpp>

#include <guiserver/bitmap.hpp>

class GraphicsDriver;

class MousePointer {
  public:
    MousePointer(void);

    bool init(void);

    void show(GraphicsDriver* driver, Bitmap* screen);
    bool hide(GraphicsDriver* driver, Bitmap* screen);

    int moveTo(GraphicsDriver* driver, Bitmap* screen, const yguipp::Point& position);
    int moveBy(GraphicsDriver* driver, Bitmap* screen, yguipp::Point offset);

  private:

  private:
    Bitmap* m_pointer;
    Bitmap* m_screenBuffer;

    bool m_visible;
    yguipp::Point m_position;
    yguipp::Rect m_pointerRect;

    static uint32_t pointerImage[16*16];
}; /* class MousePointer */

#endif /* _MOUSE_HPP_ */
