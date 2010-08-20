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

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <yaosp/debug.h>
#include <yaosp/io.h>

#include "vmware.hpp"

VMwareDriver::VMwareDriver(void) : m_device(-1), m_indexPort(-1), m_valuePort(-1), m_capabilities(0),
                                   m_fbBase(NULL), m_fbOffset(0), m_fbSize(0), m_fbRegion(-1),
                                   m_mmioBase(NULL), m_mmioSize(0), m_mmioRegion(-1) {
}

VMwareDriver::~VMwareDriver(void) {
    if (m_device >= 0) {
        close(m_device);
    }
}

bool VMwareDriver::detect(void) {
    struct stat st;
    return (stat("/device/video/vmware", &st) == 0);
}

bool VMwareDriver::initialize(void) {
    int ioBase;

    m_device = open("/device/video/vmware", O_RDONLY);

    if (m_device < 0) {
        return false;
    }

    if (ioctl(m_device, VMWARE_GET_IO_BASE, &ioBase) != 0) {
        return false;
    }

    dbprintf("VMware I/O base: %04x\n", ioBase);

    m_indexPort = ioBase + SVGA_INDEX_PORT;
    m_valuePort = ioBase + SVGA_VALUE_PORT;

    m_capabilities = readRegister(SVGA_REG_CAPABILITIES);

    if (!initFrameBuffer()) {
        return false;
    }

    if (!initFifo()) {
        return false;
    }

    return true;
}

std::string VMwareDriver::getName(void) {
    return "VMware";
}

size_t VMwareDriver::getModeCount(void) {
    return 0;
}

ScreenMode* VMwareDriver::getModeInfo(size_t index) {
    return NULL;
}

void* VMwareDriver::getFrameBuffer(void) {
    return reinterpret_cast<void*>(m_fbBase + m_fbOffset);
}

bool VMwareDriver::setMode(ScreenMode* info) {
    writeRegister(SVGA_REG_ENABLE, 0);
    writeRegister(SVGA_REG_WIDTH, info->m_width);
    writeRegister(SVGA_REG_HEIGHT, info->m_height);
    writeRegister(SVGA_REG_BITS_PER_PIXEL, yguipp::colorspace_to_bpp(info->m_colorSpace) * 8);
    writeRegister(SVGA_REG_GUEST_ID, GUEST_OS_OTHER);
    writeRegister(SVGA_REG_ENABLE, 1);

    m_fbRect = yguipp::Rect(0, 0, info->m_width - 1, info->m_height - 1);
    m_fbOffset = readRegister(SVGA_REG_FB_OFFSET);

    return true;
}

int VMwareDriver::drawRect(Bitmap* bitmap, const yguipp::Rect& clipRect, const yguipp::Rect& rect,
                           const yguipp::Color& color, yguipp::DrawingMode mode) {
    GraphicsDriver::drawRect(bitmap, clipRect, rect, color, mode);

    if (bitmap->hasFlag(Bitmap::SCREEN)) {
        updateRect(rect);
    }

    return 0;
}

int VMwareDriver::fillRect( Bitmap* bitmap, const yguipp::Rect& clipRect, const yguipp::Rect& rect,
                            const yguipp::Color& color, yguipp::DrawingMode mode ) {
    GraphicsDriver::fillRect(bitmap, clipRect, rect, color, mode);

    if (bitmap->hasFlag(Bitmap::SCREEN)) {
        updateRect(rect);
    }

    return 0;
}

int VMwareDriver::blitBitmap(Bitmap* dest, const yguipp::Point& point, Bitmap* src,
                             const yguipp::Rect& rect, yguipp::DrawingMode mode) {
    GraphicsDriver::blitBitmap(dest, point, src, rect, mode);

    if (dest->hasFlag(Bitmap::SCREEN)) {
        updateRect(yguipp::Rect(point.m_x, point.m_y, point.m_x + rect.width() - 1, point.m_y + rect.height() - 1));
    }

    return 0;
}

