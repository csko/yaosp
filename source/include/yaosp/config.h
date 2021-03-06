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

#ifdef __cplusplus
extern "C" {
#endif

int ycfg_get_ascii_value( const char* path, const char* attrib, char** value );
int ycfg_get_numeric_value( const char* path, const char* attrib, uint64_t* value );
int ycfg_get_binary_value( const char* path, const char* attrib, void** _data, size_t* _size );

int ycfg_add_child( const char* path, const char* child );
int ycfg_del_child( const char* path, const char* child );
int ycfg_list_children( const char* path, char*** _children );

int ycfg_init( void );

#ifdef __cplusplus
}
#endif

#endif /* _YAOSP_CONFIG_H_ */
