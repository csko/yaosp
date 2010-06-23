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

#include "menuitem.hpp"

TBMenuItemInfo::TBMenuItemInfo(uint64_t position, const std::string& name, const std::string& executable) :
    m_position(position), m_name(name), m_executable(executable) {
}

bool TBMenuItemInfo::operator<(const TBMenuItemInfo& info) const {
    return (m_position < info.m_position);
}

TBMenuItem::TBMenuItem(const TBMenuItemInfo& itemInfo) : MenuItem(itemInfo.m_name), m_executable(itemInfo.m_executable) {
}

const std::string& TBMenuItem::getExecutable(void) {
    return m_executable;
}
