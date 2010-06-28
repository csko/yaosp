/* Terminal application
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

#include <ygui++/application.hpp>
#include <ygui++/window.hpp>
#include <ygui++/panel.hpp>
#include <ygui++/layout/borderlayout.hpp>

#include "terminal.hpp"
#include "terminalview.hpp"

using namespace yguipp;

int Terminal::run(void) {
    Application::createInstance("terminal");

    TerminalView* termView = new TerminalView();

    Window* window = new Window(
        "Terminal", Point(50,50),
        Point(termView->getFont()->getStringWidth("a") * 80, termView->getFont()->getHeight() * 25)
    );
    window->init();

    Panel* container = dynamic_cast<Panel*>(window->getContainer());
    container->setLayout(new layout::BorderLayout());
    container->add(termView, new layout::BorderLayoutData(layout::BorderLayoutData::CENTER));

    window->show();
    Application::getInstance()->mainLoop();

    return 0;
}
