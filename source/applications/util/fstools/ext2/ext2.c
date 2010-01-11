/* Ext2 filesystem implementation
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

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/param.h>

#include <yutil/array.h>

#include "../fstools.h"

#include "ext2.h"

#define do_read_block(block_number) \
    if ( pread( info->fd, block, info->block_size, (block_number) * info->block_size ) != info->block_size ) { \
        return -EIO; \
    }

#define do_write_block(block_number) \
    if ( pwrite( info->fd, block, info->block_size, (block_number) * info->block_size ) != info->block_size ) { \
        return -EIO; \
    }

#define do_write_block2(block_number, data) \
    if ( pwrite( info->fd, data, info->block_size, (block_number) * info->block_size ) != info->block_size ) { \
        return -EIO; \
    }

#define do_write_block3(block_number, data, block_count) \
    if ( pwrite( info->fd, data, info->block_size * (block_count), (block_number) * info->block_size ) != info->block_size * (block_count) ) { \
        return -EIO; \
    }

typedef struct ext2_group {
    ext2_group_desc_t descriptor;
    uint32_t* inode_bitmap;
    uint32_t* block_bitmap;
} ext2_group_t;

typedef struct ext2_block {
    uint32_t block_number;
    uint8_t* block_data;
} ext2_block_t;

typedef struct ext2_inode {
    uint32_t inode_number;
    ext2_fs_inode_t fs_inode;
} ext2_inode_t;

typedef struct ext2_info {
    int fd;
    uint32_t block_size;
    uint64_t device_size;
    uint64_t group_size;
    uint32_t group_count;
    uint32_t blocks_per_group;
    uint32_t inodes_per_group;

    ext2_group_t* groups;
    ext2_super_block_t superblock;

    array_t inode_list;
    array_t block_list;
} ext2_info_t;

int ext2_initialize( const char* device, ext2_info_t* info ) {
    int error;
    uint32_t i;
    ext2_group_t* group;
    device_geometry_t geometry;

    error = init_array( &info->block_list );

    if ( error < 0 ) {
        goto error1;
    }

    error = init_array( &info->inode_list );

    if ( error < 0 ) {
        goto error2;
    }

    info->fd = open( device, O_RDWR );

    if ( info->fd < 0 ) {
        goto error3;
    }

    error = ioctl( info->fd, IOCTL_DISK_GET_GEOMETRY, ( void* )&geometry );

    if ( error < 0 ) {
        goto error4;
    }

    info->block_size = 4096;
    info->device_size = geometry.sector_count * geometry.bytes_per_sector;
    info->group_size = info->block_size * 8 * info->block_size;
    info->group_count = ( info->device_size + info->group_size - 1 ) / info->group_size;
    info->blocks_per_group = info->group_size / info->block_size;
    info->inodes_per_group = MIN( 8192, info->block_size * 2 );

    info->groups = ( ext2_group_t* )malloc( sizeof( ext2_group_t ) * info->group_count );

    if ( info->groups == NULL ) {
        goto error4;
    }

    memset( info->groups, 0, sizeof( ext2_group_t ) * info->group_count );
    memset( &info->superblock, 0, sizeof( ext2_super_block_t ) );

    for ( i = 0; i < info->group_count; i++ ) {
        uint32_t first_block;
        uint64_t current_group_size;

        group = &info->groups[ i ];

        first_block = ( i * info->blocks_per_group );

        memset( ( void* )&group->descriptor, 0, sizeof( ext2_group_desc_t ) );

        /* Calculate free blocks count */

        current_group_size = ( info->device_size - ( i * info->group_size ) );
        current_group_size = MIN( current_group_size, info->group_size );

        group->descriptor.bg_free_blocks_count = current_group_size / info->block_size;

        info->superblock.s_blocks_count += group->descriptor.bg_free_blocks_count;

        /* Calculate free inodes count */

        group->descriptor.bg_free_inodes_count = info->inodes_per_group;

        info->superblock.s_inodes_count += group->descriptor.bg_free_inodes_count;

        group->descriptor.bg_block_bitmap = first_block + 2;
        group->descriptor.bg_inode_bitmap = first_block + 3;
        group->descriptor.bg_inode_table = first_block + 4;

        /* Create block and inode bitmaps for the current group */

        group->inode_bitmap = ( uint32_t* )malloc( info->block_size );

        if ( group->inode_bitmap == NULL ) {
            goto error5;
        }

        group->block_bitmap = ( uint32_t* )malloc( info->block_size );

        if ( group->block_bitmap == NULL ) {
            goto error5;
        }

        memset( group->inode_bitmap, 0, info->block_size );
        memset( group->block_bitmap, 0, info->block_size );
    }

    info->superblock.s_r_blocks_count = 0;
    info->superblock.s_free_blocks_count = info->superblock.s_blocks_count;
    info->superblock.s_free_inodes_count = info->superblock.s_inodes_count;
    info->superblock.s_first_data_block = 0;
    info->superblock.s_log_block_size = 2; /* NOTE: We assume 4096 block size here */
    info->superblock.s_log_frag_size = info->superblock.s_log_block_size;
    info->superblock.s_blocks_per_group = info->group_size / info->block_size;
    info->superblock.s_frags_per_group = info->superblock.s_blocks_per_group;
    info->superblock.s_inodes_per_group = info->inodes_per_group;
    info->superblock.s_max_mnt_count = 32;
    info->superblock.s_magic = EXT2_SUPER_MAGIC;
    info->superblock.s_state = EXT2_VALID_FS;
    info->superblock.s_errors = EXT2_ERRORS_CONTINUE;
    info->superblock.s_rev_level = 1;
    info->superblock.s_minor_rev_level = 0;
    info->superblock.s_feature_incompat = EXT2_FEATURE_INCOMPAT_FILETYPE;
    info->superblock.s_first_ino = 11;
    info->superblock.s_inode_size = sizeof( ext2_fs_inode_t );

    /* Mark the first 12 inode as used */

    info->superblock.s_free_inodes_count -= 11;
    info->groups[ 0 ].descriptor.bg_free_inodes_count -= 11;
    info->groups[ 0 ].descriptor.bg_used_dirs_count += 2;
    info->groups[ 0 ].inode_bitmap[ 0 ] = 0x7FF; /* inodes from 1 to 11 are reserved */

    /* Mark the blocks containing the superblocks, group descriptors, bitmaps and inode table as used */

    for ( i = 0; i < info->group_count; i++ ) {
        uint32_t j;
        uint32_t mark_blocks_at_end;

        uint32_t inode_table_blocks =
            ( info->superblock.s_inodes_per_group * sizeof( ext2_fs_inode_t ) + info->block_size - 1 ) / info->block_size;

        uint32_t blocks_used =
            1 /* superblock */ +
            1 /* group descriptors */ +
            1 /* block bitmap */ +
            1 /* inode bitmap */ +
            inode_table_blocks;

        group = &info->groups[ i ];

        if ( group->descriptor.bg_free_blocks_count < info->blocks_per_group ) {
            mark_blocks_at_end = info->blocks_per_group - group->descriptor.bg_free_blocks_count;
        } else {
            mark_blocks_at_end = 0;
        }

        group->descriptor.bg_free_blocks_count -= blocks_used;
        info->superblock.s_free_blocks_count -= blocks_used;

        for ( j = 0; j < blocks_used; j++ ) {
            group->block_bitmap[ j / 32 ] |= ( 1 << ( j % 32 ) );
        }

        for ( j = 0; j < mark_blocks_at_end; j++ ) {
            uint32_t current_block = info->blocks_per_group - ( j + 1 );

            group->block_bitmap[ current_block / 32 ] |= ( 1 << ( current_block % 32 ) );
        }
    }

    return 0;

