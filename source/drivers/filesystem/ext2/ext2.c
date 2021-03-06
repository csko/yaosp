/* ext2 filesystem driver
 *
 * Copyright (c) 2009, 2010 Attila Magyar, Zoltan Kovacs, Kornel Csernai
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
#include <console.h>
#include <macros.h>
#include <mm/kmalloc.h>
#include <vfs/filesystem.h>
#include <vfs/vfs.h>

#include "ext2.h"

/**
 * Determine the direct block number from linear block number.
 * The input parameter can be either direct, indirect, 2x indirect, 3x indirect block number.
 * The output is the number of the direct data block.
 */
int ext2_calc_block_num( ext2_cookie_t* cookie, ext2_inode_t* node, uint32_t block_number, uint32_t* out ) {
    int error;
    uint32_t* buffer;
    uint32_t ind_block;
    ext2_fs_inode_t* inode = ( ext2_fs_inode_t* )&node->fs_inode;

    /* First check if this is a direct block [0..11] */

    if ( block_number < EXT2_NDIR_BLOCKS ) {
        *out = inode->i_block[ block_number ];

        return 0;
    }

    block_number -= EXT2_NDIR_BLOCKS;

    /* Is this an indirect block? */

    if ( block_number < cookie->ptr_per_block ) {
        ind_block = inode->i_block[ EXT2_IND_BLOCK ];

        if ( ind_block == 0 ) {
            return -EINVAL;
        }

        buffer = ( uint32_t* )kmalloc( cookie->blocksize );

        if ( __unlikely( buffer == NULL ) ) {
            return -ENOMEM;
        }

        /* Read the indirect block from the disk */

        if ( pread( cookie->fd, buffer, cookie->blocksize,
                    ind_block * cookie->blocksize ) != cookie->blocksize ) {
            kfree( buffer );
            return -EIO;
        }

        /* The element of the indirect block points to the data */

        *out = buffer[ block_number ];

        kfree( buffer );

        return 0;
    }

    block_number -= cookie->ptr_per_block;

    /* Doubly-indirect block? */

    if ( block_number < cookie->doubly_indirect_block_count ) {
        ind_block = inode->i_block[ EXT2_DIND_BLOCK ];

        if ( ind_block == 0 ) {
            return -EINVAL;
        }

        buffer = ( uint32_t* )kmalloc( cookie->blocksize );

        if ( __unlikely( buffer == NULL ) ) {
            return -ENOMEM;
        }

        /* Read the double-indirect block from the disk */

        error = pread( cookie->fd, buffer, cookie->blocksize, ind_block * cookie->blocksize );

        if ( __unlikely( error != cookie->blocksize ) ) {
            kfree( buffer );
            return -EIO;
        }

        ind_block = buffer[ block_number / cookie->ptr_per_block ];

        if ( ind_block == 0 ) {
            kfree( buffer );
            return -EINVAL;
        }

        /* Read the single-indirect block from the disk */

        error =  pread( cookie->fd, buffer, cookie->blocksize, ind_block * cookie->blocksize );

        if ( __unlikely( error != cookie->blocksize ) ) {
            kfree( buffer );
            return -EIO;
        }

        /* Find the direct block */

        *out = buffer[ block_number % cookie->ptr_per_block ];

        kfree( buffer );

        return 0;

    }

    block_number -= cookie->doubly_indirect_block_count;

    /* Triply-indirect block... wow :) */

    if ( block_number < cookie->triply_indirect_block_count ) {
        uint32_t mod;

        ind_block = inode->i_block[ EXT2_TIND_BLOCK ];

        if ( ind_block == 0 ) {
            return -EINVAL;
        }

        buffer = ( uint32_t* )kmalloc( cookie->blocksize );

        if ( __unlikely( buffer == NULL ) ) {
            return -ENOMEM;
        }

        error = pread( cookie->fd, buffer, cookie->blocksize, ind_block * cookie->blocksize );

        /* Read in the triply-indirect block from the disk */

        if ( __unlikely( error != cookie->blocksize ) ) {
            kfree( buffer );
            return -EIO;
        }

        ind_block = buffer[ block_number / cookie->doubly_indirect_block_count ];

        if ( ind_block == 0 ) {
            kfree( buffer );
            return -EINVAL;
        }

        /* Read the doubly-indirect block from the disk */

        error = pread( cookie->fd, buffer, cookie->blocksize, ind_block * cookie->blocksize );

        if ( __unlikely( error != cookie->blocksize ) ) {
            kfree( buffer );
            return -EIO;
        }

        /* Find the indirect-direct block */

        mod = block_number % cookie->doubly_indirect_block_count;
        ind_block = buffer[ mod / cookie->ptr_per_block ];

        /* Read the indirect-block from the disk */

        error = pread( cookie->fd, buffer, cookie->blocksize, ind_block * cookie->blocksize );

        if ( __unlikely( error != cookie->blocksize ) ) {
            kfree( buffer );
            return -EIO;
        }

        *out = buffer[ mod % cookie->ptr_per_block ];

        kfree( buffer );

        return 0;
    }

    return -ERANGE;
}

static int ext2_flush_group_descriptors( ext2_cookie_t* cookie ) {
    int error;
    uint32_t i;
    uint8_t* block;
    ext2_group_t* group;

    block = ( uint8_t* )kmalloc( cookie->blocksize );

    if ( __unlikely( block == NULL ) ) {
        return -ENOMEM;
    }

    for ( i = 0; i < cookie->ngroups; i++ ) {
        bool do_flush_gd;
        uint32_t gd_offset;
        uint32_t block_number;
        uint32_t block_offset;

        group = &cookie->groups[ i ];
        do_flush_gd = false;

        if ( group->flags & EXT2_INODE_BITMAP_DIRTY ) {
            if ( pwrite( cookie->fd, group->inode_bitmap, cookie->blocksize,
                         group->descriptor.bg_inode_bitmap * cookie->blocksize ) != cookie->blocksize ) {
                error = -EIO;
                goto out;
            }

            group->flags &= ~EXT2_INODE_BITMAP_DIRTY;
            do_flush_gd = true;
        }


        if ( group->flags & EXT2_BLOCK_BITMAP_DIRTY ) {
            if ( pwrite( cookie->fd, group->block_bitmap, cookie->blocksize,
                         group->descriptor.bg_block_bitmap * cookie->blocksize ) != cookie->blocksize ) {
                error = -EIO;
                goto out;
            }

            group->flags &= ~EXT2_BLOCK_BITMAP_DIRTY;
            do_flush_gd = true;
        }

        /* Update the group descriptor */

        if ( do_flush_gd == false ) {
            continue;
        }

        gd_offset = ( EXT2_MIN_BLOCK_SIZE / cookie->blocksize + 1 ) * cookie->blocksize + i * sizeof( ext2_group_desc_t );

        block_number = gd_offset / cookie->blocksize;
        block_offset = gd_offset % cookie->blocksize;

        ASSERT( ( block_offset + sizeof( ext2_group_desc_t ) ) <= cookie->blocksize );

        if ( pread( cookie->fd, block, cookie->blocksize,
                    block_number * cookie->blocksize ) != cookie->blocksize ) {
            error = -EIO;
            goto out;
        }

        memcpy( block + block_offset, ( void* )&group->descriptor, sizeof( ext2_group_desc_t ) );

        if ( pwrite( cookie->fd, block, cookie->blocksize,
                     block_number * cookie->blocksize ) != cookie->blocksize ) {
            error = -EIO;
            goto out;
        }
    }

    error = 0;

out:
    kfree( block );

    return error;
}

