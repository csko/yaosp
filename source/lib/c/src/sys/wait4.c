/* wait4 function
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

#include <errno.h>
#include <sys/wait.h>

#include <yaosp/syscall.h>
#include <yaosp/syscall_table.h>

pid_t wait4( pid_t pid, int* status, int options, struct rusage* rusage ) {
    int error;

    error = syscall4(
        SYS_wait4,
        pid,
        ( int )status,
        options,
        ( int )rusage
    );

    if ( error < 0 ) {
        errno = -error;
        return -1;
    }

    return error;
}
