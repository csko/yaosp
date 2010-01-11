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

#include <stdio.h>
#include <stdlib.h>

#include "ui/ui.h"
#include "core/eventmanager.h"
#include "network/irc.h"

extern char* my_nick;

int main( int argc, char** argv ) {
    if ( argc < 2 ) {
        printf( "%s nick\n", argv[ 0 ] );
        return EXIT_SUCCESS;
    }

    my_nick = argv[ 1 ];

    init_event_manager();
    init_ui();
    init_irc();

    event_manager_mainloop();

    destroy_ui();

    return EXIT_SUCCESS;
}
