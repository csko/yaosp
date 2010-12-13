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

#ifndef _YGUIPP_FONT_HPP_
#define _YGUIPP_FONT_HPP_

#include <string>

#include <ygui++/object.hpp>
#include <ygui++/yconstants.hpp>

namespace yguipp {

class Font : public Object {
  public:
    Font(const std::string& name, const std::string& style, const FontInfo& fontInfo);
    virtual ~Font(void);

  private:
    Font(const Font& f);
    Font& operator=(const Font& f);

  public:
    bool init(void);

    int getHandle(void);
    int getAscender(void);
    int getDescender(void);
    int getHeight(void);
    int getWidth(const std::string& text);

  private:
    int getWidthFromServer(const std::string& text);

  private:
    int m_handle;
    int m_ascender;
    int m_descender;
    int m_height;

    std::string m_name;
    std::string m_style;
    FontInfo m_fontInfo;
}; /* class Font */

} /* namespace yguipp */

#endif /* _YGUIPP_FONT_HPP_ */
