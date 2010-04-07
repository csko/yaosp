/* yaosp C library
 *
 * Copyright (c) 2009, 2010 Zoltan Kovacs, Kornel Csernai
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

#ifndef _FCNTL_H_
#define _FCNTL_H_

#include <bits/fcntl.h>
#include <sys/cdefs.h>
#include <sys/types.h>

#define O_ACCMODE    0003
#define O_RDONLY       00
#define O_WRONLY       01
#define O_RDWR         02
#define O_CREAT      0100
#define O_EXCL       0200
#define O_NOCTTY     0400
#define O_TRUNC     01000
#define O_APPEND    02000
#define O_NONBLOCK  04000
#define O_NDELAY   O_NONBLOCK
#define O_SYNC     010000
#define O_FSYNC    O_SYNC
#define O_ASYNC    020000

#define F_DUPFD 0
#define F_GETFD 1
#define F_SETFD 2
#define F_GETFL 3
#define F_SETFL 4

#define FD_CLOEXEC 1

#define F_ULOCK 0  /* Unlock a previously locked region.  */
#define F_LOCK  1  /* Lock a region for exclusive use.  */
#define F_TLOCK 2  /* Test and lock a region for exclusive use.  */
#define F_TEST  3  /* Test a region for other processes locks.  */


int open( const char* filename, int flags, ... ) __nonnull((1));
int creat( const char* pathname, mode_t mode ) __nonnull((1));
int fcntl( int fd, int cmd, ... );

#endif /* _FCNTL_H_ */
