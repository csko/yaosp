/* Text editor application
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
#include <fcntl.h>
#include <unistd.h>

#include <ygui/window.h>
#include <ygui/textarea.h>
#include <yutil/string.h>

#include "io.h"
#include "statusbar.h"

extern window_t* window;
extern widget_t* textarea;

static int io_file_loader_insert_lines( void* data ) {
    array_t* lines;

    lines = ( array_t* )data;
    textarea_set_lines( textarea, lines );

    destroy_array( lines );
    free( lines );

    return 0;
}

static void* io_file_loader_thread( void* arg ) {
    int f;
    int ret;
    char tmp[ 8192 ];

    char* input = NULL;
    size_t input_size = 0;

    array_t* lines = ( array_t* )malloc( sizeof( array_t ) );
    /* todo: error checking */
    init_array( lines );
    array_set_realloc_size( lines, 256 );

    f = open( ( char* )arg, O_RDONLY );

    if ( f < 0 ) {
        goto out;
    }

    do {
        ret = read( f, tmp, sizeof( tmp ) );

        if ( ret > 0 ) {
            size_t new_input_size = input_size + ret;

            input = ( char* )realloc( input, new_input_size + 1 );
            /* todo: error checking */

            memcpy( input + input_size, tmp, ret );
            input[ new_input_size ] = 0;

            input_size = new_input_size;

            char* end;
            char* start = input;

            while ( ( end = strchr( start, '\n' ) ) != NULL ) {
                *end = 0;

                string_t* line = ( string_t* )malloc( sizeof( string_t ) );
                /* todo: error checking */
                init_string_from_buffer( line, start, end - start );

                array_add_item( lines, ( void* )line );

                start = end + 1;
            }

            if ( start > input ) {
                size_t remaining_input = input_size - ( start - input );

                if ( remaining_input == 0 ) {
                    free( input );
                    input = NULL;
                } else {
                    input = ( char* )realloc( input, remaining_input + 1 );
                    /* todo: error checking */

                    input[ remaining_input ] = 0;
                }

                input_size = remaining_input;
            }
        }
    } while ( ret == sizeof( tmp ) );

    close( f );

 out:
    if ( array_get_size( lines ) > 0 ) {
        window_insert_callback( window, io_file_loader_insert_lines, ( void* )lines );
    } else {
        destroy_array( lines );
        free( lines );
    }

    statusbar_set_text( "File '%s' opened.", ( char* )arg );

    return NULL;
}

int io_load_file( char* filename ) {
    pthread_t thread;
    pthread_attr_t attr;

    pthread_attr_init( &attr );
    pthread_attr_setname( &attr, "file loader" );

    pthread_create(
        &thread, &attr,
        io_file_loader_thread, ( void* )filename
    );

    pthread_attr_destroy( &attr );

    return 0;
}