static int ext2_flush_superblock( ext2_cookie_t* cookie ) {
    int error;

    /* Write the superblock back to the disk */

    error = pwrite( cookie->fd, ( void* )&cookie->super_block, sizeof( ext2_super_block_t ), 1024 );

    if ( __unlikely( error != sizeof( ext2_super_block_t ) ) ) {
        kprintf( ERROR, "ext2: Failed to flush superblock\n" );
        return -EIO;
    }

    /* TODO: Write the superblock to all groups (if required) */

    return 0;
}

static int ext2_read_inode( void *fs_cookie, ino_t inode_number, void** out ) {
    int error;
    ext2_cookie_t* cookie = ( ext2_cookie_t* )fs_cookie;
    ext2_inode_t* inode;

    inode = ( ext2_inode_t* )kmalloc( sizeof( ext2_inode_t ) );

    if ( inode == NULL ) {
        error = -ENOMEM;
        goto error1;
    }

    inode->inode_number = inode_number;

    mutex_lock( cookie->lock, LOCK_IGNORE_SIGNAL );
    error = ext2_do_read_inode( cookie, inode );
    mutex_unlock( cookie->lock );

    if ( error < 0 ) {
        goto error1;
    }

    *out = ( void* )inode;

    return 0;

error1:
    kfree( inode );

    return error;
}

static int ext2_write_inode( void* fs_cookie, void* node ) {
    int error;
    ext2_cookie_t* cookie;
    ext2_inode_t* inode;

    cookie = ( ext2_cookie_t* )fs_cookie;
    inode = ( ext2_inode_t* )node;

    mutex_lock( cookie->lock, LOCK_IGNORE_SIGNAL );

    /* Link count == 0 means that the inode is deleted, so we
       have to delete it from the disk as well. */

    if ( inode->fs_inode.i_links_count == 0 ) {
        /* Free the inode and the allocated blocks */

        error = ext2_do_free_inode_blocks(
            cookie,
            inode
        );

        if ( error < 0 ) {
            goto out;
        }

        error = ext2_do_free_inode(
            cookie,
            inode,
            ( ( inode->fs_inode.i_mode & S_IFMT ) == S_IFDIR )
        );

        if ( error < 0 ) {
            goto out;
        }

        /* Flush group descriptor(s) and the superblock */

        error = ext2_flush_group_descriptors( cookie );

        if ( error < 0 ) {
            goto out;
        }

        error = ext2_flush_superblock( cookie );

        if ( error < 0 ) {
            goto out;
        }
    } else {
        /* Simply write the inode to the disk */

        error = ext2_do_write_inode( cookie, inode );
    }

 out:
    mutex_unlock( cookie->lock );

    kfree( inode );

    return error;
}

static int ext2_read( void* fs_cookie, void* node, void* file_cookie, void* buffer, off_t pos, size_t size ) {
    int error;
    uint8_t* data;
    uint8_t* block = NULL;
    uint32_t trunc;
    size_t saved_size;
    uint32_t block_index;
    ext2_inode_t* inode;
    ext2_cookie_t* cookie;

    if ( size == 0 ) {
        return 0;
    }

    if ( pos < 0 ) {
        return -EINVAL;
    }

    cookie = ( ext2_cookie_t* )fs_cookie;
    inode = ( ext2_inode_t* )node;

    mutex_lock( cookie->lock, LOCK_IGNORE_SIGNAL );

    if ( pos >= inode->fs_inode.i_size ) {
        error = 0;
        goto out;
    }

    if ( ( pos + size ) > inode->fs_inode.i_size ) {
        size = inode->fs_inode.i_size - pos;
    }

    data = ( uint8_t* )buffer;
    block_index = pos / cookie->blocksize;

    saved_size = size;
    trunc = pos % cookie->blocksize;

    /* Handle the first block */

    if ( trunc != 0 ) {
        uint32_t to_copy;

        block = ( uint8_t* )kmalloc( cookie->blocksize );

        if ( __unlikely( block == NULL ) ) {
            error = -ENOMEM;
            goto out;
        }

        error = ext2_do_read_inode_block( cookie, inode, block_index, block );

        if ( __unlikely( error < 0 ) ) {
            goto out;
        }

        to_copy = MIN( size, cookie->blocksize - trunc );

        memcpy( data, block + trunc, to_copy );

        data += to_copy;
        size -= to_copy;

        block_index++;
    }

    /* Handle full blocks */

    while ( size >= cookie->blocksize ) {
        error = ext2_do_read_inode_block( cookie, inode, block_index, data );

        if ( __unlikely( error < 0 ) ) {
            goto out;
        }

        block_index++;
        size -= cookie->blocksize;
        data += cookie->blocksize;
    }

    /* Handle the last block */

    if ( size > 0 ) {
        if ( block == NULL ) {
            block = ( uint8_t* )kmalloc( cookie->blocksize );

            if ( __unlikely( block == NULL ) ) {
                error = -ENOMEM;
                goto out;
            }
        }

        error = ext2_do_read_inode_block( cookie, inode, block_index, block );

        if ( __unlikely( error < 0 ) ) {
            goto out;
        }

        memcpy( data, block, size );
    }

    if ( ( ( cookie->flags & MOUNT_RO ) == 0 ) &&
         ( ( cookie->flags & MOUNT_NOATIME ) == 0 ) ) {
        /* Update the access time of the inode, ext2_write_inode() will flush it to the disk */

       inode->fs_inode.i_atime = time( NULL );
    }

    error = saved_size;

 out:
    mutex_unlock( cookie->lock );

    if ( block != NULL ) {
        kfree( block );
    }

    return error;
}

