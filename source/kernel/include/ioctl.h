/* ioctl constant values
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

#ifndef _IOCTL_H_
#define _IOCTL_H_

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

/* Network ioctls */

#define IOCTL_NET_SET_IN_QUEUE    0x00000300
#define IOCTL_NET_START           0x00000301
#define IOCTL_NET_GET_HW_ADDRESS  0x00000302

/* Terminal control ioctls */

#define IOCTL_TERM_SET_ACTIVE     0x00000400

/* Disk ioctls */

#define IOCTL_DISK_GET_GEOMETRY   0x00000500

/* Input device ioctls */

#define IOCTL_INPUT_CREATE_DEVICE 0x00000600

#endif /* _IOCTL_H_ */
