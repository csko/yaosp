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

#ifndef _MENU_HPP_
#define _MENU_HPP_

#include <ygui++/widget.hpp>
#include <ygui++/window.hpp>

namespace yguipp {

class MenuItem;

class MenuItemParent {
  public:
    MenuItemParent( void );
    virtual ~MenuItemParent( void ) {}

    int setParentLevel(MenuItemParent* parentLevel);

    virtual int itemActivated( MenuItem* item ) = 0;
    virtual int itemDeActivated( MenuItem* item ) = 0;
    virtual int itemPressed( MenuItem* item ) = 0;
    virtual int hideAllLevel( void ) = 0;

  protected:
    MenuItemParent* m_parentLevel;
}; /* class MenuItemParent */

class Menu;

class MenuBar : public Widget, public MenuItemParent {
  public:
    MenuBar( void );
    virtual ~MenuBar( void );

    void add( Widget* child, layout::LayoutData* data = NULL );

    Point getPreferredSize( void );

    int validate( void );
    int paint( GraphicsContext* g );

    int itemActivated( MenuItem* item );
    int itemDeActivated( MenuItem* item );
    int itemPressed( MenuItem* item );
    int hideAllLevel( void );

  private:
    int showSubMenu( MenuItem* item, Menu* subMenu );
    int hideSubMenu( MenuItem* item, Menu* subMenu );

  private:
    bool m_mouseOnItem;
    bool m_subMenuActive;
    MenuItem* m_activeItem;
}; /* class MenuBar */

class Menu : public Object, public MenuItemParent, public WindowListener {
  public:
    Menu( void );
    ~Menu( void );

    void add( MenuItem* item );

    Point getPreferredSize( void );

    void show( const Point& p );
    void hide( void );

    int itemActivated( MenuItem* item );
    int itemDeActivated( MenuItem* item );
    int itemPressed( MenuItem* item );
    int hideAllLevel( void );

    int windowDeActivated( Window* window, DeActivationReason reason );

  private:
    void doLayout( const Point& menuSize );

    int showSubMenu( MenuItem* item, Menu* subMenu );
    int hideSubMenu( MenuItem* item, Menu* subMenu );

  private:
    Window* m_window;
    MenuItem* m_activeItem;
}; /* class Menu */

} /* namespace yguipp */

#endif /* _MENU_HPP_ */
