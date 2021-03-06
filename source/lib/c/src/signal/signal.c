/* signal function
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

#include <signal.h>
#include <errno.h>

sighandler_t signal( int signum, sighandler_t handler ) {
    int error;
    struct sigaction act;
    struct sigaction oldact;

    if ( ( handler == SIG_ERR ) ||
         ( signum < 0 ) ||
         ( signum >= _NSIG ) ) {
        errno = -EINVAL;
        return SIG_ERR;
    }

    act.sa_handler = handler;

    error = sigemptyset( &act.sa_mask );

    if ( error < 0 ) {
        return SIG_ERR;
    }

    error = sigaddset( &act.sa_mask, signum );

    if ( error < 0 ) {
        return SIG_ERR;
    }

    act.sa_flags = SA_RESTART;

    if ( sigaction( signum, &act, &oldact ) < 0 ) {
        return SIG_ERR;
    }

    return oldact.sa_handler;
}
