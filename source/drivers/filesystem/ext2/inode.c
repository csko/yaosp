/* ext2 filesystem driver (inode handling)
 *
 * Copyright (c) 2009 Attila Magyar, Zoltan Kovacs, Kornel Csernai
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
#include <console.h>
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
int ext2_do_read_inode( ext2_cookie_t* cookie, ext2_inode_t* inode ) {
    int error;
    uint8_t* buffer;
    uint32_t offset;
    uint32_t real_offset;
    uint32_t block_offset;
    uint32_t group;
    uint32_t inode_table_offset;

    ASSERT( cookie != NULL );
    ASSERT( inode != NULL );

    if ( inode->inode_number < EXT2_ROOT_INO ) {
        error = -EINVAL;
        goto error1;
    }

    buffer = ( uint8_t* )kmalloc( cookie->blocksize );

    if ( buffer == NULL ) {
        error = -ENOMEM;
        goto error1;
    }

    /**
     * The s_inodes_per_group field in the superblock structure tells us how many inodes are defined per group.
     * block group = (inode - 1) / s_inodes_per_group
     */

    group = ( inode->inode_number - 1 ) / cookie->super_block.s_inodes_per_group;

    ASSERT( group < cookie->ngroups );

    /**
     * Once the block is identified, the local inode index for the local inode table can be identified using:
     * local inode index = (inode - 1) % s_inodes_per_group
     */

    inode_table_offset = cookie->groups[ group ].descriptor.bg_inode_table * cookie->blocksize;

    offset = inode_table_offset + ( ( ( inode->inode_number - 1 ) % cookie->super_block.s_inodes_per_group ) * cookie->super_block.s_inode_size );

    real_offset = offset & ~( cookie->blocksize - 1 );
    block_offset = offset % cookie->blocksize;

    /* Make sure all calculations were right */

    ASSERT( ( block_offset + cookie->super_block.s_inode_size ) <= cookie->blocksize );

    /* Read the block in */

    error = pread( cookie->fd, buffer, cookie->blocksize, real_offset );

    if ( __unlikely( error != cookie->blocksize ) ) {
        error = -EIO;
        goto error2;
    }

    /* Copy the inode structure to the wrapper structure */

    memcpy( &inode->fs_inode, buffer + block_offset, sizeof( ext2_fs_inode_t ) );

    error = 0;

error2:
    kfree( buffer );

error1:
    return error;
}

int ext2_do_write_inode( ext2_cookie_t* cookie, ext2_inode_t* inode ) {
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

    /**
     * The s_inodes_per_group field in the superblock structure tells us how many inodes are defined per group.
     * block group = (inode - 1) / s_inodes_per_group
     */

    group = ( inode->inode_number - 1 ) / cookie->super_block.s_inodes_per_group;

    /**
     * Once the block is identified, the local inode index for the local inode table can be identified using:
     * local inode index = (inode - 1) % s_inodes_per_group
     */

    inode_table_offset = cookie->groups[ group ].descriptor.bg_inode_table * cookie->blocksize;

    offset = inode_table_offset + ( ( ( inode->inode_number - 1 ) % cookie->super_block.s_inodes_per_group ) * cookie->super_block.s_inode_size );

    real_offset = offset & ~( cookie->blocksize - 1 );
    block_offset = offset % cookie->blocksize;

    /* Make sure all calculations were right */

    ASSERT( ( block_offset + cookie->super_block.s_inode_size ) <= cookie->blocksize );

    /* Read the block in */

    error = pread( cookie->fd, buffer, cookie->blocksize, real_offset );

    if ( __unlikely( error != cookie->blocksize ) ) {
        error = -EIO;
        goto error2;
    }

    /* Copy the inode structure from the wrapper structure */

    memcpy( buffer + block_offset, &inode->fs_inode, sizeof( ext2_fs_inode_t ) );

    /* If the inode size on the disk is bigger than our struct then we fill the rest of it with 0s */

    if ( cookie->super_block.s_inode_size > sizeof( ext2_fs_inode_t ) ) {
        memset(
            buffer + block_offset + sizeof( ext2_fs_inode_t ),
            0,
            cookie->super_block.s_inode_size - sizeof( ext2_fs_inode_t )
        );
    }

    /* Write the inode table block back */

    error = pwrite( cookie->fd, buffer, cookie->blocksize, real_offset );

    if ( __unlikely( error != cookie->blocksize ) ) {
        error = -EIO;
        goto error2;
    }

    error = 0;

error2:
    kfree( buffer );

error1:
    return error;
}

