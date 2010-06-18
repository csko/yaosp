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

#include <ygui++/menu.hpp>
#include <ygui++/menuitem.hpp>

namespace yguipp {

MenuBar::MenuBar( void ) : m_subMenuActive(false), m_activeItem(NULL) {
}

MenuBar::~MenuBar( void ) {
}

void MenuBar::add( Widget* child, layout::LayoutData* data ) {
    MenuItem* item;

    item = dynamic_cast<MenuItem*>(child);

    if (item == NULL) {
        return;
    }

    Widget::add(child);
    item->setMenuParent(this);
}

Point MenuBar::getPreferredSize( void ) {
    Point size;

    for ( ChildVectorCIter it = m_children.begin();
          it != m_children.end();
          ++it ) {
        Widget* child = it->first;
        Point childSize = child->getPreferredSize();

        size.m_x += childSize.m_x;
        size.m_y = std::max(size.m_y, childSize.m_y);
    }

    return size;
}

int MenuBar::validate( void ) {
    Point position;

    for ( ChildVectorCIter it = m_children.begin();
          it != m_children.end();
          ++it ) {
        Widget* child = it->first;
        Point childSize = child->getPreferredSize();

        child->setPosition(position);
        child->setSize(childSize);

        position.m_x += childSize.m_x;
    }

    return 0;
}

int MenuBar::paint( GraphicsContext* g ) {
    return 0;
}

int MenuBar::itemActivated( MenuItem* item ) {
    if (m_subMenuActive) {
        Menu* subMenu;

        assert(m_activeItem != NULL);

        if (m_activeItem != item) {
            subMenu = m_activeItem->getSubMenu();
            if (subMenu != NULL) {
                hideSubMenu(m_activeItem, subMenu);
            }

            m_activeItem->setActive(false);
            m_activeItem = item;
            m_activeItem->setActive(true);

            subMenu = m_activeItem->getSubMenu();
            if (subMenu != NULL) {
                showSubMenu(m_activeItem, subMenu);
            }
        }
    } else {
        assert(m_activeItem == NULL);
        m_activeItem = item;
        m_activeItem->setActive(true);
    }

    return 0;
}

int MenuBar::itemDeActivated( MenuItem* item ) {
    if (!m_subMenuActive) {
        assert(m_activeItem == item);
        m_activeItem->setActive(false);
        m_activeItem = NULL;
    }

    return 0;
}

int MenuBar::itemPressed( MenuItem* item ) {
    assert(m_activeItem != NULL && m_activeItem == item);

    Menu* subMenu = item->getSubMenu();

    if (subMenu != NULL) {
        showSubMenu(item, subMenu);
        m_subMenuActive = true;
    }

    return 0;
}

int MenuBar::hideAllLevel( void ) {
    Menu* subMenu;

    assert(m_parentLevel == NULL);
    assert(m_activeItem != NULL);

    subMenu = m_activeItem->getSubMenu();
    if (subMenu != NULL) {
        assert(m_subMenuActive);
        hideSubMenu(m_activeItem, subMenu);
        m_subMenuActive = false;
    } else {
        assert(!m_subMenuActive);
    }

    m_activeItem->setActive(false);
    m_activeItem = NULL;

    return 0;
}

int MenuBar::showSubMenu( MenuItem* item, Menu* subMenu ) {
    Point position = getWindow()->getPosition();
    position += Point(3,21); // todo: window decorator size
    position += getPosition();
    position += item->getPosition();
    position += Point(0,getSize().m_y);

    subMenu->setParentLevel(this);
    subMenu->show(position);
    subMenu->decRef();

    return 0;
}

int MenuBar::hideSubMenu( MenuItem* item, Menu* subMenu ) {
    subMenu->hide();
    subMenu->decRef();

    return 0;
}

} /* namespace yguipp */
