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
    virtual ~MenuItemParent( void ) {}

    virtual int itemActivated( MenuItem* item ) = 0;
    virtual int itemDeActivated( MenuItem* item ) = 0;
}; /* class MenuItemParent */

class MenuBar : public Widget, public MenuItemParent {
  public:
    MenuBar( void );
    virtual ~MenuBar( void );

    void addChild( Widget* child, layout::LayoutData* data = NULL );

    Point getPreferredSize( void );

    int validate( void );
    int paint( GraphicsContext* g );

    int itemActivated( MenuItem* item );
    int itemDeActivated( MenuItem* item );

  private:
    MenuItem* m_activeItem;
}; /* class MenuBar */

class Menu : public Object, public MenuItemParent {
  public:
    Menu( void );
    ~Menu( void );

    void addItem( MenuItem* item );

    void show( const Point& p );

    int itemActivated( MenuItem* item );
    int itemDeActivated( MenuItem* item );

  private:
    Point getPreferredSize( void );
    void doLayout( void );

  private:
    Window* m_window;
    MenuItem* m_activeItem;
}; /* class Menu */

} /* namespace yguipp */

#endif /* _MENU_HPP_ */
