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

#ifndef _MENUITEM_HPP_
#define _MENUITEM_HPP_

#include <string>

#include <ygui++/widget.hpp>

namespace yguipp {

class MenuItemParent;
class Menu;

class MenuItem : public Widget {
  public:
    MenuItem( const std::string& text );
    virtual ~MenuItem( void );

    Point getPreferredSize( void );
    Menu* getSubMenu( void );

    int setActive( bool active );
    int setSubMenu( Menu* menu );
    int setMenuParent( MenuItemParent* parent );

    int mouseEntered( const Point& p );
    int mouseExited( void );
    int mousePressed( const Point& p );
    int paint( GraphicsContext* g );

  private:
    void initFont( void );

  private:
    Font* m_font;
    std::string m_text;

    Menu* m_subMenu;
    MenuItemParent* m_menuParent;

    bool m_active;
}; /* class MenuItem */

} /* namespace yguipp */

#endif /* _MENUITEM_HPP_ */
