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

#include <guiserver/mouse.hpp>
#include <guiserver/graphicsdriver.hpp>

#define MW 0xFFFFFFFF
#define MB 0xFF000000
#define MI 0x00000000

uint32_t MousePointer::pointerImage[ 16 * 16 ] = {
    MW, MW, MI, MI, MI, MI, MI, MI, MI, MI, MI, MI, MI, MI, MI, MI,
    MW, MB, MW, MI, MI, MI, MI, MI, MI, MI, MI, MI, MI, MI, MI, MI,
    MW, MB, MB, MW, MI, MI, MI, MI, MI, MI, MI, MI, MI, MI, MI, MI,
    MW, MB, MB, MB, MW, MI, MI, MI, MI, MI, MI, MI, MI, MI, MI, MI,
    MW, MB, MB, MB, MB, MW, MI, MI, MI, MI, MI, MI, MI, MI, MI, MI,
    MW, MB, MB, MB, MB, MB, MW, MI, MI, MI, MI, MI, MI, MI, MI, MI,
    MW, MB, MB, MB, MB, MB, MB, MW, MI, MI, MI, MI, MI, MI, MI, MI,
    MW, MB, MB, MB, MB, MB, MB, MB, MW, MI, MI, MI, MI, MI, MI, MI,
    MW, MB, MB, MB, MB, MB, MB, MB, MB, MW, MI, MI, MI, MI, MI, MI,
    MW, MB, MB, MB, MB, MB, MW, MW, MW, MW, MW, MI, MI, MI, MI, MI,
    MW, MB, MB, MW, MB, MB, MW, MI, MI, MI, MI, MI, MI, MI, MI, MI,
    MW, MB, MW, MW, MW, MB, MB, MW, MI, MI, MI, MI, MI, MI, MI, MI,
    MW, MW, MI, MI, MW, MB, MB, MW, MI, MI, MI, MI, MI, MI, MI, MI,
    MI, MI, MI, MI, MI, MW, MB, MB, MW, MI, MI, MI, MI, MI, MI, MI,
    MI, MI, MI, MI, MI, MW, MB, MB, MW, MI, MI, MI, MI, MI, MI, MI,
    MI, MI, MI, MI, MI, MI, MW, MW, MW, MI, MI, MI, MI, MI, MI, MI
};

MousePointer::MousePointer(void) : m_pointer(NULL), m_screenBuffer(NULL), m_visible(false) {
}

bool MousePointer::init(void) {
    m_pointer = new Bitmap(16, 16, yguipp::CS_RGB32, reinterpret_cast<uint8_t*>(pointerImage) );
    m_screenBuffer = new Bitmap(16, 16, yguipp::CS_RGB32);

    return true;
}

void MousePointer::show(GraphicsDriver* driver, Bitmap* screen) {
    driver->blitBitmap(m_screenBuffer, yguipp::Point(0,0), screen, m_pointerRect, yguipp::DM_COPY);
    driver->blitBitmap(screen, m_position, m_pointer, m_pointer->bounds(), yguipp::DM_BLEND);

    m_visible = true;
}

bool MousePointer::hide(GraphicsDriver* driver, Bitmap* screen) {
    if (!m_visible) {
        return false;
    }

    driver->blitBitmap(screen, m_position, m_screenBuffer, m_screenBuffer->bounds(), yguipp::DM_COPY);

    m_visible = false;

    return true;
}

int MousePointer::moveTo(GraphicsDriver* driver, Bitmap* screen, const yguipp::Point& position) {
    bool doShow = hide(driver, screen);

    m_position = position;
    m_pointerRect = yguipp::Rect(
        position.m_x, position.m_y,
        position.m_x + m_pointer->width() - 1,
        position.m_y + m_pointer->height() - 1
    );

    if (doShow) {
        show(driver, screen);
    }

    return 0;
}

int MousePointer::moveBy(GraphicsDriver* driver, Bitmap* screen, yguipp::Point offset) {
    bool doShow = hide(driver, screen);

    m_position += offset;

    if (m_position.m_x < 0) { m_position.m_x = 0; }
    else if (m_position.m_x >= (int)screen->width()) { m_position.m_x = screen->width() - 1; }
    if (m_position.m_y < 0) { m_position.m_y = 0; }
    else if (m_position.m_y >= (int)screen->height()) { m_position.m_y = screen->height() - 1; }

    m_pointerRect = yguipp::Rect(
        m_position.m_x, m_position.m_y,
        m_position.m_x + m_pointer->width() - 1,
        m_position.m_y + m_pointer->height() - 1
    );

    if (doShow) {
        show(driver, screen);
    }

    return 0;
}
