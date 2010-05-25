/* GUI server
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

#include <guiserver/graphicsdriver.hpp>

#include "vesa.hpp"

VesaDriver::VesaDriver( void ) : m_device(-1), m_fbAddress(NULL), m_fbRegion(-1) {
}

VesaDriver::~VesaDriver( void ) {
    if ( m_device >= 0 ) {
        close(m_device);
    }
}

std::string VesaDriver::getName( void ) {
    return "VESA";
}

bool VesaDriver::detect( void ) {
    uint16_t idTable[128];
    vesa_cmd_modelist_t modeListCmd;

    m_device = open( "/device/video/vesa", O_RDONLY );

    if ( m_device < 0 ) {
        return false;
    }

    modeListCmd.max_count = sizeof(idTable);
    modeListCmd.mode_list = idTable;

    if ( ioctl( m_device, IOCTL_VESA_GET_MODE_LIST, &modeListCmd ) != 0 ) {
        return false;
    }

    if ( modeListCmd.current_count == 0 ) {
        return false;
    }

    for ( uint32_t i = 0; i < modeListCmd.current_count; i++ ) {
        vesa_cmd_modeinfo_t modeInfoCmd;
        vesa_mode_info_t* modeInfo;

        modeInfoCmd.mode_number = idTable[i];
        modeInfo = &modeInfoCmd.mode_info;

        if ( ioctl( m_device, IOCTL_VESA_GET_MODE_INFO, &modeInfoCmd ) != 0 ) {
            continue;
        }

        if ( ( modeInfo->phys_base_ptr == 0 ) ||
             ( modeInfo->num_planes != 1 ) ||
             ( ( modeInfo->bits_per_pixel != 16 ) &&
               ( modeInfo->bits_per_pixel != 24 ) &&
               ( modeInfo->bits_per_pixel != 32 ) ) ||
             ( modeInfo->width < 640 ) ||
             ( modeInfo->height < 480 ) ) {
            continue;
        }

        m_screenModes.push_back(
            new VesaScreenMode(
                modeInfo->width, modeInfo->height, bpp_to_colorspace(modeInfo->bits_per_pixel),
                idTable[i], reinterpret_cast<void*>(modeInfo->phys_base_ptr)
            )
        );
    }

    return true;
}

size_t VesaDriver::getModeCount( void ) {
    return m_screenModes.size();
}

ScreenMode* VesaDriver::getModeInfo( size_t index ) {
    return m_screenModes[index];
}

bool VesaDriver::setMode( ScreenMode* info ) {
    bool found = false;
    VesaScreenMode* vesaMode;
    vesa_cmd_setmode_t setModeCmd;

    for ( std::vector<VesaScreenMode*>::const_iterator it = m_screenModes.begin();
          it != m_screenModes.end();
          ++it ) {
        vesaMode = *it;

        if ( *vesaMode == *info ) {
            found = true;
            break;
        }
    }

    if ( !found ) {
        return false;
    }

    setModeCmd.mode_number = vesaMode->m_modeId | ( 1 << 14 );

    if ( ioctl( m_device, IOCTL_VESA_SET_MODE, &setModeCmd ) != 0 ) {
        return false;
    }

    if ( m_fbRegion != -1 ) {
        memory_region_delete( m_fbRegion );
    }

    m_fbRegion = memory_region_create(
        "vesa_fb",
        PAGE_ALIGN( vesaMode->m_width * vesaMode->m_height * colorspace_to_bpp(vesaMode->m_colorSpace) ),
        REGION_READ | REGION_WRITE,
        &m_fbAddress
    );

    if ( m_fbRegion < 0 ) {
        return false;
    }

    if ( memory_region_remap_pages( m_fbRegion, vesaMode->m_frameBufferBase ) != 0 ) {
        return false;
    }

    return true;
}

void* VesaDriver::getFrameBuffer( void ) {
    return m_fbAddress;
}
