/* Image viewer application
 *
 * Copyright (c) 2010 Attila Magyar, Zoltan Kovacs
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
#include <unistd.h>
#include <yaosp/debug.h>

#include <ygui/application.h>

#include "ui.h"
#include "worker.h"

int main( int argc, char** argv ) {
    if ( application_init( APP_NONE ) != 0 ) {
        return EXIT_FAILURE;
    }

    ui_init();
    worker_init();
    worker_start();

    window_show( window );

    /* The mainloop of the application ... */

    application_run();

    worker_shutdown();

    return EXIT_SUCCESS;
}