error5:
    for ( i = 0; i < info->group_count; i++ ) {
        group = &info->groups[ i ];

        if ( group->inode_bitmap != NULL ) {
            free( group->inode_bitmap );
        }

        if ( group->block_bitmap != NULL ) {
            free( group->block_bitmap );
        }
    }

    free( info->groups );

error4:
    close( info->fd );

error3:
    destroy_array( &info->inode_list );

error2:
    destroy_array( &info->block_list );

error1:
    return -1;
}

int ext2_allocate_block( ext2_info_t* info, ext2_block_t** _block ) {
    uint32_t i;
    uint32_t j;
    uint32_t k;
    uint32_t mask;
    uint32_t entry;
    ext2_group_t* group;
    ext2_block_t* block;

    for ( i = 0; i < info->group_count; i++ ) {
        group = &info->groups[ i ];

        for ( j = 0; j < info->block_size * 8 / 32; j++ ) {
            entry = group->block_bitmap[ j ];

            for ( k = 0, mask = 1; k < 32; k++, mask <<= 1 ) {
                if ( ( entry & mask ) == 0 ) {
                    block = ( ext2_block_t* )malloc( sizeof( ext2_block_t ) );

                    if ( block == NULL ) {
                        goto error1;
                    }

                    block->block_number =
                        i * info->superblock.s_blocks_per_group + ( j * 32 ) + k + info->superblock.s_first_data_block;

                    block->block_data = ( uint8_t* )malloc( info->block_size );

                    if ( block->block_data == NULL ) {
                        goto error2;
                    }

                    group->block_bitmap[ j ] |= mask;

                    group->descriptor.bg_free_blocks_count--;
                    info->superblock.s_free_blocks_count--;

                    array_add_item( &info->block_list, ( void* )block );

                    *_block = block;

                    return 0;
                }
            }
        }
    }

    return -ENOSPC;

error2:
    free( block );

error1:
    return -ENOMEM;
}