int ext2_do_alloc_inode( ext2_cookie_t* cookie, ext2_inode_t* inode, bool for_directory ) {
    uint32_t i, j, k;
    uint32_t mask;
    uint32_t entry;
    uint32_t* bitmap;
    ext2_group_t* group;

    if ( cookie->super_block.s_free_inodes_count == 0 ) {
        return -ENOMEM;
    }

    for ( i = 0; i < cookie->ngroups; i++ ) {
        group = &cookie->groups[ i ];

        if ( group->descriptor.bg_free_inodes_count == 0 ) {
            continue;
        }

        bitmap = group->inode_bitmap;

        /* TODO: Handle superblock.s_first_ino here */

        for ( j = 0; j < ( cookie->blocksize * 8 ) / 32; j++, bitmap++ ) {
            entry = *bitmap;

            if ( entry == 0xFFFFFFFF ) {
                continue;
            }

            for ( k = 0, mask = 1; k < 32; k++, mask <<= 1 ) {
                if ( ( entry & mask ) == 0 ) {
                    cookie->groups[ i ].inode_bitmap[ j ] |= mask;

                    group->descriptor.bg_free_inodes_count--;
                    cookie->super_block.s_free_inodes_count--;

                    if ( for_directory ) {
                        group->descriptor.bg_used_dirs_count++;
                    }

                    inode->inode_number = i * cookie->super_block.s_inodes_per_group + ( j * 32 + k ) + 1;

                    group->flags |= EXT2_INODE_BITMAP_DIRTY;

                    return 0;
                }
            }
        }
    }

    return -ENOSPC;
}

int ext2_do_free_inode( ext2_cookie_t* cookie, ext2_inode_t* inode, bool is_directory ) {
    uint32_t group_number;
    uint32_t offset;
    ext2_group_t* group;

    group_number = ( inode->inode_number - 1 ) / cookie->super_block.s_inodes_per_group;
    offset =  ( inode->inode_number - 1 ) % cookie->super_block.s_inodes_per_group;

    ASSERT( group_number < cookie->ngroups );
    ASSERT( offset < cookie->super_block.s_inodes_per_group );

    group = &cookie->groups[ group_number ];

    group->inode_bitmap[ offset / 32 ] &= ~( 1UL << ( offset % 32 ) );

    group->descriptor.bg_free_inodes_count++;
    cookie->super_block.s_free_inodes_count++;

    if ( is_directory ) {
        group->descriptor.bg_used_dirs_count--;
    }

    group->flags |= EXT2_INODE_BITMAP_DIRTY;

    return 0;
}

