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

#ifndef _SYS_WAIT_H_
#define _SYS_WAIT_H_

#include <sys/types.h>
#include <sys/resource.h>

#define WNOHANG   1
#define WUNTRACED 2

pid_t wait( int* status );
pid_t waitpid( pid_t pid, int* status, int options );
pid_t wait3( int* status, int options, struct rusage* rusage );
pid_t wait4( pid_t pid, int* status, int options, struct rusage* rusage );

#endif /* _SYS_WAIT_H_ */