int ext2_create_inode( ext2_info_t* info, uint32_t inode_number, ext2_inode_t** _inode ) {
    ext2_inode_t* inode;

    inode = ( ext2_inode_t* )malloc( sizeof( ext2_inode_t ) );

    if ( inode == NULL ) {
        return -ENOMEM;
    }

    inode->inode_number = inode_number;

    memset( &inode->fs_inode, 0, sizeof( ext2_fs_inode_t ) );

    array_add_item( &info->inode_list, ( void* )inode );

    *_inode = inode;

    return 0;
}

int ext2_delete_inode( ext2_info_t* info, ext2_inode_t* inode ) {
    /* TODO: remove from inode_list array */
    /* TODO: delete blocks allocated for this inode */

    free( inode );

    return 0;
}

int ext2_inode_add_block( ext2_info_t* info, ext2_inode_t* inode, ext2_block_t** _block ) {
    int error;
    ext2_block_t* block;

    error = ext2_allocate_block( info, &block );

    if ( error < 0 ) {
        return error;
    }

    /* TODO: at the moment we support only one block per inode... this should be enough for now :) */

    assert( inode->fs_inode.i_block[ 0 ] == 0 );

    inode->fs_inode.i_block[ 0 ] = block->block_number;
    inode->fs_inode.i_blocks = info->block_size / 512;

    *_block = block;

    return 0;
}

int ext2_dir_entry_initialize( ext2_dir_entry_t* entry, uint32_t inode, char* name, int name_len, int name_len_to_copy, int file_type ) {
    entry->inode = inode;
    entry->name_len = name_len;
    entry->file_type = file_type;

    assert( name_len <= name_len_to_copy );

    memcpy(
        ( void* )( entry + 1 ),
        name,
        name_len_to_copy
    );

    return 0;
}

