/* HTTP downloader application
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

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

#include <curl/curl.h>

static char* argv0 = NULL;

/* Example URL: http://domain.org/path/to/filename.txt */

static int validate_url( char* url ) {
    char* separator;

    if ( strncmp( url, "http://", 7 ) != 0 ) {
        return -1;
    }

    separator = strchr( url + 7, '/' );

    if ( ( separator == NULL ) ||
         ( strlen( separator ) == 1 ) ) {
        return -1;
    }

    return 0;
}

static size_t file_writer( void* ptr, size_t size, size_t nmemb, void* data ) {
    int file = *( int* )data;
    size_t realsize = size * nmemb;

    write( file, ptr, realsize );

    return realsize;
}

static int do_download( char* url, int file ) {
    CURL* curl;
    CURLcode res;

    curl = curl_easy_init();

    if ( curl == NULL ) {
        return -1;
    }

    curl_easy_setopt( curl, CURLOPT_URL, url );
    curl_easy_setopt( curl, CURLOPT_WRITEFUNCTION, file_writer );
    curl_easy_setopt( curl, CURLOPT_WRITEDATA, ( void* )&file );

    res = curl_easy_perform( curl );

    switch ( res ) {
        case CURLE_COULDNT_RESOLVE_HOST :
            fprintf( stderr, "%s: failed to resolv the hostname.\n", argv0 );
            break;

        case CURLE_COULDNT_CONNECT :
            fprintf( stderr, "%s: failed to connect to the server.\n", argv0 );
            break;

        case CURLE_OK :
            break;

        default :
            fprintf( stderr, "%s: unhandled cURL error: %d\n", argv0, res );
            break;
    }

    curl_easy_cleanup( curl );

    return 0;
}

int main( int argc, char** argv ) {
    int file;
    char* url;
    char* filename;

    if ( argc != 2 ) {
        fprintf( stderr, "%s url\n", argv[ 0 ] );
        return EXIT_FAILURE;
    }

    argv0 = argv[ 0 ];
    url = argv[ 1 ];

    if ( validate_url( url ) != 0 ) {
        fprintf( stderr, "%s: invalid url: %s.\n", argv0, url );
        fprintf( stderr, "%s: valid url format is http://domain/file\n", argv0 );
        return EXIT_FAILURE;
    }

    curl_global_init( CURL_GLOBAL_ALL );

    filename = strrchr( url, '/' ) + 1;

    file = open( filename, O_WRONLY | O_CREAT | O_TRUNC );

    if ( file < 0 ) {
        fprintf( stderr, "%s: failed to open %s: %s.\n", argv0, filename, strerror( errno ) );
        return EXIT_FAILURE;
    }

    do_download( url, file );

    close( file );

    curl_global_cleanup();

    return EXIT_SUCCESS;
}