static int ext2_write( void* fs_cookie, void* node, void* _file_cookie, const void* buffer, off_t pos, size_t size ) {
    int error;
    uint8_t* data;
    ext2_cookie_t* cookie;
    ext2_inode_t* inode;
    uint32_t block_number;
    size_t saved_size;
    uint32_t inode_block;
    ext2_file_cookie_t* file_cookie;

    if ( size == 0 ) {
        return 0;
    }

    cookie = ( ext2_cookie_t* )fs_cookie;

    inode = ( ext2_inode_t* )node;
    file_cookie = ( ext2_file_cookie_t* )_file_cookie;
    data = ( uint8_t* )buffer;

    mutex_lock( cookie->lock, LOCK_IGNORE_SIGNAL );

    if ( ( pos < 0 ) ||
         ( pos > inode->fs_inode.i_size ) ) {
        error = -EINVAL;
        goto out2;
    }

    saved_size = size;

    if ( file_cookie->open_flags & O_APPEND ) {
        pos = inode->fs_inode.i_size;
    }

    inode_block = pos / cookie->blocksize;

    if ( pos < ROUND_UP(inode->fs_inode.i_size, cookie->blocksize) ) {
        uint32_t tmp;
        uint32_t tmp2;

        tmp = ROUND_UP(inode->fs_inode.i_size, cookie->blocksize) - ROUND_DOWN(pos, cookie->blocksize);
        ASSERT( tmp > 0 );
        ASSERT( ( tmp % cookie->blocksize ) == 0 );

        /* We have tmp / blocksize blocks at the end of the file that we can write without any allocation */

        tmp2 = pos % cookie->blocksize;

        /* If pos is not aligned to blocksize then we have to handle the first block specially */

        if ( tmp2 != 0 ) {
            uint8_t* block;
            uint32_t to_write;

            block = ( uint8_t* )kmalloc( cookie->blocksize );

            if ( block == NULL ) {
                error = -ENOMEM;
                goto out2;
            }

            error = ext2_do_read_inode_block( cookie, inode, inode_block, block );

            if ( error < 0 ) {
                kfree( block );
                goto out2;
            }

            to_write = MIN( cookie->blocksize - tmp2, size );

            memcpy( block + tmp2, data, to_write );

            error = ext2_do_write_inode_block( cookie, inode, inode_block, block );

            kfree( block );

            if ( error < 0 ) {
                goto out2;
            }

            data += to_write;
            size -= to_write;
            tmp -= cookie->blocksize;

            inode_block++;

            inode->fs_inode.i_size = MAX( inode->fs_inode.i_size, pos + to_write );
        }

        while ( ( size > cookie->blocksize ) && ( tmp > 0 ) ) {
            error = ext2_do_write_inode_block( cookie, inode, inode_block, data );

            if ( error < 0 ) {
                goto out2;
            }

            data += cookie->blocksize;
            size -= cookie->blocksize;
            tmp -= cookie->blocksize;

            inode_block++;
            ASSERT( ( inode_block * cookie->blocksize ) <= inode->fs_inode.i_size );
        }

        if ( ( size > 0 ) && ( tmp > 0 ) ) {
            uint8_t* block;

            ASSERT( size < cookie->blocksize );
            ASSERT( tmp >= cookie->blocksize );

            block = ( uint8_t* )kmalloc( cookie->blocksize );

            if ( block == NULL ) {
                error = -ENOMEM;
                goto out2;
            }

            error = ext2_do_read_inode_block( cookie, inode, inode_block, block );

            if ( error < 0 ) {
                kfree( block );
                goto out2;
            }

            memcpy( block, data, size );

            error = ext2_do_write_inode_block( cookie, inode, inode_block, block );

            kfree( block );

            if ( error < 0 ) {
                error = -EIO;
                goto out2;
            }

            inode->fs_inode.i_size = MAX( inode->fs_inode.i_size, inode_block * cookie->blocksize + size );
        }
    }

    while ( size > cookie->blocksize ) {
        error = ext2_do_get_new_inode_block( cookie, inode, &block_number );

        if ( error < 0 ) {
            goto out2;
        }

        if ( pwrite( cookie->fd, data, cookie->blocksize,
                     block_number * cookie->blocksize ) != cookie->blocksize ) {
            error = -EIO;
            goto out2;
        }

        data += cookie->blocksize;
        size -= cookie->blocksize;

        inode->fs_inode.i_size += cookie->blocksize;
    }

    if ( size > 0 ) {
        uint8_t* block;

        error = ext2_do_get_new_inode_block( cookie, inode, &block_number );

        if ( error < 0 ) {
            goto out2;
        }

        block = ( uint8_t* )kmalloc( cookie->blocksize );

        if ( block == NULL ) {
            error = -ENOMEM;
            goto out2;
        }

        memcpy( block, data, size );
        memset( block + size, 0, cookie->blocksize - size );

        error = pwrite( cookie->fd, data, cookie->blocksize, block_number * cookie->blocksize );

        kfree( block );

        if ( error != cookie->blocksize ) {
            error = -EIO;
            goto out2;
        }

        inode->fs_inode.i_size += size;
    }

    /* Update the inode on the disk */

    error = ext2_do_write_inode( cookie, inode );

    if ( error < 0 ) {
        goto out2;
    }

    /* Update the group descriptors and the superblock */

    error = ext2_flush_group_descriptors( cookie );

    if ( error < 0 ) {
        goto out2;
    }

    error = ext2_flush_superblock( cookie );

    if ( error < 0 ) {
        goto out2;
    }

    /* Update the modification time of the inode, ext2_write_inode() will flush it to the disk */
    inode->fs_inode.i_mtime = time( NULL );

    error = saved_size;

 out2:
    mutex_unlock( cookie->lock );

    return error;
}

static int ext2_open_directory( ext2_inode_t* inode, void** out ) {
    ext2_dir_cookie_t* cookie;

    cookie = ( ext2_dir_cookie_t* )kmalloc( sizeof( ext2_dir_cookie_t ) );

    if ( cookie == NULL ) {
        return -ENOMEM;
    }

    cookie->dir_offset = 0;

    *out = cookie;

    return 0;
}

static int ext2_open_file( ext2_cookie_t* cookie, ext2_inode_t* inode, void** out, int flags ) {
    ext2_file_cookie_t* file_cookie;

    file_cookie = ( ext2_file_cookie_t* )kmalloc( sizeof( ext2_file_cookie_t ) );

    if ( file_cookie == NULL ) {
        return -ENOMEM;
    }

    file_cookie->open_flags = flags;

    if ( ( flags & O_TRUNC ) &&
         ( inode->fs_inode.i_size > 0 ) ) {
        int error;

        error = ext2_do_free_inode_blocks( cookie, inode );

        if ( error < 0 ) {
            return error;
        }
    }

    *out = file_cookie;

    return 0;
}