int ext2_create_structure( ext2_info_t* info ) {
    int error;
    uint32_t offset;
    ext2_inode_t* root_inode;
    ext2_fs_inode_t* root_fs_ino;
    ext2_block_t* block;
    ext2_dir_entry_t* entry;
    ext2_inode_t* lf_inode;
    ext2_fs_inode_t* lf_fs_inode;

    /* Create the root inode */

    error = ext2_create_inode( info, 2, &root_inode );

    if ( error < 0 ) {
        goto error1;
    }

    root_fs_ino = &root_inode->fs_inode;

    root_fs_ino->i_mode = S_IFDIR | 0777;
    root_fs_ino->i_size = info->block_size;
    root_fs_ino->i_links_count = 3;

    error = ext2_inode_add_block( info, root_inode, &block );

    if ( error < 0 ) {
        goto error2;
    }

    offset = 0;
    entry = ( ext2_dir_entry_t* )block->block_data;

    entry->rec_len = sizeof( ext2_dir_entry_t ) + 4;
    ext2_dir_entry_initialize( entry, 2, ".\0\0", 1, 4, EXT2_FT_DIRECTORY );

    offset += sizeof( ext2_dir_entry_t ) + 4;

    entry = ( ext2_dir_entry_t* )( block->block_data + offset );

    entry->rec_len = sizeof( ext2_dir_entry_t ) + 4;
    ext2_dir_entry_initialize( entry, 2, "..\0", 2, 4, EXT2_FT_DIRECTORY );

    offset += sizeof( ext2_dir_entry_t ) + 4;

    entry = ( ext2_dir_entry_t* )( block->block_data + offset );

    entry->rec_len = info->block_size - offset;
    ext2_dir_entry_initialize( entry, 11, "lost+found", 10, 10, EXT2_FT_DIRECTORY );

    /* Create the lost+found inode */

    error = ext2_create_inode( info, 11, &lf_inode );

    if ( error < 0 ) {
        goto error2;
    }

    lf_fs_inode = &lf_inode->fs_inode;

    lf_fs_inode->i_mode = S_IFDIR | 0777;
    lf_fs_inode->i_size = info->block_size;
    lf_fs_inode->i_links_count = 2;

    error = ext2_inode_add_block( info, lf_inode, &block );

    if ( error < 0 ) {
        goto error3;
    }

    entry = ( ext2_dir_entry_t* )block->block_data;

    entry->rec_len = sizeof( ext2_dir_entry_t ) + 4;
    ext2_dir_entry_initialize( entry, 11, ".\0\0", 1, 4, EXT2_FT_DIRECTORY );

    entry = ( ext2_dir_entry_t* )( block->block_data + sizeof( ext2_dir_entry_t ) + 4 );

    entry->rec_len = info->block_size - ( sizeof( ext2_dir_entry_t ) + 4 );
    ext2_dir_entry_initialize( entry, 2, "..\0", 2, 4, EXT2_FT_DIRECTORY );

    return 0;

 error3:
    ext2_delete_inode( info, lf_inode );

 error2:
    ext2_delete_inode( info, root_inode );

 error1:
    return -1;
}

