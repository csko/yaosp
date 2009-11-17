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

region_id create_region( const char* name, uint32_t size, region_flags_t flags, alloc_type_t alloc_method, void** _address ) {
#if 0
    return syscall5(
        SYS_create_region,
        ( int )name,
        size,
        ( int )flags,
        ( int )alloc_method,
        ( int )_address
    );
#endif

    return -1;
}

int delete_region( region_id id ) {
#if 0
    return syscall1(
        SYS_delete_region,
        id
    );
#endif

    return -1;
}

int remap_region( region_id id, void* address ) {
#if 0
    return syscall2(
        SYS_remap_region,
        id,
        ( int )address
    );
#endif

    return -1;
}

region_id clone_region( region_id id, void** address ) {
#if 0
    return syscall2(
        SYS_clone_region,
        id,
        ( int )address
    );
#endif

    return -1;
}
