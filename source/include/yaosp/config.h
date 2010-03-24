/* Configuration handling functions
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

#ifndef _YAOSP_CONFIG_H_
#define _YAOSP_CONFIG_H_

int ycfg_get_ascii_value( char* path, char* attrib, char** value );
int ycfg_get_numeric_value( char* path, char* attrib, uint64_t* value );
int ycfg_get_binary_value( char* path, char* attrib, void** _data, size_t* _size );

int ycfg_list_children( char* path, char*** _children );

int ycfg_init( void );

#endif /* _YAOSP_CONFIG_H_ */
