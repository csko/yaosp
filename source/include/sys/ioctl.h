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

/* PS/2 driver ioctls */

#define IOCTL_PS2KBD_TOGGLE_LEDS 0x00000001

/* RAMDisk driver ioctls */

#define IOCTL_RAMDISK_CREATE     0x00000100
#define IOCTL_RAMDISK_DELETE     0x00000101
#define IOCTL_RAMDISK_GET_COUNT  0x00000102
#define IOCTL_RAMDISK_GET_LIST   0x00000103

int ioctl( int fd, int request, ... );

#endif // _SYS_IOCTL_H_
