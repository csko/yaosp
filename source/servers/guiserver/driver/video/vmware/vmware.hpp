/* VMware graphics driver
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

#include <yaosp/region.h>

#include <guiserver/graphicsdriver.hpp>

enum VMwareIoctls {
    VMWARE_GET_IO_BASE = 0x00100000
}; /* enum VMwareIoctls */

class VMwareDriver : public GraphicsDriver {
  public:
    VMwareDriver(void);
    virtual ~VMwareDriver(void);

    bool detect(void);

    std::string getName(void);
    size_t getModeCount(void);
    ScreenMode* getModeInfo(size_t index);
    void* getFrameBuffer(void);

    bool setMode(ScreenMode* info);

  private:
    bool initFrameBuffer(void);
    bool initFifo(void);

    uint32_t readReg(uint32_t index);
    void writeReg(uint32_t index, uint32_t value);

  private:
    int m_device;

    int m_indexPort;
    int m_valuePort;

    uint32_t* m_mmioBase;
    uint32_t m_mmioSize;
    region_id m_mmioRegion;

    static const int SVGA_INDEX_PORT = 0;
    static const int SVGA_VALUE_PORT = 1;

    static const uint32_t SVGA_REG_FB_START = 13;
    static const uint32_t SVGA_REG_FB_OFFSET = 14;
    static const uint32_t SVGA_REG_VRAM_SIZE = 15;
    static const uint32_t SVGA_REG_FB_SIZE = 16;
    static const uint32_t SVGA_REG_MEM_START = 18;
    static const uint32_t SVGA_REG_MEM_SIZE = 19;
    static const uint32_t SVGA_REG_CONFIG_DONE = 20;

    static const uint32_t SVGA_FIFO_MIN = 0;
    static const uint32_t SVGA_FIFO_MAX = 1;
    static const uint32_t SVGA_FIFO_NEXT_CMD = 2;
    static const uint32_t SVGA_FIFO_STOP = 3;
}; /* class VMwareDriver */

#endif /* _VMWARE_HPP_ */
