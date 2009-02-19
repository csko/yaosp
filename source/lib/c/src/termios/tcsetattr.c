/* tcsetattr function
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

#include <termios.h>
#include <errno.h>
#include <stddef.h>
#include <sys/ioctl.h>

int tcsetattr( int fd, int optional_actions, const struct termios* tio ) {
    int error;

    if ( tio == NULL ) {
        errno = EINVAL;
        return -1;
    }

    switch ( optional_actions ) {
        case TCSANOW :
            error = ioctl( fd, TCSETA, tio );
            break;

        case TCSADRAIN :
            error = ioctl( fd, TCSETAW, tio );
            break;

        case TCSAFLUSH :
            error = ioctl( fd, TCSETAF, tio );
            break;

        default :
            error = -EINVAL;
            break;
    }

    if ( error < 0 ) {
        errno = -error;
        return -1;
    }

    return 0;
}
