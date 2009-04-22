/* ext2 filesystem driver
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

#include <errno.h>

#include "ext2.h"

int ext2_do_alloc_block( ext2_cookie_t* cookie, uint32_t* block_number ) {
    uint32_t i, j, k;
    uint32_t mask;
    uint32_t entry;
    uint32_t* bitmap;
    ext2_group_t* group;

    if ( cookie->super_block.s_free_blocks_count == 0 ) {
        return -ENOMEM;
    }

    for ( i = 0; i < cookie->ngroups; i++ ) {
        group = &cookie->groups[ i ];

        if ( group->descriptor.bg_free_blocks_count == 0 ) {
            continue;
        }

        bitmap = group->block_bitmap;

        for ( j = 0; j < ( cookie->blocksize * 8 ) / 32; j++, bitmap++ ) {
            entry = *bitmap;

            if ( entry == 0xFFFFFFFF ) {
                continue;
            }

            for ( k = 0, mask = 1; k < 32; k++, mask <<= 1 ) {
                if ( ( entry & mask ) == 0 ) {
                    cookie->groups[ i ].block_bitmap[ j ] |= mask;

                    group->descriptor.bg_free_blocks_count--;
                    cookie->super_block.s_free_blocks_count--;

                    *block_number = i * cookie->super_block.s_blocks_per_group + ( j * 32 + k ) + cookie->super_block.s_first_data_block;

                    group->flags |= EXT2_BLOCK_BITMAP_DIRTY;

                    return 0;
                }
            }
        }
    }

    return -ENOSPC;
}
