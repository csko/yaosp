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

#ifndef _YGUIPP_TABPANEL_HPP_
#define _YGUIPP_TABPANEL_HPP_

#include <vector>

#include <ygui++/widget.hpp>
#include <ygui++/font.hpp>

namespace yguipp {

class TabPanel : public Widget {
  public:
    TabPanel(void);

    int addTab(const std::string& title, Widget* component);

    int validate(void);
    int paint(GraphicsContext* g);

  private:
    Font* m_font;

    std::vector< std::pair<std::string, Widget*> > m_tabs;
}; /* class TabPanel */

} /* namespace yguipp */

#endif /* _YGUIPP_TABPANEL_HPP_ */
