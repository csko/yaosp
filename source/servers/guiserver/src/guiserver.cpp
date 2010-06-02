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

#include <guiserver/guiserver.hpp>

#include "../driver/video/vesa/vesa.hpp"

GuiServer::GuiServer( void ) : m_graphicsDriver(NULL), m_screenBitmap(NULL), m_windowManager(NULL) {
}

int GuiServer::run( void ) {
    /* Setup screen mode. */

    m_graphicsDriver = new VesaDriver();
    m_graphicsDriver->detect();

    ScreenMode mode(640, 480, CS_RGB32);
    m_graphicsDriver->setMode(&mode);

    m_screenBitmap = new Bitmap(
        640, 480, CS_RGB32,
        reinterpret_cast<uint8_t*>(m_graphicsDriver->getFrameBuffer())
    );

    /* Initialize other stuffs. */

    m_windowManager = new WindowManager(m_graphicsDriver, m_screenBitmap);
    m_inputThread = new InputThread(m_windowManager);
    m_inputThread->init();

    m_graphicsDriver->fillRect(
        m_screenBitmap, m_screenBitmap->bounds(), yguipp::Rect(0, 0, 639, 479), yguipp::Color(75, 100, 125), DM_COPY
    );

    m_windowManager->enable();
    m_inputThread->start();

    return 0;
}
