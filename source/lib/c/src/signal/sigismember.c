/* sigismember function
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

int sigismember( const sigset_t* set, int signum ) {
    if ( ( set == NULL ) ||
         ( signum < 1 ) ||
         ( signum >= _NSIG ) ) {
        errno = -EINVAL;
        return -1;
    }

    if ( ( ( *set ) & ( 1ULL << ( signum - 1 ) ) ) != 0 ) {
        return 1;
    }

    return 0;
}