static int ext2_open( void* fs_cookie, void* node, int mode, void** file_cookie ) {
    int error;
    ext2_inode_t* inode;
    ext2_cookie_t* cookie;

    cookie = ( ext2_cookie_t* )fs_cookie;
    inode = ( ext2_inode_t* )node;

    mutex_lock( cookie->lock, LOCK_IGNORE_SIGNAL );

    if ( ( inode->fs_inode.i_mode & S_IFDIR ) == S_IFDIR ) {
        error = ext2_open_directory( inode, file_cookie );
    } else {
        error = ext2_open_file( cookie, inode, file_cookie, mode );
    }

    mutex_unlock( cookie->lock );

    return error;
}

static int ext2_read_directory( void* fs_cookie, void* node, void* file_cookie, struct dirent* direntry ) {
    int size;
    int error;
    uint8_t* block = NULL;
    uint32_t block_number;
    ext2_dir_entry_t *entry;
    ext2_inode_t* vparent = ( ext2_inode_t* )node;
    ext2_cookie_t* cookie = ( ext2_cookie_t* )fs_cookie;
    ext2_dir_cookie_t*  dir_cookie = ( ext2_dir_cookie_t* )file_cookie;

    block = ( uint8_t* )kmalloc( cookie->blocksize );

    if ( __unlikely( block == NULL ) ) {
        return -ENOMEM;
    }

    mutex_lock( cookie->lock, LOCK_IGNORE_SIGNAL );

 next_entry:
    if ( __unlikely( dir_cookie->dir_offset >= vparent->fs_inode.i_size ) ) {
        error = 0;
        goto out;
    }

    block_number = dir_cookie->dir_offset / cookie->blocksize;

    error = ext2_do_read_inode_block( cookie, vparent, block_number, block );

    if ( __unlikely( error < 0 ) ) {
        goto out;
    }

    /* Location of entry inside the block */

    entry = ( ext2_dir_entry_t* )( block + ( dir_cookie->dir_offset % cookie->blocksize ) );

    if ( __unlikely( entry->rec_len == 0 ) ) {
        error = -EINVAL;
        goto out;
    }

    /* Move to the next entry */

    dir_cookie->dir_offset += entry->rec_len;

    if ( entry->inode == 0 ) {
        goto next_entry;
    }

    size = entry->name_len > sizeof( direntry->name ) - 1 ? sizeof( direntry->name ) - 1 : entry->name_len;

    direntry->inode_number = entry->inode;
    memcpy( direntry->name, ( void* )( entry + 1 ), size );
    direntry->name[ size ] = 0;

    if ( ( ( cookie->flags & MOUNT_RO ) == 0 ) &&
         ( ( cookie->flags & MOUNT_NOATIME ) == 0 ) ) {
        /* Update the access time of the inode, ext2_write_inode() will flush it to the disk */

        vparent->fs_inode.i_atime = time( NULL );
    }

    error = 1;

 out:
    mutex_unlock( cookie->lock );

    kfree( block );

    return error;
}

static int ext2_rewind_directory( void* fs_cookie, void* node, void* file_cookie ) {
    ext2_cookie_t* cookie = ( ext2_cookie_t* )fs_cookie;
    ext2_dir_cookie_t* dir_cookie = ( ext2_dir_cookie_t* )file_cookie;

    mutex_lock( cookie->lock, LOCK_IGNORE_SIGNAL );
    dir_cookie->dir_offset = 0;
    mutex_unlock( cookie->lock );

    return 0;
}

static bool ext2_lookup_inode_helper( ext2_dir_entry_t* entry, void* _data ) {
    ext2_lookup_data_t* data;

    data = ( ext2_lookup_data_t* )_data;

    if ( ( entry->name_len != data->name_length ) ||
         ( strncmp( ( const char* )( entry + 1 ), data->name, data->name_length ) != 0 ) ) {
        return true;
    }

    data->inode_number = entry->inode;

    return false;
}

static int ext2_create( void* fs_cookie, void* node, const char* name, int name_length, int mode,
                        int perms, ino_t* inode_number, void** _file_cookie ) {
    int error;
    ext2_cookie_t* cookie;
    ext2_inode_t* parent;
    ext2_inode_t child;
    ext2_fs_inode_t* inode;
    ext2_file_cookie_t* file_cookie;
    ext2_lookup_data_t lookup_data;
    ext2_dir_entry_t* new_entry;

    cookie = ( ext2_cookie_t* )fs_cookie;
    parent = ( ext2_inode_t* )node;

    if ( name_length <= 0 ) {
        return -EINVAL;
    }

    lookup_data.name = ( char* )name;
    lookup_data.name_length = name_length;

    mutex_lock( cookie->lock, LOCK_IGNORE_SIGNAL );

    /* Make sure the name we want to create doesn't exist */

    error = ext2_do_walk_directory( fs_cookie, node, ext2_lookup_inode_helper, ( void* )&lookup_data );

    if ( error == 0 ) {
        error = -EEXIST;
        goto error1;
    }

    new_entry = ext2_do_alloc_dir_entry( name_length );

    if ( new_entry == NULL ) {
        error = -ENOMEM;
        goto error1;
    }

    file_cookie = ( ext2_file_cookie_t* )kmalloc( sizeof( ext2_file_cookie_t ) );

    if ( file_cookie == NULL ) {
        error = -ENOMEM;
        goto error1;
    }

    file_cookie->open_flags = mode;

    /* Allocate a new inode for the file */

    error = ext2_do_alloc_inode( fs_cookie, &child, false );

    if ( error < 0 ) {
        goto error2;
    }

    /* Fill the inode fields */

    inode = &child.fs_inode;

    memset( inode, 0, sizeof( ext2_fs_inode_t ) );

    inode->i_mode = S_IFREG | 0777;
    inode->i_atime = inode->i_mtime = inode->i_ctime = time( NULL );
    inode->i_links_count = 1;

    /* Write the inode to the disk */

    error = ext2_do_write_inode( fs_cookie, &child );

    if ( error < 0 ) {
        goto error3;
    }

    /* Link the new file to the directory */

    new_entry->inode = child.inode_number;
    new_entry->file_type = EXT2_FT_REG_FILE;
    memcpy( ( void* )( new_entry + 1 ), name, name_length );

    error = ext2_do_insert_entry( cookie, parent, new_entry, sizeof( ext2_dir_entry_t ) + name_length );

    if ( error < 0 ) {
        goto error3;
    }

    kfree( new_entry );

    /* Write dirty group descriptors and bitmaps to the disk */

    error = ext2_flush_group_descriptors( fs_cookie );

    if ( error < 0 ) {
        goto error4;
    }

    /* Write the updated superblock to the disk */

    error = ext2_flush_superblock( fs_cookie );

    if ( error < 0 ) {
        goto error4;
    }

    *inode_number = child.inode_number;
    *_file_cookie = ( void* )file_cookie;

    mutex_unlock( cookie->lock );

    return 0;

 error4:
    /* TODO: cleanup */

 error3:
    /* TODO: release the allocated inode */

 error2:
    kfree( file_cookie );

 error1:
    mutex_unlock( cookie->lock );

    return error;
}

