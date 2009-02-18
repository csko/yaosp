/* Filesystem definitions
 *
 * Copyright (c) 2008 Zoltan Kovacs
 * Copyright (c) 2009 Kornel Csernai
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
#include <lib/hashtable.h>

#define MOUNT_NONE    0
#define MOUNT_RO      1

struct dirent;
struct select_request;
struct stat;

typedef struct filesystem_calls {
    bool ( *probe )( const char* device );
    int ( *mount )( const char* device, uint32_t flags, void** fs_cookie, ino_t* root_inode_number );
    int ( *unmount )( void* fs_cookie );
    int ( *read_inode )( void* fs_cookie, ino_t inode_number, void** node );
    int ( *write_inode )( void* fs_cookie, void* node );
    int ( *lookup_inode )( void* fs_cookie, void* parent, const char* name, int name_length, ino_t* inode_number );
    int ( *open )( void* fs_cookie, void* node, int mode, void** file_cookie );
    int ( *close )( void* fs_cookie, void* node, void* file_cookie );
    int ( *free_cookie )( void* fs_cookie, void* node, void* file_cookie );
    int ( *read )( void* fs_cookie, void* node, void* file_cookie, void* buffer, off_t pos, size_t size );
    int ( *write )( void* fs_cookie, void* node, void* file_cookie, const void* buffer, off_t pos, size_t size );
    int ( *ioctl )( void* fs_cookie, void* node, void* file_cookie, int command, void* buffer, bool from_kernel );
    int ( *read_stat )( void* fs_cookie, void* node, struct stat* stat );
    int ( *write_stat )( void* fs_cookie, void* node, struct stat* stat, uint32_t mask );
    int ( *read_directory )( void* fs_cookie, void* node, void* file_cookie, struct dirent* entry );
    int ( *rewind_directory )( void* fs_cookie, void* node, void* file_cookie );
    int ( *create )( void* fs_cookie, void* node, const char* name, int name_length, int mode, int perms, ino_t* inode_num, void** file_cookie );
    int ( *unlink )( void* fs_cookie, void* node, const char* name, int name_length );
    int ( *mkdir )( void* fs_cookie, void* node, const char* name, int name_length, int perms );
    int ( *rmdir )( void* fs_cookie, void* node, const char* name, int name_length );
    int ( *isatty )( void* fs_cookie, void* node );
    int ( *symlink )( void* fs_cookie, void* node, const char* name, int name_length, const char* link_path );
    int ( *readlink )( void* fs_cookie, void* node, char* buffer, size_t length );
    int ( *add_select_request )( void* fs_cookie, void* node, void* file_cookie, struct select_request* request );
    int ( *remove_select_request )( void* fs_cookie, void* node, void* file_cookie, struct select_request* request );
} filesystem_calls_t;

typedef struct filesystem_descriptor {
    hashitem_t hash;
    char* name;
    filesystem_calls_t* calls;
} filesystem_descriptor_t;

int register_filesystem( const char* name, filesystem_calls_t* calls );
filesystem_descriptor_t* get_filesystem( const char* name );

int init_filesystems( void );

#endif // _VFS_FILESYSTEM_H_