void VMwareDriver::updateRect(yguipp::Rect rect) {
    rect &= m_fbRect;

    if (!rect.isValid()) {
        return;
    }

    writeRegister(SVGA_REG_CONFIG_DONE, 0);
    writeFifo(SVGA_CMD_UPDATE);
    writeFifo(rect.m_left);
    writeFifo(rect.m_top);
    writeFifo(rect.width());
    writeFifo(rect.height());
    writeRegister(SVGA_REG_CONFIG_DONE, 1);
}

bool VMwareDriver::initFrameBuffer(void) {
    uint32_t fbStart;

    fbStart = readRegister(SVGA_REG_FB_START);
    m_fbSize = readRegister(SVGA_REG_VRAM_SIZE);

    dbprintf("VMware VRAM at %x size %x\n", fbStart, m_fbSize);

    m_fbRegion = memory_region_create("VMware FB", PAGE_ALIGN(m_fbSize), REGION_READ | REGION_WRITE, reinterpret_cast<void**>(&m_fbBase));

    if (m_fbRegion < 0) {
        return false;
    }

    if (memory_region_remap_pages(m_fbRegion, reinterpret_cast<void*>(fbStart)) != 0) {
        memory_region_delete(m_fbRegion);
        m_fbRegion = -1;
        m_fbBase = NULL;
        return false;
    }

    return true;
}

bool VMwareDriver::initFifo(void) {
    uint32_t mmioStart;

    mmioStart = readRegister(SVGA_REG_MEM_START);
    m_mmioSize = readRegister(SVGA_REG_MEM_SIZE);

    dbprintf("VMware MMIO at %x size %x\n", mmioStart, m_mmioSize);

    m_mmioRegion = memory_region_create("VMware MMIO", PAGE_ALIGN(m_mmioSize), REGION_READ | REGION_WRITE, reinterpret_cast<void**>(&m_mmioBase));

    if (m_mmioRegion < 0) {
        return false;
    }

    if (memory_region_remap_pages(m_mmioRegion, reinterpret_cast<void*>(mmioStart)) != 0) {
        memory_region_delete(m_mmioRegion);
        m_mmioRegion = -1;
        m_mmioBase = NULL;
        return false;
    }

    if (m_capabilities & SVGA_CAP_EXTENDED_FIFO) {
        // todo
    } else {
        m_mmioBase[SVGA_FIFO_MIN] = 16;
        m_mmioBase[SVGA_FIFO_MAX] = m_mmioSize & ~3;
        m_mmioBase[SVGA_FIFO_NEXT_CMD] = 16;
        m_mmioBase[SVGA_FIFO_STOP] = 16;
    }

    writeRegister(SVGA_REG_CONFIG_DONE, 1);
    readRegister(SVGA_REG_CONFIG_DONE);

    return true;
}

void VMwareDriver::syncFifo(void) {
    writeRegister(SVGA_REG_SYNC, 1);

    while (readRegister(SVGA_REG_BUSY) != 0) {
        /* do nothing ... */
    }
}

void VMwareDriver::writeFifo(uint32_t value) {
    if (((m_mmioBase[SVGA_FIFO_NEXT_CMD] + 4) == m_mmioBase[SVGA_FIFO_STOP]) ||
        (m_mmioBase[SVGA_FIFO_NEXT_CMD] == (m_mmioBase[SVGA_FIFO_MAX] - 4))) {
        syncFifo();
    }

    register uint32_t nextCmd = m_mmioBase[SVGA_FIFO_NEXT_CMD];
    m_mmioBase[nextCmd / 4] = value;

    if (nextCmd == (m_mmioBase[SVGA_FIFO_MAX] - 4)) {
        m_mmioBase[SVGA_FIFO_NEXT_CMD] = m_mmioBase[SVGA_FIFO_MIN];
    } else {
        m_mmioBase[SVGA_FIFO_NEXT_CMD] += 4;
    }
}

uint32_t VMwareDriver::readRegister(uint32_t index) {
    outl(index, m_indexPort);
    return inl(m_valuePort);
}

void VMwareDriver::writeRegister(uint32_t index, uint32_t value) {
    outl(index, m_indexPort);
    outl(value, m_valuePort);
}

extern "C" GraphicsDriver* get_graphics_driver(void) {
    return new VMwareDriver();
}
