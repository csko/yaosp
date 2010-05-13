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

#ifndef _LABEL_HPP_
#define _LABEL_HPP_

#include <string>
#include <ygui/yconstants.h>

#include <ygui++/widget.hpp>
#include <ygui++/font.hpp>

namespace yguipp {

class Label : public Widget {
  public:
    Label( int hAlign = H_ALIGN_CENTER, int vAlign = V_ALIGN_CENTER );
    Label( const std::string& text, int hAlign = H_ALIGN_CENTER, int vAlign = V_ALIGN_CENTER );
    virtual ~Label( void );

  private:
    Label( const Label& l );
    Label& operator=( const Label& l );

  public:
    int paint( GraphicsContext* g );

  private:
    void initFont( void );

  private:
    std::string m_text;
    Font* m_font;

    int m_hAlign;
    int m_vAlign;
}; /* class Label */

} /* namespace yguipp */

#endif /* _LABEL_HPP_ */
