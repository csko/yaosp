/* ext2 filesystem driver
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
static int ext2_calc_block_num( ext2_cookie_t *cookie, const vfs_inode_t *vinode, uint32_t block_num, uint32_t* out ) {
    uint32_t ind_block;
    uint32_t buffer[cookie->ptr_per_block];
    ext2_inode_t *inode = (ext2_inode_t*) &vinode->fs_inode;

    // direct

    if ( block_num < EXT2_NDIR_BLOCKS ) {    // if block_num in [0..11]: it is a direct block
        *out = inode->i_block[block_num];    // this points to the direct data

        return 0;
    }

    block_num -= EXT2_NDIR_BLOCKS;          // -12

    // indirect

    if ( block_num < cookie->ptr_per_block ) {     // if block_num in [12..267] it is an indirect block, rel: [0-255]
        ind_block = inode->i_block[EXT2_IND_BLOCK]; // find the indirect block

        if ( ind_block == 0 ) {
            return -EINVAL;
        }

        // read the indirect block

        if ( pread( cookie->fd, buffer, cookie->blocksize, ind_block * cookie->blocksize ) != cookie->blocksize ) {
            return -EIO;
        }

        // the element of the indirect block points to the data

        *out = buffer[ block_num ];

        return 0;
    }

    block_num -= cookie->ptr_per_block;               // -256

    // doubly-indirect

    if ( block_num < cookie->doubly_indirect_block_count ) {   // if block_num in [268..65803],  rel: [0-65535]
        ind_block = inode->i_block[EXT2_DIND_BLOCK]; // find the doubly indirect block

        if ( ind_block == 0 ) {
            return -EINVAL;
        }

        // Read in the double-indirect block

        if ( pread( cookie->fd, buffer, cookie->blocksize, ind_block * cookie->blocksize ) != cookie->blocksize ) {
            return -EIO;
        }

        ind_block = buffer[ block_num / cookie->ptr_per_block ];    // in wich indirect block? [0..255]

        if ( ind_block == 0 ) {
            return -EINVAL;
        }

        // Read the single-indirect block

        if ( pread( cookie->fd, buffer, cookie->blocksize, ind_block * cookie->blocksize ) != cookie->blocksize ) {
            return -EIO;
        }

        // find the direct block

        *out = buffer[ block_num % cookie->ptr_per_block ];         // in wich direct block? [0..255]

        return 0;

    }

    block_num -= cookie->doubly_indirect_block_count;           // -65536

    // triply-indirect

    if ( block_num < cookie->triply_indirect_block_count ) {  // [65804..16843020],  rel: [0-16777215]
        uint32_t mod;

        ind_block = inode->i_block[EXT2_TIND_BLOCK]; // find the triply indirect block

        if ( ind_block == 0 ) {
            return -EINVAL;
        }

        // Read in the triply-indirect block
        if ( pread( cookie->fd, buffer, cookie->blocksize, ind_block * cookie->blocksize ) != cookie->blocksize ) {
            return -EIO;
        }

        ind_block = buffer[ block_num / cookie->doubly_indirect_block_count ];    // in wich doubly indirect block? [0..255]

        if ( ind_block == 0 ) {
            return -EINVAL;
        }

        // Read the doubly-indirect block

        if ( pread( cookie->fd, buffer, cookie->blocksize, ind_block * cookie->blocksize ) != cookie->blocksize ) {
            return -EIO;
        }

        // find the indirect-direct block

        mod = block_num % cookie->doubly_indirect_block_count;
        ind_block = buffer[ mod / cookie->ptr_per_block ];         // in wich indirect block? [0..65536]

        // read the indirect-block

        if ( pread( cookie->fd, buffer, cookie->blocksize, ind_block * cookie->blocksize ) != cookie->blocksize ) {
            return -EIO;
        }

        *out = buffer[ mod % cookie->ptr_per_block ];

        return 0;
    }

    return -EINVAL; // block number is too large
}

/**
 * This functions reads inode structure from the file system, and wraps it into a vfs_inode
 */