int ext2_do_free_inode_blocks( ext2_cookie_t* cookie, ext2_inode_t* inode ) {
    int error;
    uint32_t i;
    uint32_t* block;
    uint32_t remaining_blocks;
    ext2_fs_inode_t* fs_inode;

    fs_inode = &inode->fs_inode;
    remaining_blocks = fs_inode->i_blocks / cookie->sectors_per_block;

    /* Free direct blocks */

    for ( i = 0; i < MIN( remaining_blocks, EXT2_NDIR_BLOCKS ); i++, remaining_blocks-- ) {
        if ( fs_inode->i_block[ i ] == 0 ) {
            kprintf( WARNING, "ext2: Non-allocated block found while freeing inode blocks!\n" );
            goto done;
        }

        error = ext2_do_free_block(
            cookie,
            fs_inode->i_block[ i ]
        );

        if ( error < 0 ) {
            return error;
        }
    }

    /* Free indirect blocks */

    if ( remaining_blocks > 0 ) {
        block = ( uint32_t* )kmalloc( cookie->blocksize );

        if ( block == NULL ) {
            return -ENOMEM;
        }

        ASSERT( fs_inode->i_block[ EXT2_IND_BLOCK ] != 0 );

        error = pread(
            cookie->fd,
            block,
            cookie->blocksize,
            fs_inode->i_block[ EXT2_IND_BLOCK ] * cookie->blocksize
        );

        if ( __unlikely( error != cookie->blocksize ) ) {
            kfree( block );
            return -EIO;
        }

        for ( i = 0; i < MIN( remaining_blocks, cookie->ptr_per_block ); i++, remaining_blocks-- ) {
            error = ext2_do_free_block(
                cookie,
                block[ i ]
            );

            if ( error < 0 ) {
                kfree( block );
                return error;
            }
        }

        kfree( block );

        error = ext2_do_free_block(
            cookie,
            fs_inode->i_block[ EXT2_IND_BLOCK ]
        );

        if ( error < 0 ) {
            return error;
        }
    }

    /* TODO: free ..... */

done:
    return 0;
}

int ext2_do_read_inode_block( ext2_cookie_t* cookie, ext2_inode_t* inode, uint32_t block_number, void* buffer ) {
    int error;
    uint32_t real_block_number;

    error = ext2_calc_block_num( cookie, inode, block_number, &real_block_number );

    if ( __unlikely( error < 0 ) ) {
        return error;
    }

    error = pread( cookie->fd, buffer, cookie->blocksize, real_block_number * cookie->blocksize );

    if ( __unlikely( error != cookie->blocksize ) ) {
        return -EIO;
    }

    return 0;
}

int ext2_do_write_inode_block( ext2_cookie_t* cookie, ext2_inode_t* inode, uint32_t block_number, void* buffer ) {
    int error;
    uint32_t real_block_number;

    error = ext2_calc_block_num( cookie, inode, block_number, &real_block_number );

    if ( __unlikely( error < 0 ) ) {
        return error;
    }

    error = pwrite( cookie->fd, buffer, cookie->blocksize, real_block_number * cookie->blocksize );

    if ( __unlikely( error != cookie->blocksize ) ) {
        return -EIO;
    }

    return 0;
}

