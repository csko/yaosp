/* Architecture specific initialization functions of the C library
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

#include <string.h>
#include <inttypes.h>
#include <yaosp/debug.h>

typedef struct elf32_i386_copy_info {
    uint32_t from;
    uint32_t to;
    uint32_t size;
} elf32_i386_copy_info_t;

int __libc_arch_start( char** argv, char** envp, void* relp ) {
    uint32_t i;
    uint32_t copy_count;
    elf32_i386_copy_info_t* info;

    copy_count = *( uint32_t* )relp;
    info = ( elf32_i386_copy_info_t* )( ( uint32_t )relp + sizeof( uint32_t ) );

    for ( i = 0; i < copy_count; i++, info++ ) {
        memcpy( ( void* )info->to, ( void* )info->from, info->size );
    }

    return 0;
}
