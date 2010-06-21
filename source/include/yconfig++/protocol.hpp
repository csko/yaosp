/* yaosp configuration library
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

#ifndef _YCONFIG_PROTOCOL_H_
#define _YCONFIG_PROTOCOL_H_

#include <yaosp/ipc.h>

enum {
    MSG_GET_ATTRIBUTE_VALUE = 1,
    MSG_NODE_ADD_CHILD,
    MSG_NODE_DEL_CHILD,
    MSG_NODE_LIST_CHILDREN
};

typedef enum attr_type {
    NUMERIC,
    ASCII,
    BOOL,
    BINARY
} attr_type_t;

typedef struct msg_get_attr {
    ipc_port_id reply_port;
} msg_get_attr_t;

typedef struct msg_get_reply {
    int error;
    attr_type_t type;
} msg_get_reply_t;

typedef struct msg_add_child {
    ipc_port_id reply_port;
} msg_add_child_t;

typedef struct msg_add_child_reply {
    int error;
} msg_add_child_reply_t;

typedef struct msg_del_child {
    ipc_port_id reply_port;
} msg_del_child_t;

typedef struct msg_del_child_reply {
    int error;
} msg_del_child_reply_t;

typedef struct msg_list_children {
    ipc_port_id reply_port;
} msg_list_children_t;

typedef struct msg_list_children_reply {
    int error;
    uint32_t count;
} msg_list_children_reply_t;

#endif /* _YCONFIG_PROTOCOL_H_ */
