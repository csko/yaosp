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

#include <guiserver/graphicsdriver.hpp>
#include <guiserver/window.hpp>
#include <guiserver/guiserver.hpp>

#include "default.hpp"

const yguipp::Color DefaultDecorator::TOP_BORDER_COLORS[DefaultDecorator::BORDER_TOP] = {
    yguipp::Color(0x6D, 0x6D, 0x6D),
    yguipp::Color(0x5D, 0x5D, 0x5D),
    yguipp::Color(0x50, 0x50, 0x50),
    yguipp::Color(0x48, 0x48, 0x48),
    yguipp::Color(0x45, 0x45, 0x45),
    yguipp::Color(0x45, 0x45, 0x45),
    yguipp::Color(0x46, 0x46, 0x46),
    yguipp::Color(0x48, 0x48, 0x48),
    yguipp::Color(0x4A, 0x4A, 0x4A),
    yguipp::Color(0x4B, 0x4B, 0x4B),
    yguipp::Color(0x4D, 0x4D, 0x4D),
    yguipp::Color(0x4F, 0x4F, 0x4F),
    yguipp::Color(0x51, 0x51, 0x51),
    yguipp::Color(0x52, 0x52, 0x52),
    yguipp::Color(0x53, 0x53, 0x53),
    yguipp::Color(0x55, 0x55, 0x55),
    yguipp::Color(0x54, 0x54, 0x54),
    yguipp::Color(0x52, 0x52, 0x52),
    yguipp::Color(0x4B, 0x4B, 0x4B),
    yguipp::Color(0x41, 0x41, 0x41),
    yguipp::Color(0x33, 0x33, 0x33)
};

DefaultDecorator::DefaultDecorator(GuiServer* guiServer) {
    m_titleFont = guiServer->getFontStorage()->getFontNode("DejaVu Sans", "Bold", FontInfo(8, FONT_SMOOTHED));
}

yguipp::Point DefaultDecorator::leftTop(void) {
    return yguipp::Point(BORDER_LEFT, BORDER_TOP);
}

yguipp::Point DefaultDecorator::getSize(void) {
    return yguipp::Point(BORDER_LEFT + BORDER_RIGHT, BORDER_TOP + BORDER_BOTTOM);
}

DecoratorData* DefaultDecorator::createWindowData(void) {
    return new DefaultDecoratorData();
}

int DefaultDecorator::update(GraphicsDriver* driver, Window* window) {
    Bitmap* bitmap = window->getBitmap();

    /* Border */

    for ( size_t i = 0; i < BORDER_TOP; i++ ) {
        driver->fillRect(bitmap, bitmap->bounds(), yguipp::Rect(0,i,bitmap->width()-1,i), TOP_BORDER_COLORS[i], DM_COPY);
    }

    driver->fillRect(
        bitmap, bitmap->bounds(), yguipp::Rect(0,BORDER_TOP,BORDER_LEFT-1,bitmap->height()-1),
        TOP_BORDER_COLORS[BORDER_TOP-1], DM_COPY
    );
    driver->fillRect(
        bitmap, bitmap->bounds(),
        yguipp::Rect(bitmap->width()-BORDER_RIGHT+1,BORDER_TOP,bitmap->width()-1,bitmap->height()-1),
        TOP_BORDER_COLORS[BORDER_TOP-1], DM_COPY
    );
    driver->fillRect(
        bitmap, bitmap->bounds(),
        yguipp::Rect(BORDER_LEFT,bitmap->height()-BORDER_BOTTOM+1,bitmap->width()-BORDER_RIGHT,bitmap->height()-1),
        TOP_BORDER_COLORS[BORDER_TOP-1], DM_COPY
    );

    /* Title */

    const std::string& title = window->getTitle();

    driver->drawText(
        bitmap, bitmap->bounds(),
        yguipp::Point(
            (bitmap->width() - m_titleFont->getWidth(title.c_str(), title.size())) / 2,
            (BORDER_TOP - (m_titleFont->getAscender() - m_titleFont->getDescender())) / 2 + m_titleFont->getAscender()
        ),
        yguipp::Color(255,255,255), m_titleFont, title.c_str(), title.size()
    );

    return 0;
}
