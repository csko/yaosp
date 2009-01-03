/* Miscellaneous kernel functions
 *
 * Copyright (c) 2008 Zoltan Kovacs
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

#ifndef _KERNEL_H_
#define _KERNEL_H_

#define panic( format, arg... ) \
    handle_panic( __FILE__, __LINE__, format, ##arg )

int sys_dbprintf( const char* format, char** parameters );

void handle_panic( const char* file, int line, const char* format, ... );

int init_thread( void* arg );

int arch_late_init( void );
void kernel_main( void );

#endif // _KERNEL_H_
