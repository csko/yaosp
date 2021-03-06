/* yaosp C library
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

#ifndef _DLFCN_H_
#define _DLFCN_H_

#define RTLD_LAZY 0x01
#define RTLD_NOW  0x02

#ifdef __cplusplus
extern "C" {
#endif

void* dlopen( const char* filename, int flag );
void* dlsym( void* handle, const char* symbol );
int dlclose( void* handle );
char* dlerror( void );

#ifdef __cplusplus
}
#endif

#endif /* _DLFCN_H_ */
