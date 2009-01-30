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
#include <console.h>
#include <mm/kmalloc.h>
#include <vfs/filesystem.h>
#include <vfs/vfs.h>

#include "ext2.h"

static int ext2_mount( const char* device, uint32_t flags, void** fs_cookie, ino_t* root_inode_num ) {
    int error;
    ext2_cookie_t* cookie;

    cookie = ( ext2_cookie_t* )kmalloc( sizeof( ext2_cookie_t ) );

    if ( cookie == NULL ) {
        error = -ENOMEM;
        goto error1;
    }

    cookie->fd = open( device, O_RDONLY );

    if ( cookie->fd < 0 ) {
        error = cookie->fd;
        goto error2;
    }

    if ( pread( cookie->fd, &cookie->superblock, sizeof( ext2_superblock_t ), 1024 ) != sizeof( ext2_superblock_t ) ) {
        error = -EIO;
        goto error3;
    }

    if ( cookie->superblock.magic != EXT2_MAGIC ) {
        kprintf( "ext2: Invalid magic value: %x\n", cookie->superblock.magic );
        goto error3;
    }

    return 0;

error3:
    close( cookie->fd );

error2:
    kfree( cookie );

error1:
    return error;
}

static int ext2_unmount( void* fs_cookie ) {
    return -ENOSYS;
}

static int ext2_read_inode( void* fs_cookie, ino_t inode_num, void** node ) {
    return -ENOSYS;
}

static int ext2_write_inode( void* fs_cookie, void* node ) {
    return -ENOSYS;
}

static int ext2_lookup_inode( void* fs_cookie, void* parent, const char* name, int name_len, ino_t* inode_num ) {
    return -ENOENT;
}

static filesystem_calls_t ext2_calls = {
    .probe = NULL,
    .mount = ext2_mount,
    .unmount = ext2_unmount,
    .read_inode = ext2_read_inode,
    .write_inode = ext2_write_inode,
    .lookup_inode = ext2_lookup_inode,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
};

int init_module( void ) {
    int error;

    error = register_filesystem( "ext2", &ext2_calls );

    if ( error < 0 ) {
        return error;
    }

    return 0;
}

int destroy_module( void ) {
    return 0;
}
