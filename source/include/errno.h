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

#ifndef _ERRNO_H_
#define _ERRNO_H_

#define ENOMEM       1
#define EINVAL       2
#define EIO          3
#define ETIME        4
#define ENOSYS       5
#define ENOENT       6
#define EEXIST       7
#define EBUSY        8
#define EISDIR       9
#define ENOINO       10
#define ENOEXEC      11
#define EBADF        12
#define EHW          13
#define ERANGE       14
#define ENXIO        15
#define EDOM         16
#define ENODEV       17
#define EINTR        18
#define ENOTTY       19
#define EPERM        20
#define EROFS        21
#define ELOOP        22
#define ENOTDIR      23
#define ENOTEMPTY    24
#define EAGAIN       25
#define E2BIG        26
#define ETIMEDOUT    27
#define EOVERFLOW    28
#define ENOSPC       29
#define ECHILD       30
#define ENAMETOOLONG 31
#define ESPIPE       32
#define EACCES       33

extern int errno;

#endif /* _ERRNO_H_ */
