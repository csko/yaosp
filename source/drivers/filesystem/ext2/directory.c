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

            error = ext2_get_inode_data( cookie, parent, block_number * cookie->blocksize, cookie->blocksize, block );

            if ( __unlikely( error < 0 ) ) {
                goto out;
            }
        }

        entry = ( ext2_dir_entry_t* )( block + ( offset % cookie->blocksize ) );

        if ( __unlikely( entry->rec_len == 0 ) ) {
            error = -EINVAL;
            goto out;
        }

        kprintf( "%s() name=%s inode=%llu\n", __FUNCTION__, entry->name, ( ino_t )entry->inode );

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