static int ext2_unlink( void* fs_cookie, void* node, const char* name, int name_length ) {
    int error;
    ext2_cookie_t* cookie;
    ext2_inode_t* inode;
    ext2_inode_t tmp_inode;
    ext2_lookup_data_t lookup_data;
    ext2_inode_t* vfs_inode;
    mount_point_t* mnt_point;

    cookie = ( ext2_cookie_t* )fs_cookie;
    inode = ( ext2_inode_t* )node;

    lookup_data.name = ( char* )name;
    lookup_data.name_length = name_length;

    mutex_lock( cookie->lock, LOCK_IGNORE_SIGNAL );

    error = ext2_do_walk_directory( fs_cookie, node, ext2_lookup_inode_helper, ( void* )&lookup_data );

    if ( error < 0 ) {
        goto error1;
    }

    tmp_inode.inode_number = lookup_data.inode_number;

    error = ext2_do_read_inode( cookie, &tmp_inode );

    if ( error < 0 ) {
        goto error1;
    }

    if ( ( tmp_inode.fs_inode.i_mode & S_IFMT ) == S_IFDIR ) {
        error = -EISDIR;
        goto error1;
    }

    if ( tmp_inode.fs_inode.i_links_count > 1 ) {
        error = -EBUSY;
        goto error1;
    }

    /* Remove the inode from the directory */

    error = ext2_do_remove_entry( cookie, inode, tmp_inode.inode_number );

    if ( error < 0 ) {
        goto error1;
    }

    /* Decrease the reference count on the inode */

    mnt_point = get_mount_point_by_cookie( fs_cookie );
    ASSERT( mnt_point != NULL );

    error = get_vnode( mnt_point, tmp_inode.inode_number, ( void** )&vfs_inode );

    if ( error < 0 ) {
        goto error1;
    }

    ASSERT( vfs_inode->fs_inode.i_links_count == 1 );

    vfs_inode->fs_inode.i_links_count--;

    put_vnode( mnt_point, tmp_inode.inode_number );

    /* Flush group descriptor and superblock */

    error = ext2_flush_group_descriptors( cookie );

    if ( error < 0 ) {
        goto error1;
    }

    error = ext2_flush_superblock( cookie );

 error1:
    mutex_unlock( cookie->lock );

    return error;
}

static int ext2_mkdir( void* fs_cookie, void* node, const char* name, int name_length, int perms ) {
    int error;
    ext2_cookie_t* cookie;
    ext2_inode_t* parent;
    ext2_lookup_data_t lookup_data;
    ext2_dir_entry_t* new_entry;
    ext2_inode_t child;
    ext2_fs_inode_t* inode;

    cookie = ( ext2_cookie_t* )fs_cookie;
    parent = ( ext2_inode_t* )node;

    if ( name_length <= 0 ) {
        return -EINVAL;
    }

    lookup_data.name = ( char* )name;
    lookup_data.name_length = name_length;

    mutex_lock( cookie->lock, LOCK_IGNORE_SIGNAL );

    /* Make sure the name we want to create doesn't exist */

    error = ext2_do_walk_directory( cookie, parent, ext2_lookup_inode_helper, ( void* )&lookup_data );

    if ( error == 0 ) {
        error = -EEXIST;
        goto error1;
    }

    /* Create the new directory node */

    new_entry = ext2_do_alloc_dir_entry( name_length );

    if ( new_entry == NULL ) {
        error = -ENOMEM;
        goto error1;
    }

    /* Allocate a new inode for the directory */

    error = ext2_do_alloc_inode( fs_cookie, &child, true );

    if ( error < 0 ) {
        goto error2;
    }

    /* Fill the inode fields */

    inode = &child.fs_inode;

    memset( inode, 0, sizeof( ext2_fs_inode_t ) );

    inode->i_mode = S_IFDIR | 0777;
    inode->i_atime = inode->i_mtime = inode->i_ctime = time( NULL );
    inode->i_links_count = 2; /* 1 from parent and 1 from "." :) */

    /* Write the inode to the disk */

    error = ext2_do_write_inode( fs_cookie, &child );

    if ( error < 0 ) {
        goto error3;
    }

    /* Link the new directory to the parent */

    new_entry->inode = child.inode_number;
    new_entry->file_type = EXT2_FT_DIRECTORY;

    memcpy( ( void* )( new_entry + 1 ), name, name_length );

    error = ext2_do_insert_entry( cookie, parent, new_entry, sizeof( ext2_dir_entry_t ) + name_length );

    if ( error < 0 ) {
        goto error3;
    }

    /* Insert "." node */

    new_entry->inode = child.inode_number;
    new_entry->name_len = 1;

    memcpy( ( void* )( new_entry + 1 ), ".\0\0", 4 /* Copy the padding \0s as well */ );

    error = ext2_do_insert_entry( cookie, &child, new_entry, sizeof( ext2_dir_entry_t ) + 1 );

    if ( error < 0 ) {
        goto error3;
    }

    /* Insert ".." node */

    new_entry->inode = parent->inode_number;
    new_entry->name_len = 2;
    memcpy( ( void* )( new_entry + 1 ), "..\0", 4 /* Copy the padding \0s as well */ );

    error = ext2_do_insert_entry( cookie, &child, new_entry, sizeof( ext2_dir_entry_t ) + 2 );

    if ( error < 0 ) {
        goto error3;
    }

    kfree( new_entry );

    /* Increment the link count of the parent */

    parent->fs_inode.i_links_count++;

    error = ext2_do_write_inode( fs_cookie, parent );

    if ( error < 0 ) {
        goto error1; /* todo: is this correct? */
    }

    /* Write dirty group descriptors and bitmaps to the disk */

    error = ext2_flush_group_descriptors( fs_cookie );

    if ( error < 0 ) {
        goto error1; /* todo: is this correct? */
    }

    /* Write the updated superblock to the disk */

    error = ext2_flush_superblock( fs_cookie );

    if ( error < 0 ) {
        goto error1; /* todo: is this correct? */
    }

    mutex_unlock( cookie->lock );

    return 0;

 error3:
    /* TODO: release the allocated inode */

 error2:
    kfree( new_entry );

 error1:
    mutex_unlock( cookie->lock );

    return error;
}

