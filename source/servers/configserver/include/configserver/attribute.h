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

#ifndef _CFG_ATTRIBUTE_H_
#define _CFG_ATTRIBUTE_H_

#include <yutil/hashtable.h>

typedef enum attr_type {
    NUMERIC,
    ASCII,
    BINARY,
    BOOL
} attr_type_t;

typedef struct attribute {
    hashitem_t hash;

    char* name;
    attr_type_t type;
    union {
        uint8_t bool;
        int64_t numeric;

        union {
            off_t offset;
            uint64_t size;
        } binary;

        union {
            off_t offset;
            uint32_t size;
        } ascii;
    } value;
} attribute_t;

void* attribute_key( hashitem_t* item );

#endif /* _CFG_NODE_H_ */
