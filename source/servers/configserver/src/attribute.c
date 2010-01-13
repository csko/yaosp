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

#include <stdlib.h>
#include <string.h>

#include <configserver/attribute.h>

void* attribute_key( hashitem_t* item ) {
    attribute_t* attr = ( attribute_t* )item;
    return attr->name;
}

attribute_t* attribute_create( const char* name, attr_type_t type ) {
    size_t name_length;
    attribute_t* attrib;

    name_length = strlen( name );

    attrib = ( attribute_t* )malloc( sizeof( attribute_t ) + name_length + 1 );

    if ( attrib == NULL ) {
        return NULL;
    }

    attrib->name = ( char* )( attrib + 1 );
    attrib->type = type;
    memcpy( attrib->name, name, name_length + 1 );

    return attrib;
}
