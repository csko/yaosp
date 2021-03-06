/* Terminal driver
 *
 * Copyright (c) 2009 Zoltan Kovacs, Kornel Csernai
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

#ifndef _TERMINAL_PTY_H_
#define _TERMINAL_PTY_H_

#include <types.h>
#include <lock/mutex.h>
#include <lock/condition.h>
#include <vfs/inode.h>
#include <vfs/vfs.h>
#include <lib/hashtable.h>
#include <time.h>

#include "termios.h"

#define PTY_ROOT_INODE 0

typedef struct pty_node {
    hashitem_t hash;

    char* name;
    ino_t inode_number;
    mode_t mode;
    time_t atime;
    time_t mtime;
    time_t ctime;

    lock_id lock;
    lock_id read_queue;
    lock_id write_queue;

    struct pty_node* partner;

    uint8_t* buffer;
    size_t buffer_size;

    size_t size;
    size_t read_position;
    size_t write_position;

    select_request_t* read_requests;
    select_request_t* write_requests;

    int line_size;

    struct termios* term_info;
    struct winsize* window_size;
} pty_node_t;

typedef struct pty_lookup_data {
    char* name;
    int length;
    ino_t* inode_number;
} pty_lookup_data_t;

typedef struct pty_read_dir_data {
    int current;
    int required;
    dirent_t* entry;
} pty_read_dir_data_t;

typedef struct pty_dir_cookie {
    int current;
} pty_dir_cookie_t;

int init_pty_filesystem( void );

#endif /* _TERMINAL_PTY_H_ */
