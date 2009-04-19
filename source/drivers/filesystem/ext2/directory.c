/* ext2 filesystem driver
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

#include <macros.h>
#include <errno.h>
#include <console.h>
#include <mm/kmalloc.h>
#include <vfs/vfs.h>

#include "ext2.h"

int ext2_do_walk_directory( ext2_cookie_t* cookie, vfs_inode_t* parent, ext2_walk_callback_t* callback, void* data ) {
    int error;
    bool result;
    uint8_t* block;
    uint32_t offset;
    ext2_dir_entry_t* entry;

    block = ( uint8_t* )kmalloc( cookie->blocksize );

    if ( block == NULL ) {
        return -ENOMEM;
    }

    offset = 0;

    while ( offset < parent->fs_inode.i_size ) {
        if ( __unlikely( ( offset % cookie->blocksize ) == 0 ) ) {
            uint32_t block_number;

            block_number = offset / cookie->blocksize;

            error = ext2_do_read_inode_block( cookie, parent, block_number, block );

            if ( __unlikely( error < 0 ) ) {
                goto out;
            }
        }

        entry = ( ext2_dir_entry_t* )( block + ( offset % cookie->blocksize ) );

        if ( __unlikely( entry->rec_len == 0 ) ) {
            error = -EINVAL;
            goto out;
        }

        result = callback( entry, data );

        if ( !result ) {
            error = 0;
            goto out;
        }

        offset += entry->rec_len;
    }

    error = -ENOENT;

out:
    kfree( block );

    return error;
}

int ext2_do_insert_entry( ext2_cookie_t* cookie, vfs_inode_t* parent, ext2_dir_entry_t* new_entry, int new_entry_size ) {
    int error;
    uint8_t* block;
    uint32_t offset;
    uint32_t new_block;
    ext2_dir_entry_t* tmp;
    ext2_dir_entry_t* entry;

    block = ( uint8_t* )kmalloc( cookie->blocksize );

    if ( block == NULL ) {
        return -ENOMEM;
    }

    offset = 0;

    while ( offset < parent->fs_inode.i_size ) {
        int real_size;
        int free_size;

        if ( __unlikely( ( offset % cookie->blocksize ) == 0 ) ) {
            uint32_t block_number;

            block_number = offset / cookie->blocksize;

            error = ext2_do_read_inode_block( cookie, parent, block_number, block );

            if ( __unlikely( error < 0 ) ) {
                goto out;
            }
        }

        entry = ( ext2_dir_entry_t* )( block + ( offset % cookie->blocksize ) );

        if ( __unlikely( entry->rec_len == 0 ) ) {
            error = -EINVAL;
            goto out;
        }

        real_size = sizeof( ext2_dir_entry_t ) + entry->name_len;
        real_size = ( real_size + 3 ) & ~3;
        free_size = entry->rec_len - real_size;

        /* Truncate an existing entry if possible */

        if ( new_entry_size <= free_size ) {
            /* Link the new entry in */

            entry->rec_len = real_size;

            tmp = ( ext2_dir_entry_t* )( ( uint8_t* )entry + real_size );

            memcpy( tmp, new_entry, new_entry_size );

            tmp->rec_len = free_size;

            /* Update the block on the disk */

            error = ext2_do_write_inode_block( cookie, parent, offset / cookie->blocksize, block );

            if ( error < 0 ) {
                return error;
            }

            error = 0;

            goto out;
        }

        offset += entry->rec_len;
    }

    /* Add a new block to the inode */

    memcpy( block, new_entry, new_entry_size );

    tmp = ( ext2_dir_entry_t* )block;

    tmp->rec_len = cookie->blocksize;

    /* Link the new block to the parent inode */

    error = ext2_do_get_new_inode_block( cookie, parent, &new_block );

    if ( error < 0 ) {
        goto out;
    }

    /* Write the block data to the disk */

    error = pwrite( cookie->fd, block, cookie->blocksize, new_block * cookie->blocksize );

    if ( __unlikely( error != cookie->blocksize ) ) {
        error = -EIO;
        goto out;
    }

    /* Update the parent inode */

    parent->fs_inode.i_size += cookie->blocksize;

    error = ext2_do_write_inode( cookie, parent );

    if ( __unlikely( error < 0 ) ) {
        goto out;
    }

out:
    error = 0;

    kfree( block );

    return error;
}
