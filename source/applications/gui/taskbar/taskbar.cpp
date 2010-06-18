/* Taskbar application
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

#include <ygui++/application.hpp>
#include <ygui++/window.hpp>
#include <ygui++/panel.hpp>
#include <ygui++/layout/borderlayout.hpp>

#include "taskbar.hpp"

using namespace yguipp;

int TaskBar::run(void) {
    Application::createInstance("taskbar");
    Application* app = Application::getInstance();

    Point size = app->getDesktopSize();
    Point position(0, size.m_y - 22);
    size.m_y = 22;

    Window* window = new Window("taskbar", position, size, WINDOW_NO_BORDER);
    window->init();
    Panel* container = dynamic_cast<Panel*>(window->getContainer());
    container->setLayout(new layout::BorderLayout());

    window->show();

    app->mainLoop();

    return 0;
}
