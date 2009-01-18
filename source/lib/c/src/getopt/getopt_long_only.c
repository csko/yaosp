/* yaosp C library, based on GNU libc's getopt1.c
 *
 * Copyright (C) 1989-1994,1996-1999,2001,2003,2004
 * Free Software Foundation, Inc.
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
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h> /* NULL */

#include <getopt.h>
#include "getopt_int.h"

int getopt_long_only( int argc, char* const * argv,
    const char* shortopts, const struct option* longopts, int* longind ){
    return _getopt_internal (argc, argv, shortopts, longopts, longind, 1);
}
