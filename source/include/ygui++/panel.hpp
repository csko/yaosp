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

#ifndef _PANEL_HPP_
#define _PANEL_HPP_

#include <ygui++/color.hpp>
#include <ygui++/widget.hpp>
#include <ygui++/layout/layout.hpp>

namespace yguipp {

class Panel : public Widget {
  public:
    Panel( void );
    virtual ~Panel( void );

    void setLayout( layout::Layout* layout );
    void setBackgroundColor( const Color& c );

    int validate( void );
    int paint( GraphicsContext* g );

  private:
    layout::Layout* m_layout;
    Color m_backgroundColor;
}; /* class Panel */

} /* namespace yguipp */

#endif /* _PANEL_HPP_ */
