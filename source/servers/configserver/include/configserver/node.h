/* Config server
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

#ifndef _CFG_NODE_H_
#define _CFG_NODE_H_

#include <yutil/hashtable.h>

#include <configserver/attribute.h>

typedef struct node {
    hashitem_t hash;

    char* name;
    hashtable_t children;
    hashtable_t attributes;

    uint64_t file_offset;
} node_t;

int node_add_child( node_t* parent, node_t* child );
int node_add_attribute( node_t* parent, attribute_t* attribute );

node_t* node_get_child( node_t* parent, char* name );
attribute_t* node_get_attribute( node_t* parent, char* name );

int node_dump( node_t* node, int level );

node_t* node_create( const char* name );

#endif /* _CFG_NODE_H_ */
