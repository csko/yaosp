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

#include <ygui/layout/borderlayout.h>

static int borderlayout_do_layout( widget_t* widget ) {
    return 0;
}

static layout_operations_t borderlayout_ops = {
    .do_layout = borderlayout_do_layout
};

layout_t* create_border_layout( void ) {
    layout_t* layout;

    layout = create_layout( &borderlayout_ops );

    if ( layout == NULL ) {
        goto error1;
    }

    return layout;

error1:
    return NULL;
}
