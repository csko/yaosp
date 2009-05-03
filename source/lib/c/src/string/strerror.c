/* strerror function
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

#include <string.h>

static char* error_strings[] = {
    "Cannot allocate memory", /* ENOMEM */
    "Invalid argument", /* EINVAL */
    "Input/output error", /* EIO */
    "Timer expired", /* ETIME */
    "Function not implemented", /* ENOSYS */
    "No such file or directory", /* ENOENT */
    "File exists", /* EEXIST */
    "Device or resource busy", /* EBUSY */
    "Is a directory", /* EISDIR */
    "Inode not found", /* ENOINO */
    "Executable format error", /* ENOEXEC */
    "Bad file descriptor", /* EBADF */
    "Hardware error", /* EHW */
    "Numerical result out of range", /* ERANGE */
    "No such device or address", /* ENXIO */
    "Numerical argument out of domain", /* EDOM */
    "No such device", /* ENODEV */
    "Interrupted system call", /* EINTR */
    "Inappropriate ioctl for device", /* ENOTTY */
    "Operation not permitted", /* EPERM */
    "Read-only file system", /* EROFS */
    "Too many levels of symbolic links", /* ELOOP */
    "Not a directory", /* ENOTDIR */
    "Directory not empty", /* ENOTEMPTY */
    "Resource temporarily unavailable", /* EAGAIN */
    "Argument list too long", /* E2BIG */
    "Connection timed out", /* ETIMEDOUT */
    "Value too large for defined data type", /* EOVERFLOW */
    "No space left on device", /* ENOSPC */
    "No child processes", /* ECHILD */
    "File name too long", /* ENAMETOOLONG */
    "Illegal seek", /* ESPIPE */
    "Permission denied" /* EACCES */
};

char* strerror( int errnum ) {
    if ( errnum < 0 ) {
        errnum = -errnum;
    }

    errnum--;

    if ( ( errnum < 0 ) ||
         ( errnum >= ( sizeof( error_strings ) / sizeof( error_strings[ 0 ] ) ) ) ) {
        return NULL;
    }

    return error_strings[ errnum ];
}