static int ext2_write_to_disk( ext2_info_t* info ) {
    int error;
    uint32_t i;
    uint32_t j;
    uint32_t size;
    uint8_t* tmp;
    uint8_t* block;
    uint8_t* gd_block;
    uint8_t* empty_blocks;
    uint32_t first_block;
    ext2_block_t* data_block;
    ext2_inode_t* inode;

    uint32_t inode_table_blocks =
        ( info->superblock.s_inodes_per_group * sizeof( ext2_fs_inode_t ) + info->block_size - 1 ) / info->block_size;

    error = -1;

    block = ( uint8_t* )malloc( info->block_size );

    if ( block == NULL ) {
        goto error1;
    }

    gd_block = ( uint8_t* )malloc( info->block_size );

    if ( gd_block == NULL ) {
        goto error2;
    }

    empty_blocks = ( uint8_t* )malloc( 32 * info->block_size );

    if ( empty_blocks == NULL ) {
        goto error3;
    }

    /* Create group descriptors block */

    memset( gd_block, 0, info->block_size );

    for ( j = 0, tmp = gd_block; j < info->group_count; j++, tmp += sizeof( ext2_group_desc_t ) ) {
        memcpy( tmp, ( void* )&info->groups[ j ].descriptor, sizeof( ext2_group_desc_t ) );
    }

    memset( empty_blocks, 0, 32 * info->block_size );

    /* Write group informations */

    for ( i = 0; i < info->group_count; i++ ) {
        first_block = i * info->blocks_per_group;

        /* Write the superblock */

        memset( block, 0, info->block_size );

        info->superblock.s_block_group_nr = i;
        memcpy( ( void* )( block + 1024 ), ( void* )&info->superblock, sizeof( ext2_super_block_t ) );

        do_write_block( first_block )

        /* Write group descriptors */

        do_write_block2( first_block + 1, gd_block )

        /* Write block and inode bitmaps */

        do_write_block2( first_block + 2, info->groups[ i ].block_bitmap )
        do_write_block2( first_block + 3, info->groups[ i ].inode_bitmap )

        /* Empty the inode table */

        assert( ( inode_table_blocks % 32 ) == 0 );

        for ( j = 0; j < inode_table_blocks; j += 32 ) {
            do_write_block3( first_block + 4 + j, empty_blocks, 32 );
        }
    }

    /* Write inodes */

    size = ( uint32_t )array_get_size( &info->inode_list );

    for ( i = 0; i < size; i++ ) {
        uint32_t group_number;
        uint32_t inode_table_offset;

        uint32_t block_number;
        uint32_t block_offset;

        inode = ( ext2_inode_t* )array_get_item( &info->inode_list, i );

        group_number = ( inode->inode_number - 1 ) / info->superblock.s_inodes_per_group;
        inode_table_offset =
            info->groups[ group_number ].descriptor.bg_inode_table * info->block_size +
            ( ( inode->inode_number - 1 ) % info->superblock.s_inodes_per_group ) * sizeof( ext2_fs_inode_t );

        block_number = inode_table_offset / info->block_size;
        block_offset = inode_table_offset % info->block_size;

        do_read_block( block_number )

        memcpy( block + block_offset, ( void* )&inode->fs_inode, sizeof( ext2_fs_inode_t ) );

        do_write_block( block_number )
    }

    /* Write dirty blocks */

    size = ( uint32_t )array_get_size( &info->block_list );

    for ( i = 0; i < size; i++ ) {
        data_block = ( ext2_block_t* )array_get_item( &info->block_list, i );

        do_write_block2( data_block->block_number, data_block->block_data )
    }

    error = 0;

error3:
    free( gd_block );

error2:
    free( block );

error1:
    return error;
}

int ext2_finish( ext2_info_t* info ) {
    uint32_t i;
    ext2_group_t* group;

    for ( i = 0; i < info->group_count; i++ ) {
        group = &info->groups[ i ];

        free( group->inode_bitmap );
        free( group->block_bitmap );
    }

    free( info->groups );

    /* TODO: free inode and block list arrays */

    close( info->fd );

    return 0;
}

static int ext2_create( const char* device ) {
    int error;
    ext2_info_t info;

    printf( "ext2: Initializing ... " );

    error = ext2_initialize( device, &info );

    if ( error < 0 ) {
        printf( "failed. error=%d\n", error );
        return error;
    }

    printf( "done!\n" );

    printf( "ext2: Creating filesystem structure ... " );

    error = ext2_create_structure( &info );

    if ( error < 0 ) {
        printf( "failed. error=%d\n", error );
        return error;
    }

    printf( "done!\n" );

    printf( "ext2: Writing the structure to the disk ... " );

    error = ext2_write_to_disk( &info );

    if ( error < 0 ) {
        printf( "failed. error=%d\n", error );
        return error;
    }

    printf( "done!\n" );

    ext2_finish( &info );

    return 0;
}

filesystem_calls_t ext2_calls = {
    .name = "ext2",
    .create = ext2_create
};
