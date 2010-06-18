/* yaosp GUI library
 *
 * Copyright (c) 2009, 2010 Zoltan Kovacs
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
#include <assert.h>

#include <ygui/layout/layout.h>

int layout_inc_ref( layout_t* layout ) {
    layout->ref_count++;

    return 0;
}

int layout_dec_ref( layout_t* layout ) {
    assert( layout->ref_count > 0 );

    if ( --layout->ref_count == 0 ) {
        layout->ops = NULL;

        free( layout );
    }

    return 0;
}

int layout_get_type( layout_t* layout ) {
    return layout->type;
}

void* layout_get_data( layout_t* layout ) {
    return ( void* )( layout + 1 );
}

layout_t* create_layout( int type, layout_operations_t* ops, int data_size ) {
    layout_t* layout;

    layout = ( layout_t* )malloc( sizeof( layout_t ) + data_size );

    if ( layout == NULL ) {
        return NULL;
    }

    layout->ops = ops;
    layout->type = type;
    layout->ref_count = 1;

    return layout;
}
