/* stat shell command
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <sys/stat.h>

static char* argv0;

int do_stat( char* file ) {
    struct stat st;
    char device_id[ 32 ];
    char* type;
    char buf[ sizeof( "DDD MMM DD HH:MM:SS ZZZ YYYY" ) + 1 ];
    tm_t timeval;
    int error;

    error = stat( file, &st );

    if ( error != 0 ) {
        fprintf( stderr, "%s: cannot stat `%s': %s\n", argv0, file, strerror( errno ) );
        return EXIT_FAILURE;
    }

    if ( S_ISDIR ( st.st_mode ) ) {
        type = "directory";
    } else if ( S_ISREG ( st.st_mode ) ) {
        type = "regular file";
    } else if ( S_ISCHR ( st.st_mode ) ) {
        type = "character device";
    } else if ( S_ISBLK ( st.st_mode ) ) {
        type = "block device";
    } else if ( S_ISFIFO ( st.st_mode ) ){
        type = "FIFO (named pipe)";
    } else if ( S_ISLNK ( st.st_mode ) ) {
        type = "symbolic link";
    } else if ( S_ISSOCK ( st.st_mode ) ) {
        type = "socket";
    } else {
        type = "unknown";
    }

    printf( "  File: `%s'\n", file );
    printf( "  Size: %-15llu Blocks: %-10lld IO Block: %-6d %s\n", st.st_size,
            st.st_blocks, st.st_blksize, type );
    snprintf( device_id, sizeof( device_id ), "%xh/%dd", st.st_dev, st.st_dev );
    printf( "Device: %s Inode: %-11lld Links: %d\n", device_id, st.st_ino, st.st_nlink );
    printf( "Access: none               Uid: none               Gid: none\n" );

    if ( gmtime_r( ( const time_t* )&st.st_atime, &timeval ) != NULL ) {
        strftime( buf, sizeof( buf ), "%c", &timeval );
        printf( "Access: %s\n", buf );
    }

    if ( gmtime_r( ( const time_t* )&st.st_mtime, &timeval ) != NULL ) {
        strftime( buf, sizeof( buf ), "%c", &timeval );
        printf( "Modify: %s\n", buf );
    }

    if ( gmtime_r( ( const time_t* )&st.st_ctime, &timeval ) != NULL ) {
        strftime( buf, sizeof( buf ), "%c", &timeval );
        printf( "Change: %s\n", buf );
    }

    return EXIT_SUCCESS;
}

int main( int argc, char** argv ) {
    int i;
    int ret = EXIT_SUCCESS;

    argv0 = argv[ 0 ];

    if ( argc > 1 ) {
        for ( i = 1; i < argc; i++){
            if( do_stat( argv[ i ] ) < 0 ){
                ret = EXIT_FAILURE;
            }
        }
    } else {
        fprintf( stderr, "%s: missing operand\n", argv[ 0 ] );
        return EXIT_FAILURE;
    }

    return ret;
}
