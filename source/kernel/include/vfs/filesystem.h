/* Filesystem definitions
 *
 * Copyright (c) 2008 Zoltan Kovacs
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

#ifndef _VFS_FILESYSTEM_H_
#define _VFS_FILESYSTEM_H_

#include <types.h>

#include <vfs/inode.h>

struct dirent;

typedef struct filesystem_calls {
    bool ( *probe )( const char* device );
    int ( *mount )( const char* device, uint32_t flags, void** fs_cookie, ino_t* root_inode_num );
    int ( *unmount )( void* fs_cookie );
    int ( *read_inode )( void* fs_cookie, ino_t inode_num, void** node );
    int ( *write_inode )( void* fs_cookie, void* node );
    int ( *lookup_inode )( void* fs_cookie, void* parent, const char* name, int name_len, ino_t* inode_num );
    int ( *open )( void* fs_cookie, void* node, int mode, void** file_cookie );
    int ( *close )( void* fs_cookie, void* node, void* file_cookie );
    int ( *free_cookie )( void* fs_cookie, void* node, void* file_cookie );
    int ( *read )( void* fs_cookie, void* node, void* file_cookie, void* buffer, off_t pos, size_t size );
    int ( *write )( void* fs_cookie, void* node, void* file_cookie, const void* buffer, off_t pos, size_t size );
    int ( *read_directory )( void* fs_cookie, void* node, void* file_cookie, struct dirent* entry );
} filesystem_calls_t;

#endif // _VFS_FILESYSTEM_H_
