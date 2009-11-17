/* Memory region functions
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

#include <yaosp/syscall.h>
#include <yaosp/syscall_table.h>
#include <yaosp/region.h>

region_id memory_region_create( const char* name, uint64_t size, uint32_t flags, void** address ) {
    return syscall4(
        SYS_memory_region_create,
        ( int )name,
        ( int )&size,
        ( int )flags,
        ( int )address
    );
}

int memory_region_delete( region_id id ) {
    return syscall1( SYS_memory_region_delete, id );
}

int memory_region_remap_pages( region_id id, void* address ) {
    return syscall2( SYS_memory_region_remap_pages, id, ( int )address );
}

int memory_region_alloc_pages( region_id id ) {
    return syscall1( SYS_memory_region_alloc_pages, id );
}

region_id memory_region_clone_pages( region_id id, void** address ) {
    return syscall2( SYS_memory_region_clone_pages, id, ( int )address );
}