int ext2_do_get_new_inode_block( ext2_cookie_t* cookie, ext2_inode_t* inode, uint32_t* new_block_number ) {
    int error;
    uint32_t* block = NULL;
    uint32_t new_index;
    uint32_t new_block;

    /* Calculate the next index for the new block */

    if ( inode->fs_inode.i_size != 0 ) {
        new_index = ( inode->fs_inode.i_size / cookie->blocksize ) + ( inode->fs_inode.i_size % cookie->blocksize ? 1 : 0 );
    } else {
        new_index = 0;
    }

    /* Allocate a new block */

    error = ext2_do_alloc_block( cookie, &new_block );

    if ( error < 0 ) {
        return error;
    }

    ASSERT( ( inode->fs_inode.i_blocks % cookie->sectors_per_block ) == 0 );

    /* First check direct blocks */

    if ( new_index < EXT2_NDIR_BLOCKS ) {
        ASSERT( inode->fs_inode.i_block[ new_index ] == 0 );

        inode->fs_inode.i_block[ new_index ] = new_block;

        goto out;
    }

    /* Try indirect block */

    new_index -= EXT2_NDIR_BLOCKS;

    if ( new_index < cookie->ptr_per_block ) {
        uint32_t ind_block;

        block = ( uint32_t* )kmalloc( cookie->blocksize );

        if ( __unlikely( block == NULL ) ) {
            return -ENOMEM;
        }

        ind_block = inode->fs_inode.i_block[ EXT2_IND_BLOCK ];

        if ( ind_block == 0 ) {
            memset( block, 0, cookie->blocksize );

            /* Allocate a new block for the indirect block */

            error = ext2_do_alloc_block( cookie, &ind_block );

            if ( error < 0 ) {
                goto error1;
            }

            inode->fs_inode.i_block[ EXT2_IND_BLOCK ] = ind_block;
            inode->fs_inode.i_blocks += cookie->sectors_per_block;
        } else {
            error = pread( cookie->fd, block, cookie->blocksize, ind_block * cookie->blocksize );

            if ( __unlikely( error != cookie->blocksize ) ) {
                error = -EIO;
                goto error1;
            }
        }

        /* Update the indirect block */

        ASSERT( block[ new_index ] == 0 );

        block[ new_index ] = new_block;

        /* Write the indirect block back to the disk */

        error = pwrite( cookie->fd, block, cookie->blocksize, ind_block * cookie->blocksize );

        if ( __unlikely( error != cookie->blocksize ) ) {
            error = -EIO;
            goto error1;
        }

        kfree( block );

        goto out;
    }

    /* Try double indirect block */

    new_index -= cookie->ptr_per_block;

    if ( new_index < cookie->doubly_indirect_block_count ) {
        uint32_t idx1;
        uint32_t idx2;
        uint32_t block_lvl1;
        uint32_t block_lvl2;

        block = ( uint32_t* )kmalloc( cookie->blocksize );

        if ( __unlikely( block == NULL ) ) {
            return -ENOMEM;
        }

        block_lvl1 = inode->fs_inode.i_block[ EXT2_DIND_BLOCK ];

        if ( block_lvl1 == 0 ) {
            memset( block, 0, cookie->blocksize );

            /* Allocate a new block for the double indirect block */

            error = ext2_do_alloc_block( cookie, &block_lvl1 );

            if ( error < 0 ) {
                goto error1;
            }

            inode->fs_inode.i_block[ EXT2_DIND_BLOCK ] = block_lvl1;
            inode->fs_inode.i_blocks += cookie->sectors_per_block;
        } else {
            error = pread( cookie->fd, block, cookie->blocksize, block_lvl1 * cookie->blocksize );

            if ( __unlikely( error != cookie->blocksize ) ) {
                error = -EIO;
                goto error1;
            }
        }

        idx1 = new_index / cookie->ptr_per_block;
        idx2 = new_index % cookie->ptr_per_block;

        if ( block[ idx1 ] == 0 ) {
            /* Allocate a new block for the double indirect block */

            error = ext2_do_alloc_block( cookie, &block_lvl2 );

            if ( error < 0 ) {
                goto error1;
            }

            block[ idx1 ] = block_lvl2;
            inode->fs_inode.i_blocks += cookie->sectors_per_block;

            /* Write it back to the disk */

            error = pwrite( cookie->fd, block, cookie->blocksize, block_lvl1 * cookie->blocksize );

            if ( __unlikely( error != cookie->blocksize ) ) {
                error = -EIO;
                goto error1;
            }

            memset( block, 0, cookie->blocksize );
        } else {
            block_lvl2 = block[ idx1 ];

            error = pread( cookie->fd, block, cookie->blocksize, block_lvl2 * cookie->blocksize );

            if ( __unlikely( error != cookie->blocksize ) ) {
                error = -EIO;
                goto error1;
            }
        }

        /* Update the index */

        ASSERT( block[ idx2 ] == 0 );

        block[ idx2 ] = new_block;

        /* Write the block to the disk */

        error = pwrite( cookie->fd, block, cookie->blocksize, block_lvl2 * cookie->blocksize );

        if ( __unlikely( error != cookie->blocksize ) ) {
            error = -EIO;
            goto error1;
        }

        kfree( block );

        goto out;
    }

    /* Try triple indirect block */

    new_index -= cookie->doubly_indirect_block_count;

    if ( new_index < cookie->triply_indirect_block_count ) {
        uint32_t idx1;
        uint32_t idx2;
        uint32_t idx3;
        uint32_t block_lvl1;
        uint32_t block_lvl2;
        uint32_t block_lvl3;

        block = ( uint32_t* )kmalloc( cookie->blocksize );

        if ( __unlikely( block == NULL ) ) {
            return -ENOMEM;
        }

        block_lvl1 = inode->fs_inode.i_block[ EXT2_TIND_BLOCK ];

        if ( block_lvl1 == 0 ) {
            memset( block, 0, cookie->blocksize );

            /* Allocate a new block for the double indirect block */

            error = ext2_do_alloc_block( cookie, &block_lvl1 );

            if ( error < 0 ) {
                goto error1;
            }

            inode->fs_inode.i_block[ EXT2_DIND_BLOCK ] = block_lvl1;
            inode->fs_inode.i_blocks += cookie->sectors_per_block;
        } else {
            error = pread( cookie->fd, block, cookie->blocksize, block_lvl1 * cookie->blocksize );

            if ( __unlikely( error != cookie->blocksize ) ) {
                error = -EIO;
                goto error1;
            }
        }

        idx1 = new_index / cookie->doubly_indirect_block_count;
        new_index %= cookie->doubly_indirect_block_count;
        idx2 = new_index / cookie->ptr_per_block;
        idx3 = new_index % cookie->ptr_per_block;

        if ( block[ idx1 ] == 0 ) {
            /* Allocate a new block for the double indirect block */

            error = ext2_do_alloc_block( cookie, &block_lvl2 );

            if ( error < 0 ) {
                goto error1;
            }

            block[ idx1 ] = block_lvl2;
            inode->fs_inode.i_blocks += cookie->sectors_per_block;

            /* Write it back to the disk */

            error = pwrite( cookie->fd, block, cookie->blocksize, block_lvl1 * cookie->blocksize );

            if ( __unlikely( error != cookie->blocksize ) ) {
                error = -EIO;
                goto error1;
            }

            memset( block, 0, cookie->blocksize );
        } else {
            block_lvl2 = block[ idx1 ];

            error = pread( cookie->fd, block, cookie->blocksize, block_lvl2 * cookie->blocksize );

            if ( __unlikely( error != cookie->blocksize ) ) {
                error = -EIO;
                goto error1;
            }
        }

        if ( block[ idx2 ] == 0 ) {
            /* Allocate a new block for the double indirect block */

            error = ext2_do_alloc_block( cookie, &block_lvl3 );

            if ( error < 0 ) {
                goto error1;
            }

            block[ idx2 ] = block_lvl3;
            inode->fs_inode.i_blocks += cookie->sectors_per_block;

            /* Write it back to the disk */

            error = pwrite( cookie->fd, block, cookie->blocksize, block_lvl2 * cookie->blocksize );

            if ( __unlikely( error != cookie->blocksize ) ) {
                error = -EIO;
                goto error1;
            }

            memset( block, 0, cookie->blocksize );
        } else {
            block_lvl3 = block[ idx2 ];

            error = pread( cookie->fd, block, cookie->blocksize, block_lvl3 * cookie->blocksize );

            if ( __unlikely( error != cookie->blocksize ) ) {
                error = -EIO;
                goto error1;
            }
        }

        /* Update the index */

        ASSERT( block[ idx3 ] == 0 );

        block[ idx3 ] = new_block;

        /* Write the block to the disk */

        error = pwrite( cookie->fd, block, cookie->blocksize, block_lvl3 * cookie->blocksize );

        if ( __unlikely( error != cookie->blocksize ) ) {
            error = -EIO;
            goto error1;
        }

        kfree( block );

        goto out;
    }

    error = -ENOMEM;

    goto error1;

out:
    inode->fs_inode.i_blocks += cookie->sectors_per_block;

    *new_block_number = new_block;

    return 0;

error1:
    /* TODO: free the new block */

    kfree( block );

    return error;
}
