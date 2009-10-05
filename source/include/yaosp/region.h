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

typedef enum alloc_type {
    ALLOC_NONE,
    ALLOC_LAZY,
    ALLOC_PAGES,
    ALLOC_CONTIGUOUS
} alloc_type_t;

typedef enum region_flags {
    REGION_READ = ( 1 << 0 ),
    REGION_WRITE = ( 1 << 1 )
} region_flags_t;

region_id create_region( const char* name, uint32_t size, region_flags_t flags,
                         alloc_type_t alloc_method, void** _address );
int delete_region( region_id id );
int remap_region( region_id id, void* address );
region_id clone_region( region_id id, void** address );

#endif /* _YAOSP_REGION_H_ */
