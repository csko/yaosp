/* yaosp GUI library
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

#include <iostream>

#include <ygui++/menu.hpp>
#include <ygui++/menuitem.hpp>
#include <ygui++/border/lineborder.hpp>

namespace yguipp {

MenuItemParent::MenuItemParent(void) : m_parentLevel(NULL) {
}

int MenuItemParent::setParentLevel(MenuItemParent* parentLevel) {
    m_parentLevel = parentLevel;
    return 0;
}

Menu::Menu( void ) : m_activeItem(NULL) {
    m_window = new Window("", Point(0,0), Point(0,0), WINDOW_NO_BORDER|WINDOW_MENU);
    m_window->init();
    m_window->addWindowListener(this);
    //m_window->getContainer()->setBorder(new border::LineBorder());
}

Menu::~Menu( void ) {
    delete m_window;
}

void Menu::add( MenuItem* item ) {
    m_window->getContainer()->add(item);
    item->setMenuParent(this);
}

void Menu::show( const Point& p ) {
    Point size = getPreferredSize();
    m_window->resize(size);
    m_window->moveTo(p);
    doLayout(size);
    m_window->show();
}

void Menu::hide( void ) {
    m_window->hide();

    if (m_activeItem != NULL) {
        Menu* subMenu = m_activeItem->getSubMenu();
        if (subMenu != NULL) {
            hideSubMenu(m_activeItem, subMenu);
        }

        m_activeItem->setActive(false);
        m_activeItem = NULL;
    }
}

int Menu::itemActivated( MenuItem* item ) {
    bool sameItem = (m_activeItem == item);

    if ( (!sameItem) &&
         (m_activeItem != NULL)) {
        Menu* subMenu = m_activeItem->getSubMenu();
        if (subMenu != NULL) {
            hideSubMenu(m_activeItem, subMenu);
            m_activeItem->setActive(false);
        }
    }

    m_activeItem = item;
    m_activeItem->setActive(true);

    if (!sameItem) {
        Menu* subMenu = m_activeItem->getSubMenu();
        if (subMenu != NULL) {
            showSubMenu(m_activeItem, subMenu);
        }
    }

    return 0;
}

int Menu::itemDeActivated( MenuItem* item ) {
    Menu* subMenu;

    assert(m_activeItem == item);

    subMenu = m_activeItem->getSubMenu();
    if (subMenu == NULL) {
        m_activeItem->setActive(false);
    } else {
        subMenu->decRef();
    }

    return 0;
}

int Menu::itemPressed( MenuItem* item ) {
    hideAllLevel();
    return 0;
}

int Menu::hideAllLevel( void ) {
    if (m_parentLevel == NULL) {
        hide();
    } else {
        m_parentLevel->hideAllLevel();
    }

    return 0;
}

int Menu::windowDeActivated( Window* window, DeActivationReason reason ) {
    if (reason == OTHER_WINDOW_CLICKED) {
        hideAllLevel();
    }

    return 0;
}

Point Menu::getPreferredSize( void ) {
    Point size;
    const Widget::ChildVector& children = m_window->getContainer()->getChildren();

    for ( Widget::ChildVectorCIter it = children.begin();
          it != children.end();
          ++it ) {
        Point childSize = it->first->getPreferredSize();

        size.m_x = std::max(size.m_x, childSize.m_x);
        size.m_y += childSize.m_y;
    }

    /* todo: border size hack */
    //size.m_x += 8;
    //size.m_y += 8;

    return size;
}

void Menu::doLayout( const Point& menuSize ) {
    Point position;
    const Widget::ChildVector& children = m_window->getContainer()->getChildren();

    for (Widget::ChildVectorCIter it = children.begin();
         it != children.end();
         ++it) {
        Widget* child = it->first;
        Point childSize = child->getPreferredSize();

        childSize.m_x = menuSize.m_x;
        child->setPosition(position);
        child->setSize(childSize);

        position.m_y += childSize.m_y;
    }
}

int Menu::showSubMenu( MenuItem* item, Menu* subMenu ) {
    Point position = m_window->getPosition();
    position += item->getPosition();
    position += Point(item->getSize().m_x,0);

    subMenu->setParentLevel(this);
    subMenu->show(position);
    subMenu->decRef();

    return 0;
}

int Menu::hideSubMenu( MenuItem* item, Menu* subMenu ) {
    subMenu->setParentLevel(this);
    subMenu->hide();
    subMenu->decRef();

    return 0;
}

} /* namespace yguipp */
