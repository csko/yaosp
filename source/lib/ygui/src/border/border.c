/* yaosp GUI library
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

#include <stdlib.h>

#include <ygui/border/border.h>

int border_inc_ref( border_t* border ) {
    /* todo */
    return 0;
}

int border_dec_ref( border_t* border ) {
    /* todo */
    return 0;
}

border_t* create_border( border_operations_t* ops ) {
    border_t* border;

    border = ( border_t* )malloc( sizeof( border_t ) );

    if ( border == NULL ) {
        return NULL;
    }

    border->ref_count = 1;
    border->ops = ops;

    return border;
}
