/* Desktop application
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

#include <ygui++/panel.hpp>

#include "desktop.hpp"

using namespace yguipp;

Desktop::Desktop(void) : m_window(NULL) {
}

int Desktop::run(void) {
    Application::createInstance("desktop");

    Application* app = Application::getInstance();
    app->addListener(this);

    ScreenModeInfo modeInfo = app->getCurrentScreenMode();
    m_window = new Window(
        "desktop", Point(0, 0), Point(modeInfo.m_width, modeInfo.m_height), WINDOW_NO_BORDER, WINDOW_ORDER_ALWAYS_ON_BOTTOM
    );
    m_window->init();

    Panel* panel = dynamic_cast<Panel*>(m_window->getContainer());
    panel->setBackgroundColor(Color(75, 100, 125));

    m_window->show();
    app->mainLoop();

    return 0;
}

void Desktop::onScreenModeChanged(const yguipp::ScreenModeInfo& modeInfo) {
    m_window->resize(Point(modeInfo.m_width, modeInfo.m_height));
}
