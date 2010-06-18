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

#include <yaosp/debug.h>

#include <ygui++/application.hpp>
#include <ygui++/window.hpp>
#include <ygui++/panel.hpp>
#include <ygui++/imageloader.hpp>
#include <ygui++/menuitem.hpp>
#include <ygui++/layout/borderlayout.hpp>

#include "taskbar.hpp"

using namespace yguipp;

MenuButton::MenuButton(void) {
    m_image = Bitmap::loadFromFile("/system/images/unknown.png");
}

Point MenuButton::getPreferredSize(void) {
    return m_image->getSize();
}

int MenuButton::paint(GraphicsContext* g) {
    g->setDrawingMode(DM_BLEND);
    g->drawBitmap(Point(0,0), m_image);
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

    Point size = app->getDesktopSize();
    Point position(0, size.m_y - 22);
    size.m_y = 22;

    m_window = new Window("taskbar", position, size, WINDOW_NO_BORDER);
    m_window->init();
    Panel* container = dynamic_cast<Panel*>(m_window->getContainer());
    container->setLayout(new layout::BorderLayout());

    m_menuButton = new MenuButton();
    m_menuButton->addActionListener(this);
    container->add(m_menuButton, new layout::BorderLayoutData(layout::BorderLayoutData::LINE_START));

    m_menu = new Menu();
    m_menu->add(new MenuItem("Hello"));
    m_menu->add(new MenuItem("World"));

    m_window->show();
    app->mainLoop();

    return 0;
}

int TaskBar::actionPerformed(yguipp::Widget* widget) {
    Point size = m_menu->getPreferredSize();
    m_menu->show(m_window->getPosition() - Point(0,size.m_y));

    return 0;
}
