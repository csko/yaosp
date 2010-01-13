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

#include <fcntl.h>
#include <unistd.h>
#include <inttypes.h>
#include <stdlib.h>
#include <yaosp/debug.h>
#include <yutil/array.h>

#include <configserver/storage.h>
#include <configserver/attribute.h>

enum {
    TYPE_NODE = 1,
    TYPE_ATTRIBUTE = 2
};

enum {
    ATTR_NUMERIC = 0,
    ATTR_ASCII,
    ATTR_BOOL,
    ATTR_BINARY
};

static int binary_file = -1;

static node_t* binary_read_node( int f, char* name ) {
    node_t* node;
    uint64_t offset;

    if ( read( f, &offset, 8 ) != 8 ) {
        return NULL;
    }

    node = node_create( name );

    if ( node == NULL ) {
        return NULL;
    }

    node->file_offset = offset;

    return node;
}

static int binary_read_attribute_numeric( int f, attribute_t* attrib ) {
    uint64_t value;

    if ( read( f, &value, 8 ) != 8 ) {
        return -1;
    }

    attrib->value.numeric = value;

    return 0;
}

static int binary_read_attribute_ascii( int f, attribute_t* attrib ) {
    uint32_t data_length;
    uint64_t data_offset;

    if ( ( read( f, &data_length, 4 ) != 4 ) ||
         ( read( f, &data_offset, 8 ) != 8 ) ) {
        return -1;
    }

    attrib->value.ascii.offset = data_offset;
    attrib->value.ascii.size = data_length;

    return 0;
}

static int binary_read_attribute_bool( int f, attribute_t* attrib ) {
    uint8_t value;

    if ( read( f, &value, 1 ) != 1 ) {
        return -1;
    }

    attrib->value.bool = value;

    return 0;
}

static int binary_read_attribute_binary( int f, attribute_t* attrib ) {
    uint32_t data_length;
    uint64_t data_offset;

    if ( ( read( f, &data_length, 4 ) != 4 ) ||
         ( read( f, &data_offset, 8 ) != 8 ) ) {
        return -1;
    }

    attrib->value.binary.offset = data_offset;
    attrib->value.binary.size = data_length;

    return 0;
}

static attribute_t* binary_read_attribute( int f, char* name ) {
    uint8_t type;
    attribute_t* attrib;

    if ( read( f, &type, 1 ) != 1 ) {
        return NULL;
    }

    attrib = attribute_create( name, type );

    if ( attrib == NULL ) {
        return NULL;
    }

    switch ( type ) {
        case ATTR_NUMERIC :
            binary_read_attribute_numeric( f, attrib );
            break;

        case ATTR_ASCII :
            binary_read_attribute_ascii( f, attrib );
            break;

        case ATTR_BOOL :
            binary_read_attribute_bool( f, attrib );
            break;

        case ATTR_BINARY :
            binary_read_attribute_binary( f, attrib );
            break;
    }

    return attrib;
}

static int binary_read_item( int f, array_t* children, array_t* attributes ) {
    uint8_t type;
    uint32_t name_length;
    char* name;

    if ( ( read( f, &type, 1 ) != 1 ) ||
         ( read( f, &name_length, 4 ) != 4 ) ) {
        return -1;
    }

    name = ( char* )malloc( name_length + 1 );

    if ( name == NULL ) {
        return -1;
    }

    if ( read( f, name, name_length ) != name_length ) {
        return -1;
    }

    name[ name_length ] = 0;

    switch ( type ) {
        case TYPE_NODE : {
            node_t* node = binary_read_node( f, name );

            if ( node != NULL ) {
                array_add_item( children, node );
            }

            break;
        }

        case TYPE_ATTRIBUTE : {
            attribute_t* attrib = binary_read_attribute( f, name );

            if ( attrib != NULL ) {
                array_add_item( attributes, attrib );
            }

            break;
        }
    }

    return 0;
}

static int binary_read_items_at( int f, off_t offset, node_t* parent ) {
    uint32_t i;
    uint32_t item_count;

    int j;
    int size;
    array_t children;
    array_t attributes;

    if ( ( init_array( &children ) != 0 ) ||
         ( init_array( &attributes ) != 0 ) ) {
        /* todo: proper error handling */
        return -1;
    }

    if ( lseek( f, offset, SEEK_SET ) != offset ) {
        return -1;
    }

    if ( read( f, &item_count, 4 ) != 4 ) {
        return -1;
    }

    for ( i = 0; i < item_count; i++ ) {
        binary_read_item( f, &children, &attributes );
    }

    /* Add and read the children nodes */

    size = array_get_size( &children );

    for ( j = 0; j < size; j++ ) {
        node_t* child = ( node_t* )array_get_item( &children, j );
        node_add_child( parent, child );
        binary_read_items_at( f, child->file_offset, child );
    }

    /* Add the attributes ... */

    size = array_get_size( &attributes );

    for ( j = 0; j < size; j++ ) {
        attribute_t* attrib = ( attribute_t* )array_get_item( &attributes, j );
        node_add_attribute( parent, attrib );
    }

    destroy_array( &children );
    destroy_array( &attributes );

    return 0;
}

static int binary_load( const char* filename, node_t** _root ) {
    node_t* root;

    root = node_create( "root" );
    /* todo: error checking */

    binary_file = open( filename, O_RDONLY );

    if ( binary_file < 0 ) {
        return -1;
    }

    binary_read_items_at( binary_file, 0, root );

    *_root = root;

    return 0;
}

static int binary_get_attribute_value( attribute_t* attrib, void* data ) {
    switch ( attrib->type ) {
        case NUMERIC :
            *( uint64_t* )data = attrib->value.numeric;
            break;

        case ASCII : {
            uint8_t* ascii_data;

            ascii_data = ( uint8_t* )data;

            if ( lseek( binary_file, attrib->value.ascii.offset,
                        SEEK_SET ) != attrib->value.ascii.offset ) {
                return -1;
            }

            if ( read( binary_file, ascii_data,
                       attrib->value.ascii.size ) != attrib->value.ascii.size ) {
                return -1;
            }

            ascii_data[ attrib->value.ascii.size ] = 0;

            break;
        }

        case BOOL :
            *( uint8_t* )data = attrib->value.bool;
            break;

        case BINARY :
            if ( lseek( binary_file, attrib->value.binary.offset,
                        SEEK_SET ) != attrib->value.binary.offset ) {
                return -1;
            }

            if ( read( binary_file, data,
                       attrib->value.binary.size ) != attrib->value.binary.size ) {
                return -1;
            }

            break;
    }

    return 0;
}

config_storage_t binary_storage = {
    .load = binary_load,
    .get_attribute_value = binary_get_attribute_value
};
