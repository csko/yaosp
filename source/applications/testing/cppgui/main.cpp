/* C++ gui tester application
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

#include <iostream>

#include <ygui++/application.hpp>
#include <ygui++/window.hpp>
#include <ygui++/label.hpp>
#include <ygui++/panel.hpp>

int main( int argc, char** argv ) {
    yguipp::Application::createInstance("cppguitest");

    yguipp::Window* win = new yguipp::Window( "Test", yguipp::Point(50,50), yguipp::Point(300,300) );
    win->init();

    yguipp::Widget* container = win->getContainer();
    dynamic_cast<yguipp::Panel*>(container)->setBackgroundColor( yguipp::Color(255,255,255) );

    yguipp::Widget* child = new yguipp::Label("Hello World");
    container->addChild(child);
    child->setPosition( yguipp::Point(50,50) );
    child->setSize( yguipp::Point(200,200) );

    win->show();

    yguipp::Application::getInstance()->run();

    return 0;
}