static int ext2_get_inode( ext2_cookie_t *cookie, ino_t inode_number, vfs_inode_t** out) {
    vfs_inode_t *vinode;
    char buffer[ cookie->blocksize ];
    uint32_t offset, real_offset, block_offset, group, inode_table_ofs;

    if ( inode_number < EXT2_ROOT_INO ) {
        return -EINVAL;
    }

    vinode = (vfs_inode_t*) kmalloc( sizeof( vfs_inode_t ) );

    if ( vinode == NULL ) {
        return -ENOMEM;
    }

    vinode->inode_number = inode_number;

    // The s_inodes_per_group field in the superblock structure tells us how many inodes are defined per group.
    // block group = (inode - 1) / s_inodes_per_group

    group = (inode_number - 1) / cookie->super_block.s_inodes_per_group;

    // Once the block is identified, the local inode index for the local inode table can be identified using:
    // local inode index = (inode - 1) % s_inodes_per_group

    inode_table_ofs = cookie->gds[group].bg_inode_table * cookie->blocksize;

    offset = inode_table_ofs + ( ( (inode_number - 1) % cookie->super_block.s_inodes_per_group) * cookie->super_block.s_inode_size );

    real_offset = offset & ~( cookie->blocksize - 1 );
    block_offset = offset % cookie->blocksize;

    ASSERT( ( block_offset + cookie->super_block.s_inode_size ) <= cookie->blocksize );

    if ( pread( cookie->fd, buffer, cookie->blocksize, real_offset ) != cookie->blocksize ) {
        kfree( vinode );
        return -EIO;
    }

    memcpy( &vinode->fs_inode, buffer + block_offset, sizeof( ext2_inode_t ) );

    *out = vinode;

    return 0;
}

static int ext2_read_inode( void *fs_cookie, ino_t inode_number, void** out) {
    int error;
    ext2_cookie_t *cookie = (ext2_cookie_t*)fs_cookie;

    error = ext2_get_inode( cookie, inode_number, (vfs_inode_t**)out );

    if ( error < 0 ) {
        return error;
    }

    return 0;
}

static int ext2_write_inode( void* fs_cookie, void* node ) {
    kfree( node );

    return 0;
}

/**
 * Read inode data
 */
static int ext2_get_inode_data( ext2_cookie_t* cookie, const vfs_inode_t* vinode, off_t begin_offs, size_t size, void* buffer ) {
    int result;
    uint32_t offset, block_num, i;
    uint32_t leftover, start_block, end_block, trunc;

    if ( begin_offs < 0 ) {
        return -EINVAL;
    }

    if ( begin_offs >= vinode->fs_inode.i_size ) {
        return 0;
    }

    if ( begin_offs + size > vinode->fs_inode.i_size ) {
        size = vinode->fs_inode.i_size - begin_offs;
    }

    char block_buf[cookie->blocksize];
    start_block = begin_offs / cookie->blocksize;
    end_block = (begin_offs + size) / cookie->blocksize; // rest of..
    offset   = 0;

    if ( (trunc = begin_offs % cookie->blocksize) != 0 ) {
        // handle the first block

        if ( ( result = ext2_calc_block_num( cookie, vinode, start_block, &block_num ) ) < 0 ) {
            goto error1;
        }

        if ( pread( cookie->fd, block_buf, cookie->blocksize, block_num * cookie->blocksize ) != cookie->blocksize ) {
            result = -EIO;
            goto error1;
        }

        if ( sizeof(block_buf) - trunc >= size ) {
            memcpy( buffer, block_buf + trunc, size );
            offset += size;
        } else {
            memcpy( buffer, block_buf + trunc, sizeof(block_buf) - trunc );
            offset += (sizeof(block_buf) - trunc);
        }

        start_block++;
    }

    for ( i = start_block; i < end_block; ++i ) {
        if ( ( result = ext2_calc_block_num( cookie, vinode, i, &block_num ) ) < 0 ) {
            goto error1;
        }

        if ( pread( cookie->fd, (buffer + offset), cookie->blocksize, block_num * cookie->blocksize ) != cookie->blocksize ) {
            result = -EIO;
            goto error1;
        }

        offset += cookie->blocksize;
    }

    leftover = (size - offset) % cookie->blocksize;

    if ( leftover > 0 ) {   // read the rest of the data
        if ( ( result = ext2_calc_block_num( cookie, vinode, i, &block_num ) ) < 0 ) {
            goto error1;
        }

        if ( pread( cookie->fd, block_buf, sizeof(block_buf), block_num * cookie->blocksize ) != sizeof(block_buf) ) {
            result = -EIO;
            goto error1;
        }

        // copy the remainder to the buffer

        memcpy( buffer + offset, block_buf, leftover );

        offset += leftover;
    }

    return offset;

error1:
    return result;
}

static int ext2_read( void* fs_cookie, void* node, void* file_cookie, void* buffer, off_t pos, size_t size ) {
    ext2_cookie_t* cookie = ( ext2_cookie_t* )fs_cookie;
    vfs_inode_t* vinode = ( vfs_inode_t* )node;

    if ( size == 0 ) {
        return 0;
    }

    return ext2_get_inode_data( cookie, vinode, pos, size, buffer );
}

static int ext2_open_directory( vfs_inode_t* vinode, void** out ) {
    ext2_dir_cookie_t* cookie;

    cookie = ( ext2_dir_cookie_t* )kmalloc( sizeof( ext2_dir_cookie_t ) );

    if ( cookie == NULL ) {
        return -ENOMEM;
    }

    cookie->position = 0;
    cookie->dir_offset = 0;

    *out = cookie;

    return 0;
}

