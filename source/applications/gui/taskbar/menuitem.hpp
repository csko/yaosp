/* Taskbar application
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

#ifndef _TASKBAR_MENU_HPP_
#define _TASKBAR_MENU_HPP_

#include <ygui++/menuitem.hpp>

struct TBMenuItemInfo {
    TBMenuItemInfo(uint64_t position, const std::string& name, const std::string& executable, yguipp::Bitmap* image);

    bool operator<(const TBMenuItemInfo& info) const;

    uint64_t m_position;
    std::string m_name;
    std::string m_executable;
    yguipp::Bitmap* m_image;
};

class TBMenuItem : public yguipp::MenuItem {
  public:
    TBMenuItem(const TBMenuItemInfo& itemInfo);

    const std::string& getExecutable(void);

  private:
    std::string m_executable;
}; /* class TBMenuItem */

#endif /* _TASKBAR_MENU_HPP_ */
