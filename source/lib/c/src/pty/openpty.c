/* openpty function
 *
 * Copyright (c) 2010 Zoltan Kovacs
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

#include <pty.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>

int openpty(int* master, int* slave, char* name, struct termios* termp, struct winsize* winp) {
    int i = 0;
    int pty;
    char path[64];

    if ((master == NULL) ||
        (slave == NULL)) {
        errno = -EINVAL;
        return -1;
    }

    while (1) {
        struct stat st;

        snprintf(path, sizeof(path), "/device/terminal/pty%d", i);

        if (stat(path, &st) != 0) {
            break;
        }

        i++;
    }

    pty = open(path, O_RDWR | O_CREAT);

    if (pty < 0) {
        return -1;
    }

    snprintf(path, sizeof(path), "/device/terminal/tty%d", i);

    *master = pty;
    *slave = open(path, O_RDWR);

    return 0;
}