static int ext2_open_file( vfs_inode_t* vinode, void** out, int flags ) {
    ext2_file_cookie_t* cookie;

    cookie = ( ext2_file_cookie_t* )kmalloc( sizeof( ext2_file_cookie_t ) );

    if ( cookie == NULL ) {
        return -ENOMEM;
    }

    cookie->open_flags = flags;

    *out = cookie;

    return 0;
}

static int ext2_open( void* fs_cookie, void* node, int mode, void** file_cookie ) {
    vfs_inode_t* vinode = ( vfs_inode_t* )node;

    if ( IS_DIR( vinode )  ) {
        return ext2_open_directory( vinode, file_cookie );
    } else {
        return ext2_open_file( vinode, file_cookie, mode );
    }
}

static int ext2_read_directory( void* fs_cookie, void* node, void* file_cookie, struct dirent* direntry ) {
    ext2_dir_entry_t *entry;
    int result;

    ext2_cookie_t* cookie = ( ext2_cookie_t* )fs_cookie;
    ext2_dir_cookie_t*  dir_cookie = ( ext2_dir_cookie_t* )file_cookie;
    vfs_inode_t *vparent = (vfs_inode_t*) node;
    char data[ cookie->blocksize ];

    if ( dir_cookie->dir_offset < vparent->fs_inode.i_size ) {
        uint32_t block_num = dir_cookie->dir_offset / cookie->blocksize;

        if ( ( result = ext2_get_inode_data( cookie, vparent, block_num * cookie->blocksize, cookie->blocksize, data ) ) < 0 ) {
            return result;
        }

        entry = ( ext2_dir_entry_t* ) ( data + ( dir_cookie->dir_offset % cookie->blocksize ) ); // location of entry inside the block

        if ( entry->rec_len == 0 ) {
            // FS error
            return -EINVAL;
        }

        dir_cookie->dir_offset += entry->rec_len; // next entry
        dir_cookie->position++;

        direntry->inode_number = entry->inode;
        int nsize = entry->name_len > sizeof(direntry->name) -1 ? sizeof(direntry->name) -1 : entry->name_len;
        strncpy( direntry->name, entry->name, nsize );
        direntry->name[ nsize ] = 0;

        return 1;
    }

    return 0;
}

static int ext2_rewind_directory( void* fs_cookie, void* node, void* file_cookie ) {
    ext2_dir_cookie_t* dir_cookie = ( ext2_dir_cookie_t* )file_cookie;

    dir_cookie->position = 0;
    dir_cookie->dir_offset = 0;

    return 0;
}

static int ext2_do_lookup_inode( ext2_cookie_t *cookie, vfs_inode_t* vparent, const char* name, int name_length, ino_t *out ) {
    struct dirent dent;
    int result;
    ext2_dir_cookie_t dir_cookie;

    dir_cookie.position = 0;
    dir_cookie.dir_offset = 0;

    if ( !IS_DIR( vparent ) ) {
        return -EINVAL;
    }

    result = ext2_read_directory( cookie, vparent, &dir_cookie, &dent );

    while ( result > 0 ) {
        if ( ( strlen( dent.name ) == name_length ) &&
             ( strncmp( dent.name, name, name_length ) == 0 ) ) {
            *out = dent.inode_number;
            return 0;
        }

        result = ext2_read_directory( cookie, vparent, &dir_cookie, &dent );
    }

    return -ENOENT;
}

static int ext2_lookup_inode( void* fs_cookie, void* _parent, const char* name, int name_length, ino_t* out ) {
    int error;

    ext2_cookie_t* cookie = ( ext2_cookie_t* )fs_cookie;
    vfs_inode_t* vparent  = ( vfs_inode_t* )_parent;

    error = ext2_do_lookup_inode( cookie, vparent, name, name_length, out );

    if ( error < 0 ) {
        return error;
    }

    return 0;
}

static int ext2_read_stat( void* fs_cookie, void* node, struct stat* stat ) {
    ext2_cookie_t* cookie = ( ext2_cookie_t* )fs_cookie;
    vfs_inode_t* vinode = ( vfs_inode_t* )node;

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

    return 0;
}

static int ext2_free_cookie( void* fs_cookie, void* node, void* file_cookie ) {
    kfree( file_cookie );

    return 0;
}

