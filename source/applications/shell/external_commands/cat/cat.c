/* cat shell command
 *
 * Copyright (c) 2009 Kornel Csernai
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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

#define BUFLEN 4096

char* argv0;
char buffer[BUFLEN];

/* Simple cat function */
int do_cat( char* file ) {
    struct stat st;
    int handle;
    ssize_t last_read;

    if ( stat( file, &st ) != 0 ) {
        fprintf( stderr, "%s: %s: No such file or directory\n", argv0, file );
        return -1;
    }

    handle = open(file, O_RDONLY);
    if(handle < 0){
        /* TODO: use errno */
        fprintf( stderr, "%s: %s: Cannot open file.\n", argv0, file );
        return -1;
    }

    for(;;){
        last_read = read(handle, buffer, BUFLEN);
        printf("%s", buffer);
        if(last_read != BUFLEN){
            break;
        }
    }

    close(handle);
    return 0;
}

int main( int argc, char** argv ) {
    int i;
    int ret = 0;

    argv0 = argv[0];

    if( argc > 1 ) {
        for ( i = 1; i < argc; i++){
            if( do_cat( argv[i] ) < 0 ){
                ret = 1;
            }
        }
    } else {
        /* TODO: Echo back from stdin to stdout */
        return 0;
    }

    return ret;
}