static int ext2_rmdir( void* fs_cookie, void* node, const char* name, int name_length ) {
    int error;
    ext2_cookie_t* cookie;
    ext2_inode_t* inode;
    ext2_inode_t tmp_inode;
    ext2_lookup_data_t lookup_data;
    ext2_inode_t* vfs_inode;
    mount_point_t* mnt_point;

    cookie = ( ext2_cookie_t* )fs_cookie;
    inode = ( ext2_inode_t* )node;

    lookup_data.name = ( char* )name;
    lookup_data.name_length = name_length;

    mutex_lock( cookie->lock, LOCK_IGNORE_SIGNAL );

    error = ext2_do_walk_directory( fs_cookie, node, ext2_lookup_inode_helper, ( void* )&lookup_data );

    if ( error < 0 ) {
        goto out;
    }

    tmp_inode.inode_number = lookup_data.inode_number;

    error = ext2_do_read_inode( cookie, &tmp_inode );

    if ( error < 0 ) {
        goto out;
    }

    if ( ( tmp_inode.fs_inode.i_mode & S_IFMT ) != S_IFDIR ) {
        error = -ENOTDIR;
        goto out;
    }

    if ( tmp_inode.fs_inode.i_links_count > 2 ) {
        error = -EBUSY;
        goto out;
    }

    error = ext2_is_directory_empty( cookie, &tmp_inode );

    if ( error < 0 ) {
        goto out;
    }

    /* Remove the inode from the directory */

    error = ext2_do_remove_entry( cookie, inode, tmp_inode.inode_number );

    if ( error < 0 ) {
        goto out;
    }

    /* Decrease the reference count on the inode */

    mnt_point = get_mount_point_by_cookie( fs_cookie );
    ASSERT( mnt_point != NULL );

    error = get_vnode( mnt_point, tmp_inode.inode_number, ( void** )&vfs_inode );

    if ( error < 0 ) {
        goto out;
    }

    ASSERT( vfs_inode->fs_inode.i_links_count == 2 );
    vfs_inode->fs_inode.i_links_count = 0;

    put_vnode( mnt_point, tmp_inode.inode_number );

    /* Decrease the reference count on the parent as well */

    error = get_vnode( mnt_point, inode->inode_number, ( void** )&vfs_inode );

    if ( error < 0 ) {
        goto out;
    }

    ASSERT( vfs_inode->fs_inode.i_links_count > 2 );
    vfs_inode->fs_inode.i_links_count--;

    put_vnode( mnt_point, inode->inode_number );

    /* Flush group descriptor and superblock */

    error = ext2_flush_group_descriptors( cookie );

    if ( error < 0 ) {
        goto out;
    }

    error = ext2_flush_superblock( cookie );

 out:
    mutex_unlock( cookie->lock );

    return error;
}

static int ext2_symlink( void* fs_cookie, void* node, const char* name, int name_length, const char* link_path ) {
    int error;
    ext2_cookie_t* cookie;
    ext2_inode_t* parent;
    ext2_inode_t child;
    ext2_fs_inode_t* inode;
    ext2_lookup_data_t lookup_data;
    ext2_dir_entry_t* new_entry;
    size_t link_size;
    uint8_t* block;

    cookie = ( ext2_cookie_t* )fs_cookie;
    parent = ( ext2_inode_t* )node;

    if ( name_length <= 0 ) {
        return -EINVAL;
    }

    lookup_data.name = ( char* )name;
    lookup_data.name_length = name_length;

    mutex_lock( cookie->lock, LOCK_IGNORE_SIGNAL );

    /* Make sure the name we want to create doesn't exist */

    error = ext2_do_walk_directory( fs_cookie, node, ext2_lookup_inode_helper, ( void* )&lookup_data );

    if ( error == 0 ) {
        error = -EEXIST;
        goto error1;
    }

    new_entry = ext2_do_alloc_dir_entry( name_length );

    if ( new_entry == NULL ) {
        error = -ENOMEM;
        goto error1;
    }

    /* Allocate a new inode for the symlink */

    error = ext2_do_alloc_inode( fs_cookie, &child, false );

    if ( error < 0 ) {
        goto error1;
    }

    /* Fill the inode fields */

    inode = &child.fs_inode;

    memset( inode, 0, sizeof( ext2_fs_inode_t ) );

    inode->i_mode = S_IFLNK | 0777;
    inode->i_atime = inode->i_mtime = inode->i_ctime = time( NULL );
    inode->i_links_count = 1;

    /* Link the new inode to the directory */

    new_entry->inode = child.inode_number;
    new_entry->file_type = EXT2_FT_SYMLINK;
    memcpy( ( void* )( new_entry + 1 ), name, name_length );

    error = ext2_do_insert_entry( cookie, parent, new_entry, sizeof( ext2_dir_entry_t ) + name_length );

    if ( error < 0 ) {
        goto error1;
    }

    kfree( new_entry );

    /* Write the file data */

    link_size = strlen( link_path );

    if ( link_size <= ( EXT2_N_BLOCKS * sizeof( uint32_t ) ) ) {
        inode->i_size = link_size;

        memcpy( &inode->i_block[ 0 ], link_path, link_size );
    } else {
        block = ( uint8_t* )kmalloc( cookie->blocksize );

        if ( block == NULL ) {
            error = -ENOMEM;
            goto error1;
        }

        while ( link_size > 0 ) {
            size_t to_write;
            uint32_t block_number;

            to_write = MIN( link_size, cookie->blocksize );

            error = ext2_do_get_new_inode_block( cookie, &child, &block_number );

            if ( error < 0 ) {
                kfree( block );
                goto error1;
            }

            memcpy( block, link_path, to_write );

            if ( pwrite( cookie->fd, block, cookie->blocksize,
                         block_number * cookie->blocksize ) != cookie->blocksize ) {
                kfree( block );
                error = -EIO;
                goto error1;
            }

            link_path += to_write;
            link_size -= to_write;
            inode->i_size += to_write;
        }

        kfree( block );
    }

    /* Write the inode to the disk */

    error = ext2_do_write_inode( fs_cookie, &child );

    if ( error < 0 ) {
        goto error1;
    }

    /* Write dirty group descriptors and bitmaps to the disk */

    error = ext2_flush_group_descriptors( fs_cookie );

    if ( error < 0 ) {
        goto error1;
    }

    /* Write the updated superblock to the disk */

    error = ext2_flush_superblock( fs_cookie );

 error1:
    mutex_unlock( cookie->lock );

    return error;
}

