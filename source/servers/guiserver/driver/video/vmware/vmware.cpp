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
#include <yaosp/debug.h>
#include <yaosp/io.h>

#include "vmware.hpp"

VMwareDriver::VMwareDriver(void) : m_device(-1), m_indexPort(-1), m_valuePort(-1), m_mmioBase(NULL),
                                   m_mmioSize(0), m_mmioRegion(-1) {
}

VMwareDriver::~VMwareDriver(void) {
    if (m_device >= 0) {
        close(m_device);
    }
}

bool VMwareDriver::detect(void) {
    int ioBase;

    m_device = open("/device/video/vmware0", O_RDONLY);

    if (m_device < 0) {
        return false;
    }

    if (ioctl(m_device, VMWARE_GET_IO_BASE, &ioBase) != 0) {
        return false;
    }

    dbprintf("VMware I/O base: %04x\n", ioBase);

    m_indexPort = ioBase + SVGA_INDEX_PORT;
    m_valuePort = ioBase + SVGA_VALUE_PORT;

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
    return NULL;
}

bool VMwareDriver::setMode(ScreenMode* info) {
    return true;
}

bool VMwareDriver::initFrameBuffer(void) {
    uint32_t fbStart;
    uint32_t vramSize;

    fbStart = readReg(SVGA_REG_FB_START);
    vramSize = readReg(SVGA_REG_VRAM_SIZE);

    dbprintf("VMware VRAM at %x size %x\n", fbStart, vramSize);

    return true;
}

bool VMwareDriver::initFifo(void) {
    uint32_t mmioStart;

    mmioStart = readReg(SVGA_REG_MEM_START);
    m_mmioSize = readReg(SVGA_REG_MEM_SIZE);

    dbprintf("VMware MMIO at %x size %x\n", mmioStart, m_mmioSize);

    m_mmioRegion = memory_region_create("VMware MMIO", m_mmioSize, REGION_READ | REGION_WRITE, reinterpret_cast<void**>(&m_mmioBase));

    if (m_mmioRegion < 0) {
        return false;
    }

    if (memory_region_remap_pages(m_mmioRegion, reinterpret_cast<void*>(mmioStart)) != 0) {
        memory_region_delete(m_mmioRegion);
        m_mmioRegion = -1;
        m_mmioBase = NULL;
        return false;
    }

    m_mmioBase[SVGA_FIFO_MIN] = 4 * sizeof(uint32_t);
    m_mmioBase[SVGA_FIFO_MAX] = m_mmioSize & ~3;
    m_mmioBase[SVGA_FIFO_NEXT_CMD] = 4 * sizeof(uint32_t);
    m_mmioBase[SVGA_FIFO_STOP] = 4 * sizeof(uint32_t);

    writeReg(SVGA_REG_CONFIG_DONE, 1);
    readReg(SVGA_REG_CONFIG_DONE);

    return true;
}

uint32_t VMwareDriver::readReg(uint32_t index) {
    outl(index, m_indexPort);
    return inl(m_valuePort);
}

void VMwareDriver::writeReg(uint32_t index, uint32_t value) {
    outl(index, m_indexPort);
    outl(value, m_valuePort);
}
