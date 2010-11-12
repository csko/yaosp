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

#include <unistd.h>
#include <pty.h>
#include <yaosp/debug.h>

#include <ygui++/application.hpp>
#include <ygui++/window.hpp>
#include <ygui++/panel.hpp>
#include <ygui++/scrollpanel.hpp>
#include <ygui++/layout/borderlayout.hpp>

#include "terminal.hpp"
#include "terminalview.hpp"

using namespace yguipp;

Terminal::Terminal(void) : m_masterPty(-1), m_slaveTty(-1), m_ptyHandler(NULL) {
    m_buffer = new TerminalBuffer(80, 25);
    m_parser = new TerminalParser(m_buffer);
}

int Terminal::run(void) {
    Application::createInstance("terminal");

    if (!startShell()) {
        return EXIT_FAILURE;
    }

    TerminalView* termView = new TerminalView(m_buffer);

    Window* window = new Window(
        "Terminal", Point(50,50),
        Point(termView->getFont()->getWidth("a") * 80, termView->getFont()->getHeight() * 25)
    );
    window->init();

    Panel* container = dynamic_cast<Panel*>(window->getContainer());
    container->setLayout(new layout::BorderLayout());

    ScrollPanel* scrollPanel = new ScrollPanel();
    scrollPanel->add(termView);

    container->add(scrollPanel, new layout::BorderLayoutData(layout::BorderLayoutData::CENTER));

    window->show();
    m_ptyHandler = new PtyHandler(m_masterPty, termView, m_parser);
    m_ptyHandler->start();
    Application::getInstance()->mainLoop();

    return EXIT_SUCCESS;
}

bool Terminal::startShell(void) {
    if (openpty(&m_masterPty, &m_slaveTty, NULL, NULL, NULL) != 0) {
        return false;
    }

    if (fork() == 0) {
        dup2(m_slaveTty, 0);
        dup2(m_slaveTty, 1);
        dup2(m_slaveTty, 2);
        close(m_slaveTty);

        if (execl("/application/bash", "bash", NULL) != 0) {
            dbprintf("Failed to execute shell!\n");
        }

        _exit(EXIT_FAILURE);
    }

    return true;
}