static int ext2_readlink( void* fs_cookie, void* node, char* buffer, size_t length ) {
    int error;
    size_t to_copy;
    ext2_cookie_t* cookie;
    ext2_inode_t* inode;

    if ( length == 0 ) {
        return 0;
    }

    cookie = ( ext2_cookie_t* )fs_cookie;
    inode = ( ext2_inode_t* )node;

    mutex_lock( cookie->lock, LOCK_IGNORE_SIGNAL );

    if ( ( inode->fs_inode.i_mode & S_IFMT ) != S_IFLNK ) {
        error = -EINVAL;
        goto out;
    }

    to_copy = MIN( length - 1, inode->fs_inode.i_size );

    if ( to_copy == 0 ) {
        error = 0;
        goto out;
    }

    if ( inode->fs_inode.i_size <= ( sizeof( uint32_t ) * EXT2_N_BLOCKS ) ) {
        memcpy( buffer, &inode->fs_inode.i_block[ 0 ], to_copy );
    } else {
        uint8_t* block;
        uint32_t block_number = 0;

        block = ( uint8_t* )kmalloc( cookie->blocksize );

        if ( block == NULL ) {
            error = -ENOMEM;
            goto out;
        }

        while ( to_copy > 0 ) {
            size_t tmp;

            tmp = MIN( to_copy, cookie->blocksize );

            error = ext2_do_read_inode_block( cookie, inode, block_number, block );

            if ( error < 0 ) {
                goto out;
            }

            memcpy( buffer, block, tmp );

            to_copy -= tmp;
            buffer += tmp;

            block_number++;
        }

        kfree( block );
    }

    buffer[ to_copy ] = 0;

    mutex_unlock( cookie->lock );

    return ( int )to_copy;

 out:
    mutex_unlock( cookie->lock );

    return error;
}

static int ext2_lookup_inode( void* fs_cookie, void* _parent, const char* name, int name_length, ino_t* inode_number ) {
    int error;
    ext2_lookup_data_t lookup_data;
    ext2_cookie_t* cookie = ( ext2_cookie_t* )fs_cookie;
    ext2_inode_t* parent  = ( ext2_inode_t* )_parent;

    lookup_data.name = ( char* )name;
    lookup_data.name_length = name_length;

    mutex_lock( cookie->lock, LOCK_IGNORE_SIGNAL );
    error = ext2_do_walk_directory( cookie, parent, ext2_lookup_inode_helper, &lookup_data );
    mutex_unlock( cookie->lock );

    if ( error < 0 ) {
        return error;
    }

    *inode_number = lookup_data.inode_number;

    return 0;
}

static int ext2_read_stat( void* fs_cookie, void* node, struct stat* stat ) {
    ext2_cookie_t* cookie = ( ext2_cookie_t* )fs_cookie;
    ext2_inode_t* vinode = ( ext2_inode_t* )node;

    mutex_lock( cookie->lock, LOCK_IGNORE_SIGNAL );

    stat->st_ino     = vinode->inode_number;
    stat->st_size    = vinode->fs_inode.i_size;
    stat->st_mode    = vinode->fs_inode.i_mode;
    stat->st_atime   = vinode->fs_inode.i_atime;
    stat->st_mtime   = vinode->fs_inode.i_mtime;
    stat->st_ctime   = vinode->fs_inode.i_ctime;
    stat->st_uid     = vinode->fs_inode.i_uid;
    stat->st_gid     = vinode->fs_inode.i_gid;
    stat->st_nlink   = vinode->fs_inode.i_links_count;
    stat->st_blocks  = vinode->fs_inode.i_blocks;
    stat->st_blksize = cookie->blocksize;

    mutex_unlock( cookie->lock );

    return 0;
}

static int ext2_write_stat( void* fs_cookie, void* node, struct stat* stat, uint32_t mask ) {
    ext2_cookie_t* cookie;
    ext2_inode_t* inode;

    cookie = ( ext2_cookie_t* )fs_cookie;
    inode = ( ext2_inode_t* )node;

    mutex_lock( cookie->lock, LOCK_IGNORE_SIGNAL );

    if ( mask & WSTAT_ATIME ) {
        inode->fs_inode.i_atime = stat->st_atime;
    }

    if ( mask & WSTAT_MTIME ) {
        inode->fs_inode.i_mtime = stat->st_mtime;
    }

    if ( mask & WSTAT_CTIME ) {
        inode->fs_inode.i_ctime = stat->st_ctime;
    }

    /* NOTE: ext2_write_inode() will flush the changed fields to the disk
       when the inode is deleted from the kernel's inode cache. */

    mutex_unlock( cookie->lock );

    return 0;
}

static int ext2_free_cookie( void* fs_cookie, void* node, void* file_cookie ) {
    kfree( file_cookie );

    return 0;
}

static bool ext2_probe( const char* device ) {
    int fd;
    int error;

    ext2_super_block_t superblock;

    fd = open( device, O_RDONLY );

    if ( __unlikely( fd < 0 ) ) {
        return false;
    }

    error = pread( fd, &superblock, sizeof( ext2_super_block_t ), 1024 );

    close( fd );

    return ( ( error == sizeof( ext2_super_block_t ) ) &&
             ( superblock.s_magic == EXT2_SUPER_MAGIC ) &&
             ( superblock.s_state == EXT2_VALID_FS ) );
}

