/* yaosp C library
 *
 * Copyright (c) 2009 Zoltan Kovacs
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

#ifndef _SYS_IOCTL_H_
#define _SYS_IOCTL_H_

#include <sys/types.h>

#define SIOCGIFCONF    0x89300003
#define SIOCGIFCOUNT   0x89300004
#define SIOCGIFADDR    0x89300005
#define SIOCGIFNETMASK 0x89300006
#define SIOCGIFHWADDR  0x89300007

/* PS/2 driver ioctls */

#define IOCTL_PS2KBD_TOGGLE_LEDS  0x00000001

/* RAMDisk driver ioctls */

#define IOCTL_RAMDISK_CREATE      0x00000100
#define IOCTL_RAMDISK_DELETE      0x00000101
#define IOCTL_RAMDISK_GET_COUNT   0x00000102
#define IOCTL_RAMDISK_GET_LIST    0x00000103

/* VESA driver ioctls */

#define IOCTL_VESA_GET_MODE_LIST  0x00000200
#define IOCTL_VESA_GET_MODE_INFO  0x00000201
#define IOCTL_VESA_SET_MODE       0x00000202

/* Terminal control ioctls */

#define IOCTL_TERM_SET_ACTIVE     0x00000400

/* Disk ioctls */

#define IOCTL_DISK_GET_GEOMETRY   0x00000500

/* Input device ioctls */

#define IOCTL_INPUT_CREATE_DEVICE 0x00000600

typedef struct device_geometry {
    uint32_t bytes_per_sector;
    uint64_t sector_count;
} device_geometry_t;

int ioctl( int fd, int request, ... );

#endif /* _SYS_IOCTL_H_ */
