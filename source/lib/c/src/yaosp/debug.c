/* Debug functions
 *
 * Copyright (c) 2009, 2010 Zoltan Kovacs
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

#include <yaosp/syscall.h>
#include <yaosp/syscall_table.h>

int dbprintf( const char* format, ... ) {
    return syscall2(
        SYS_dbprintf,
        ( unsigned int )format,
        ( unsigned int )( ( ( char** )( &format ) ) + 1 )
    );
}

int dbtrace( void ) {
    return syscall0( SYS_dbtrace );
}