static int ext2_mount( const char* device, uint32_t flags, void** fs_cookie, ino_t* root_inode_number ) {
    int i;
    int result = 0;
    uint8_t* tmp;
    uint8_t* block;
    uint32_t gd_size;
    uint32_t gd_offset;
    uint32_t gd_read_size;
    uint32_t ptr_per_block;

    ext2_group_t* group;
    ext2_cookie_t* cookie;

    cookie = ( ext2_cookie_t* )kmalloc( sizeof( ext2_cookie_t ) );

    if ( __unlikely( cookie == NULL ) ) {
        return -ENOMEM;
    }

    /* Open the device */

    cookie->fd = open( device, ( flags & MOUNT_RO ) ? O_RDONLY : O_RDWR );

    if ( cookie->fd < 0 ) {
        result = cookie->fd;

        goto error1;
    }

    /* Read the superblock */

    if ( pread( cookie->fd, &cookie->super_block,
                sizeof( ext2_super_block_t ), 1024 ) != sizeof( ext2_super_block_t ) ) {
        result = -EIO;
        goto error2;
    }

    /* Validate the superblock */

    if ( cookie->super_block.s_magic != EXT2_SUPER_MAGIC ) {
        kprintf( ERROR, "ext2: Bad magic number: 0x%x\n", cookie->super_block.s_magic );
        result = -EINVAL;
        goto error2;
    }

    /* Check fs state */

    if ( cookie->super_block.s_state != EXT2_VALID_FS ) {
        kprintf( ERROR, "ext2: Partition is damaged or was not cleanly unmounted!\n" );
        result = -EINVAL;
        goto error2;
    }

    cookie->ngroups = ( cookie->super_block.s_blocks_count - cookie->super_block.s_first_data_block +
        cookie->super_block.s_blocks_per_group - 1 ) / cookie->super_block.s_blocks_per_group;
    cookie->blocksize = EXT2_MIN_BLOCK_SIZE << cookie->super_block.s_log_block_size;
    cookie->sectors_per_block = cookie->blocksize / 512;

    ptr_per_block = cookie->blocksize / sizeof( uint32_t );

    cookie->ptr_per_block = ptr_per_block;
    cookie->doubly_indirect_block_count = ptr_per_block * ptr_per_block;
    cookie->triply_indirect_block_count = cookie->doubly_indirect_block_count * ptr_per_block;

    gd_offset = ( EXT2_MIN_BLOCK_SIZE / cookie->blocksize + 1 ) * cookie->blocksize;
    gd_size = cookie->ngroups * sizeof( ext2_group_desc_t );
    gd_read_size = ROUND_UP( gd_size, cookie->blocksize );

    block = ( uint8_t* )kmalloc( gd_read_size );

    if ( block == NULL ) {
        result = -ENOMEM;
        goto error2;
    }

    /* Read the group descriptors */

    if ( pread( cookie->fd, block, gd_read_size, gd_offset ) != gd_read_size ) {
        result = -EIO;
        goto error3;
    }

    cookie->groups = ( ext2_group_t* )kmalloc( sizeof( ext2_group_t ) * cookie->ngroups );

    if ( cookie->groups == NULL ) {
        /* TODO: cleanup! */
        result = -ENOMEM;
        goto error3;
    }

    for ( i = 0, group = &cookie->groups[ 0 ], tmp = block;
          i < cookie->ngroups;
          i++, group++, tmp += sizeof( ext2_group_desc_t ) ) {
        memcpy( &group->descriptor, tmp, sizeof( ext2_group_desc_t ) );

        group->inode_bitmap = ( uint32_t* )kmalloc( cookie->blocksize );

        if ( group->inode_bitmap == NULL ) {
            result = -ENOMEM;
            goto error3;
        }

        group->block_bitmap = ( uint32_t* )kmalloc( cookie->blocksize );

        if ( group->block_bitmap == NULL ) {
            result = -ENOMEM;
            goto error3;
        }

        /* TODO: proper error handling */

        if ( pread( cookie->fd, group->inode_bitmap, cookie->blocksize,
                    group->descriptor.bg_inode_bitmap * cookie->blocksize ) != cookie->blocksize ) {
            result = -EIO;
            goto error3;
        }

        if ( pread( cookie->fd, group->block_bitmap, cookie->blocksize,
                    group->descriptor.bg_block_bitmap * cookie->blocksize ) != cookie->blocksize ) {
            result = -EIO;
            goto error3;
        }

        group->flags = 0;
    }

    kfree( block );

    /* Create a mutex for the volume */

    cookie->lock = mutex_create( "ext2 volume lock", MUTEX_NONE );

    if ( cookie->lock < 0 ) {
        goto error3;
    }

    cookie->flags = flags;

    /* Increase mount count and mark fs in use only in RW mode */

    if ( cookie->flags & ~MOUNT_RO ) {
        cookie->super_block.s_state = EXT2_ERROR_FS;
        cookie->super_block.s_mnt_count++;

        result = ext2_flush_superblock( cookie );

        if ( result < 0 ) {
            goto error3;
        }
    }

    *root_inode_number = EXT2_ROOT_INO;
    *fs_cookie = ( void* )cookie;

    return result;

 error3:
    kfree( block );

 error2:
    close( cookie->fd );

 error1:
    kfree( cookie );

    return result;
}

static int ext2_unmount( void* fs_cookie ) {
    int i;
    int error;
    ext2_cookie_t* cookie;
    ext2_group_t* group;

    cookie = ( ext2_cookie_t* )fs_cookie;

    mutex_destroy( cookie->lock );

    /* Free group descriptors and inode/block bitmaps */

    for ( i = 0; i < cookie->ngroups; i++ ) {
        group = &cookie->groups[ i ];

        kfree( group->inode_bitmap );
        kfree( group->block_bitmap );
    }

    kfree( cookie->groups );

    /* Mark filesystem as it is not more in use */

    if ( cookie->flags & ~MOUNT_RO ) {
        cookie->super_block.s_state = EXT2_VALID_FS;

        error = ext2_flush_superblock( cookie );

        if ( error < 0 ) {
            goto error1;
        }
    }

    /* Close the device */

    close( cookie->fd );

    error = 0;

error1:
    return error;
}

static filesystem_calls_t ext2_calls = {
    .probe = ext2_probe,
    .mount = ext2_mount,
    .unmount = ext2_unmount,
    .read_inode = ext2_read_inode,
    .write_inode = ext2_write_inode,
    .lookup_inode = ext2_lookup_inode,
    .open = ext2_open,
    .close = NULL,
    .free_cookie = ext2_free_cookie,
    .read = ext2_read,
    .write = ext2_write,
    .ioctl = NULL,
    .read_stat = ext2_read_stat,
    .write_stat = ext2_write_stat,
    .read_directory = ext2_read_directory,
    .rewind_directory = ext2_rewind_directory,
    .create = ext2_create,
    .unlink = ext2_unlink,
    .mkdir = ext2_mkdir,
    .rmdir = ext2_rmdir,
    .isatty = NULL,
    .symlink = ext2_symlink,
    .readlink = ext2_readlink,
    .set_flags = NULL,
    .add_select_request = NULL,
    .remove_select_request = NULL
};

int init_module( void ) {
    kprintf( INFO, "ext2: Registering filesystem driver.\n" );

    return register_filesystem( "ext2", &ext2_calls );
}

int destroy_module( void ) {
    return 0;
}
