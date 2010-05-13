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

#ifndef _FONT_HPP_
#define _FONT_HPP_

#include <string>

#include <ygui++/object.hpp>

namespace yguipp {

class Font : public Object {
  public:
    Font( const std::string& name, const std::string& style, int size );
    virtual ~Font( void );

    bool init( void );

    int getHandle( void );

    int getHeight( void );
    int getAscender( void );
    int getDescender( void );
    int getLineGap( void );

    int getStringWidth( const std::string& text );

  private:
    int m_handle;
    int m_ascender;
    int m_descender;
    int m_lineGap;

    std::string m_name;
    std::string m_style;
    int m_size;
}; /* class Font */

} /* namespace yguipp */

#endif /* _FONT_HPP_ */
