/* strtoll function
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

#include <ctype.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>
#include <inttypes.h>

long long int strtoll( const char* nptr, char** endptr, int base ) {
  int neg = 0;
  unsigned long long int v;
  const char* orig = nptr;

  while ( isspace( *nptr ) ) {
      nptr++;
  }

  if ( *nptr == '-' && isalnum( nptr[ 1 ] ) ) {
      neg = -1;
      nptr++;
  }

  v = strtoull( nptr, endptr, base );

  if ( endptr && *endptr == nptr ) {
      *endptr = ( char* )orig;
  }

  if ( v > LLONG_MAX ) {
    if ( v == 0x8000000000000000ull && neg ) {
      errno = 0;
      return v;
    }

    errno=ERANGE;

    return ( neg ? LLONG_MIN : LLONG_MAX );
  }

  return ( neg ? -v : v );
}

intmax_t strtoimax( const char* nptr, char** endptr, int base )	__attribute__(( alias( "strtoll" ) ));
