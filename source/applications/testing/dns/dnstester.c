/* DNS resolv tester application
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
#include <stdlib.h>
#include <netdb.h>
#include <assert.h>
#include <netinet/in.h>

int main( int argc, char** argv ) {
    uint8_t* addr;
    struct hostent* hent;

    printf( "[*] Resolving 1.2.3.4 ..." );

    hent = gethostbyname("1.2.3.4");
    assert( hent != NULL );
    assert( hent->h_addrtype == AF_INET );
    assert( hent->h_length == 4 );
    assert( hent->h_addr_list[0] != NULL );
    assert( hent->h_addr_list[1] == NULL );

    addr = (uint8_t*)hent->h_addr_list[0];
    assert( addr[0] == 1 );
    assert( addr[1] == 2 );
    assert( addr[2] == 3 );
    assert( addr[3] == 4 );

    printf( "OK\n" );

    // ---

    printf( "[*] Resolving svn.yaosp.org ..." );

    hent = gethostbyname("svn.yaosp.org");
    assert( hent != NULL );
    assert( hent->h_addrtype == AF_INET );
    assert( hent->h_length == 4 );
    assert( hent->h_addr_list[0] != NULL );
    assert( hent->h_addr_list[1] == NULL );

    addr = (uint8_t*)hent->h_addr_list[0];
    assert( addr[0] == 94 );
    assert( addr[1] == 199 );
    assert( addr[2] == 181 );
    assert( addr[3] == 22 );

    printf( "OK\n" );

    return EXIT_SUCCESS;
}