int ext2_mount( const char* device, uint32_t flags, void** fs_cookie, ino_t* root_inode_number ) {
    uint32_t sb_offset = 1 * EXT2_MIN_BLOCK_SIZE; // offset of the superblock (1024)
    int result;
    ext2_cookie_t *cookie;
    ext2_group_desc_t *gds;

    cookie = (ext2_cookie_t*) kmalloc( sizeof( ext2_cookie_t ) );

    if ( cookie == NULL ) {
        return -ENOMEM;
    }

    cookie->flags = flags;
    cookie->fd = open( device, O_RDONLY );

    if ( cookie->fd < 0 ) {
        result = cookie->fd;
        goto error1;
    }

    // read the superblock

    if ( pread( cookie->fd, &cookie->super_block, sizeof( ext2_super_block_t ), sb_offset ) != sizeof( ext2_super_block_t ) ) {
        result = -EIO;
        goto error2;
    }

    // validate superblock

    if ( cookie->super_block.s_magic != EXT2_SUPER_MAGIC) {
        kprintf("ext2: bad magic number: 0x%x\n", cookie->super_block.s_magic);
        result = -EINVAL;
        goto error2;
    }

    // check fs state

    if ( cookie->super_block.s_state != EXT2_VALID_FS ) {
        kprintf( "ext2: partition is damaged or was not cleanly unmounted!\n" );
        result = -EINVAL;
        goto error2;
    }


    cookie->ngroups = (cookie->super_block.s_blocks_count - cookie->super_block.s_first_data_block +
        cookie->super_block.s_blocks_per_group - 1) / cookie->super_block.s_blocks_per_group;

    cookie->blocksize = EXT2_MIN_BLOCK_SIZE << cookie->super_block.s_log_block_size;
    uint32_t ppb = cookie->blocksize / sizeof( uint32_t );
    cookie->ptr_per_block = ppb;
    cookie->doubly_indirect_block_count = ppb * ppb;
    cookie->triply_indirect_block_count = cookie->doubly_indirect_block_count * ppb;

    uint32_t gd_offs = (EXT2_MIN_BLOCK_SIZE / cookie->blocksize +1) * cookie->blocksize;

    // allocate group descriptor array
    int gds_size = cookie->ngroups * sizeof( ext2_group_desc_t );
    gds = (ext2_group_desc_t*) kmalloc( gds_size );

    if ( gds == NULL ) {
        result = -ENOMEM;
        goto error2;
    }

    // read the group descriptors

    if ( pread( cookie->fd, gds, gds_size, gd_offs ) != gds_size ) {
        result = -EIO;
        goto error3;
    }

    cookie->gds = gds;

    // increase mount count and mark fs in use only in RW mode

    if ( cookie->flags & ~MOUNT_RO ) {
        cookie->super_block.s_state = EXT2_ERROR_FS;
        cookie->super_block.s_mnt_count++;

        if ( pwrite( cookie->fd, &cookie->super_block, sizeof( ext2_super_block_t ), sb_offset ) != sizeof( ext2_super_block_t ) ){
            kprintf( "ext2: failed to write back superblock\n" );
            result = -EIO;
            goto error3;
        }
    }

    *root_inode_number = EXT2_ROOT_INO;
    *fs_cookie = (void*) cookie;

    kprintf("ext2: mount OK\n");

    return 0;

 error3:
    kfree( gds );

 error2:
    close( cookie->fd );

 error1:
    kfree( cookie );

    return result;
}

static int ext2_unmount( void* fs_cookie ) {
    ext2_cookie_t *cookie = (ext2_cookie_t*)fs_cookie;

    // mark filesystem as it is not more in use

    if ( cookie->flags & ~MOUNT_RO ) {
        cookie->super_block.s_state = EXT2_VALID_FS;

        if ( pwrite( cookie->fd, &cookie->super_block, sizeof( ext2_super_block_t ), 1 * EXT2_MIN_BLOCK_SIZE ) != sizeof( ext2_super_block_t ) ){
            kprintf( "ext2: failed to write back superblock\n" );
            return -EIO;
        }
    }

    // TODO

    return -ENOSYS;
}

static filesystem_calls_t ext2_calls = {
    .probe = NULL,
    .mount = ext2_mount,
    .unmount = ext2_unmount,
    .read_inode = ext2_read_inode,
    .write_inode = ext2_write_inode,
    .lookup_inode = ext2_lookup_inode,
    .open = ext2_open,
    .close = NULL,
    .free_cookie = ext2_free_cookie,
    .read = ext2_read,
    .write = NULL,
    .ioctl = NULL,
    .read_stat = ext2_read_stat,
    .write_stat = NULL,
    .read_directory = ext2_read_directory,
    .rewind_directory = ext2_rewind_directory,
    .create = NULL,
    .unlink = NULL,
    .mkdir = NULL,
    .rmdir = NULL,
    .isatty = NULL,
    .symlink = NULL,
    .readlink = NULL,
    .set_flags = NULL,
    .add_select_request = NULL,
    .remove_select_request = NULL
};

int init_module( void ) {
    int error;

    kprintf( "ext2: Registering filesystem driver\n" );

    error = register_filesystem( "ext2", &ext2_calls );

    if ( error < 0 ) {
        return error;
    }

    return 0;
}

int destroy_module( void ) {
    return 0;
}
