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

#ifndef _YAOSP_REGION_H_
#define _YAOSP_REGION_H_

#define PAGE_SIZE 4096
#define PAGE_ALIGN(s) (((s)+PAGE_SIZE-1)&~(PAGE_SIZE-1))

typedef int region_id;

typedef enum region_flags {
    REGION_READ = ( 1 << 0 ),
    REGION_WRITE = ( 1 << 1 )
} region_flags_t;

region_id memory_region_create( const char* name, uint64_t size, uint32_t flags, void** address );
int memory_region_delete( region_id id );

int memory_region_remap_pages( region_id id, void* address );
int memory_region_alloc_pages( region_id id );
region_id memory_region_clone_pages( region_id id, void** address );

#endif /* _YAOSP_REGION_H_ */
