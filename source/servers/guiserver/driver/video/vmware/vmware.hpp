/* VMWare graphics driver
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

#ifndef _VMWARE_HPP_
#define _VMWARE_HPP_

#include <guiserver/graphicsdriver.hpp>

class VMWareDriver : public GraphicsDriver {
  public:
    VMWareDriver(void);
    virtual ~VMWareDriver(void);

    bool detect(void);

    std::string getName(void);
    size_t getModeCount(void);
    ScreenMode* getModeInfo(size_t index);
    void* getFrameBuffer(void);

    bool setMode(ScreenMode* info);
}; /* class VMWareDriver */

#endif /* _VMWARE_HPP_ */
