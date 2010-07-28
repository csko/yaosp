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

#include <yaosp/debug.h>

#include <guiserver/decorator.hpp>
#include <guiserver/window.hpp>

DecoratorData* Decorator::createWindowData(void) {
    return new DecoratorData();
}

int Decorator::mouseEntered(Window* window, const yguipp::Point& position) {
    return 0;
}

int Decorator::mouseMoved(Window* window, const yguipp::Point& position) {
    return 0;
}

int Decorator::mouseExited(Window* window) {
    return 0;
}

int Decorator::mousePressed(Window* window, const yguipp::Point& position, int button) {
    DecoratorItem item = checkHit(window, position);

    if (item == ITEM_CLOSE) {
        window->closeRequest();
    }

    return 0;
}

int Decorator::mouseReleased(Window* window, int button) {
    return 0;
}

DecoratorItem Decorator::checkHit(Window* window, const yguipp::Point& position) {
    DecoratorData* data = window->getDecoratorData();

    if (data->m_closeRect.hasPoint(position)) {
        return ITEM_CLOSE;
    }

    return ITEM_NONE;
}
