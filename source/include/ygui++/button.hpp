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

#ifndef _BUTTON_HPP_
#define _BUTTON_HPP_

#include <string>

#include <ygui++/yconstants.hpp>
#include <ygui++/widget.hpp>
#include <ygui++/font.hpp>

namespace yguipp {

class Button : public Widget {
  public:
    Button( int hAlign = H_ALIGN_CENTER, int vAlign = V_ALIGN_CENTER );
    Button( const std::string& text, int hAlign = H_ALIGN_CENTER, int vAlign = V_ALIGN_CENTER );
    virtual ~Button( void );

  private:
    Button( const Button& b );
    Button& operator=( const Button& b );

  public:
    Point getPreferredSize( void );
    int paint( GraphicsContext* g );
    int mousePressed( const Point& p, int button );
    int mouseReleased( int button );

  private:
    void initFont( void );

  private:
    std::string m_text;
    Font* m_font;
    bool m_pressed;

    int m_hAlign;
    int m_vAlign;
}; /* class Button */

} /* namespace yguipp */

#endif /* _Button_HPP_ */
