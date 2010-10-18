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

#include <assert.h>
#include <unistd.h>
#include <yaosp/debug.h>

#include <ygui++/application.hpp>
#include <ygui++/window.hpp>
#include <ygui++/panel.hpp>
#include <ygui++/imageloader.hpp>
#include <ygui++/menuitem.hpp>
#include <ygui++/layout/borderlayout.hpp>
#include <yconfig++/connection.hpp>

#include "taskbar.hpp"
#include "menuitem.hpp"

using namespace yguipp;

MenuButton::MenuButton(void) {
    m_image = Bitmap::loadFromFile("/system/images/unknown.png");
}

Point MenuButton::getPreferredSize(void) {
    if (m_image == NULL) {
        return Point(0, 0);
    }

    return m_image->getSize();
}

int MenuButton::paint(GraphicsContext* g) {
    if (m_image == NULL) {
        return 0;
    }

    g->setDrawingMode(DM_BLEND);
    g->drawBitmap((getSize() - m_image->getSize()) / 2, m_image);
    g->setDrawingMode(DM_COPY);

    return 0;
}

int MenuButton::mousePressed(const Point& p, int button) {
    fireActionListeners(this);
    return 0;
}

int TaskBar::run(void) {
    Application::createInstance("taskbar");
    ImageLoaderManager::getInstance()->loadLibraries();

    Application* app = Application::getInstance();
    app->addListener(this);

    ScreenModeInfo modeInfo = app->getCurrentScreenMode();
    Point size(modeInfo.m_width, 22);
    Point position(0, modeInfo.m_height - 22);

    m_window = new Window("taskbar", position, size, WINDOW_NO_BORDER);
    m_window->init();
    Panel* container = dynamic_cast<Panel*>(m_window->getContainer());
    container->setLayout(new layout::BorderLayout());

    m_menuButton = new MenuButton();
    m_menuButton->addActionListener(this);
    container->add(m_menuButton, new layout::BorderLayoutData(layout::BorderLayoutData::LINE_START));

    createMenu();

    m_window->show();
    app->mainLoop();

    return 0;
}

int TaskBar::actionPerformed(yguipp::Widget* widget) {
    if (widget == m_menuButton) {
        Point size = m_menu->getPreferredSize();
        m_menu->show(m_window->getPosition() - Point(0,size.m_y));
        return 0;
    }

    TBMenuItem* menuItem = dynamic_cast<TBMenuItem*>(widget);
    assert(menuItem != NULL);

    const std::string& executable = menuItem->getExecutable();
    dbprintf("Starting '%s'\n", executable.c_str());

    if (fork() == 0) {
        execlp(executable.c_str(), executable.c_str(), NULL);
        exit(0);
    }

    return 0;
}

void TaskBar::onScreenModeChanged(const ScreenModeInfo& modeInfo) {
    dbprintf("New screen size: %dx%d\n", modeInfo.m_width, modeInfo.m_height);
}

void TaskBar::createMenu(void) {
    std::vector<TBMenuItemInfo> menuItems;

    {
        yconfigpp::Connection config;
        config.init();

        std::vector<std::string> children;
        config.listChildren("application/taskbar/menu", children);

        for (std::vector<std::string>::const_iterator it = children.begin();
             it != children.end();
             ++it) {
            std::string name;
            uint64_t position;
            std::string executable;
            std::string path = "application/taskbar/menu/" + *it;
            Bitmap* image = NULL;

            if ((!config.getNumericValue(path, "position", position)) ||
                (!config.getAsciiValue(path, "name", name)) ||
                (!config.getAsciiValue(path, "executable", executable))) {
                continue;
            }

            uint8_t* data;
            size_t length;

            if (config.getBinaryValue(path, "image", data, length)) {
                image = Bitmap::loadFromBuffer(data, length);
                delete[] data;
            }

            menuItems.push_back(TBMenuItemInfo(position, name, executable, image));
        }
    }

    std::sort(menuItems.begin(), menuItems.end());

    m_menu = new Menu();

    for (std::vector<TBMenuItemInfo>::const_iterator it = menuItems.begin();
         it != menuItems.end();
         ++it) {
        MenuItem* menuItem = new TBMenuItem(*it);
        menuItem->addActionListener(this);

        m_menu->add(menuItem);
    }
}
