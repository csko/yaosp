/* Filesystem tools
 *
 * Copyright (c) 2009 Zoltan Kovacs, Kornel Csernai
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

#ifndef _FSTOOLS_H_
#define _FSTOOLS_H_

typedef int fstools_action_func_t( void );

typedef struct filesystem_calls {
    const char* name;
    int ( *create )( const char* device );
} filesystem_calls_t;

typedef struct fstools_action {
    const char* name;
    fstools_action_func_t* function;
} fstools_action_t;

int fstools_do_create();

#endif /* _FSTOOLS_H_ */
