/* IRC client
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

#ifndef _CORE_EVENT_H_
#define _CORE_EVENT_H_

enum {
    EVENT_READ = 0,
    EVENT_WRITE,
    EVENT_EXCEPT,
    EVENT_COUNT
};

struct event;

typedef int event_callback_t( struct event* event );

typedef struct event_type {
    int interested;
    event_callback_t* callback;
} event_type_t;

typedef struct event {
    int fd;
    event_type_t events[ EVENT_COUNT ];
} event_t;

#endif /* _CORE_EVENT_H_ */
