/* ext2 filesystem driver (inode handling)
 *
 * Copyright (c) 2009 Attila Magyar, Zoltan Kovacs
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

#include <types.h>
#include <errno.h>
#include <macros.h>
#include <mm/kmalloc.h>
#include <vfs/vfs.h>
#include <lib/string.h>

#include "ext2.h"

/**
 * This functions reads inode structure from the file system to the specified inode structure.
 *
 * @param cookie The ext2 filesystem cookie
 * @param inode The wrapper of the filesystem inode, this struct has to have a valid inode number
 * @return On success 0 is returned
 */
int ext2_do_read_inode( ext2_cookie_t* cookie, vfs_inode_t* inode ) {
    int error;
    uint8_t* buffer;
    uint32_t offset;
    uint32_t real_offset;
    uint32_t block_offset;
    uint32_t group;
    uint32_t inode_table_offset;

    if ( inode->inode_number < EXT2_ROOT_INO ) {
        error = -EINVAL;
        goto error1;
    }

    buffer = ( uint8_t* )kmalloc( cookie->blocksize );

    if ( buffer == NULL ) {
        error = -ENOMEM;
        goto error1;
    }

    /* The s_inodes_per_group field in the superblock structure tells us how many inodes are defined per group.
       block group = (inode - 1) / s_inodes_per_group */

    group = ( inode->inode_number - 1 ) / cookie->super_block.s_inodes_per_group;

    /* Once the block is identified, the local inode index for the local inode table can be identified using:
       local inode index = (inode - 1) % s_inodes_per_group */

    inode_table_offset = cookie->groups[ group ].descriptor.bg_inode_table * cookie->blocksize;

    offset = inode_table_offset + ( ( ( inode->inode_number - 1 ) % cookie->super_block.s_inodes_per_group ) * cookie->super_block.s_inode_size );

    real_offset = offset & ~( cookie->blocksize - 1 );
    block_offset = offset % cookie->blocksize;

    /* Make sure all calculation was right */

    ASSERT( ( block_offset + cookie->super_block.s_inode_size ) <= cookie->blocksize );

    /* Read the block in */

    if ( pread( cookie->fd, buffer, cookie->blocksize, real_offset ) != cookie->blocksize ) {
        error = -EIO;
        goto error2;
    }

    /* Copy the inode structure to the wrapper structure */

    memcpy( &inode->fs_inode, buffer + block_offset, sizeof( ext2_inode_t ) );

    error = 0;

error2:
    kfree( buffer );

error1:
    return error;
}

int ext2_do_write_inode( ext2_cookie_t* cookie, vfs_inode_t* inode ) {
    return -ENOSYS;
}

int ext2_do_alloc_inode( ext2_cookie_t* cookie, vfs_inode_t* inode ) {
    uint32_t i, j, k;
    uint32_t mask;
    uint32_t entry;
    uint32_t* bitmap;

    for ( i = 0; i < cookie->ngroups; i++ ) {
        bitmap = cookie->groups[ i ].inode_bitmap;

        for ( j = 0; j < ( cookie->blocksize * 8 ) / 32; j++, bitmap++ ) {
            entry = *bitmap;

            if ( entry == 0xFFFFFFFF ) {
                continue;
            }

            for ( k = 0, mask = 1; k < 32; k++, mask <<= 1 ) {
                if ( ( entry & mask ) == 0 ) {
                    cookie->groups[ i ].inode_bitmap[ j ] |= mask;

                    inode->inode_number = i * cookie->super_block.s_inodes_per_group + ( j * 32 + k ) + 1;

                    return 0;
                }
            }
        }
    }

    return -ENOMEM;
}
