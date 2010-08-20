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
    bool initialize(void);

    std::string getName(void);
    size_t getModeCount(void);
    ScreenMode* getModeInfo(size_t index);
    void* getFrameBuffer(void);

    bool setMode(ScreenMode* info);

    int drawRect(Bitmap* bitmap, const yguipp::Rect& clipRect, const yguipp::Rect& rect,
                 const yguipp::Color& color, yguipp::DrawingMode mode);
    int fillRect(Bitmap* bitmap, const yguipp::Rect& clipRect, const yguipp::Rect& rect,
                 const yguipp::Color& color, yguipp::DrawingMode mode);
    int blitBitmap(Bitmap* dest, const yguipp::Point& point, Bitmap* src,
                  const yguipp::Rect& rect, yguipp::DrawingMode mode);

  private:
    void updateRect(yguipp::Rect rect);

    bool initFrameBuffer(void);

    bool initFifo(void);
    void syncFifo(void);
    void writeFifo(uint32_t value);

    uint32_t readRegister(uint32_t index);
    void writeRegister(uint32_t index, uint32_t value);

  private:
    int m_device;

    int m_indexPort;
    int m_valuePort;

    uint32_t m_capabilities;

    uint8_t* m_fbBase;
    uint32_t m_fbOffset;
    uint32_t m_fbSize;
    region_id m_fbRegion;
    yguipp::Rect m_fbRect;

    uint32_t* m_mmioBase;
    uint32_t m_mmioSize;
    region_id m_mmioRegion;

    enum {
        SVGA_INDEX_PORT = 0,
        SVGA_VALUE_PORT = 1
    };

    enum {
        SVGA_REG_ID = 0,
        SVGA_REG_ENABLE = 1,
        SVGA_REG_WIDTH = 2,
        SVGA_REG_HEIGHT = 3,
        SVGA_REG_MAX_WIDTH = 4,
        SVGA_REG_MAX_HEIGHT = 5,
        SVGA_REG_DEPTH = 6,
        SVGA_REG_BITS_PER_PIXEL = 7,
        SVGA_REG_PSEUDOCOLOR = 8,
        SVGA_REG_RED_MASK = 9,
        SVGA_REG_GREEN_MASK = 10,
        SVGA_REG_BLUE_MASK = 11,
        SVGA_REG_BYTES_PER_LINE = 12,
        SVGA_REG_FB_START = 13,
        SVGA_REG_FB_OFFSET = 14,
        SVGA_REG_VRAM_SIZE = 15,
        SVGA_REG_FB_SIZE = 16,
        SVGA_REG_CAPABILITIES = 17,
        SVGA_REG_MEM_START = 18,
        SVGA_REG_MEM_SIZE = 19,
        SVGA_REG_CONFIG_DONE = 20,
        SVGA_REG_SYNC = 21,
        SVGA_REG_BUSY = 22,
        SVGA_REG_GUEST_ID = 23
    };

    enum {
        SVGA_FIFO_MIN = 0,
        SVGA_FIFO_MAX = 1,
        SVGA_FIFO_NEXT_CMD = 2,
        SVGA_FIFO_STOP = 3
    };

    enum {
        SVGA_CMD_INVALID = 0,
        SVGA_CMD_UPDATE = 1,
        SVGA_CMD_RECT_FILL = 2,
        SVGA_CMD_RECT_COPY = 3
    };

    enum {
        SVGA_CAP_EXTENDED_FIFO = 0x08000
    };

    static const uint32_t GUEST_OS_OTHER = 0x500A;
}; /* class VMwareDriver */

#endif /* _VMWARE_HPP_ */
