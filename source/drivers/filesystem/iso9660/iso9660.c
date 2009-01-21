/* ISO9660 filesystem driver
 *
 * Copyright (c) 2009 Zoltan Kovacs, Kornel Csernai
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

#include <console.h>
#include <errno.h>
#include <macros.h>
#include <mm/kmalloc.h>
#include <vfs/filesystem.h>
#include <vfs/vfs.h>
#include <lib/string.h>
#include <lib/ctype.h>
#include <time.h>

#include "iso9660.h"

#define INODE_TO_BLOCK(inode) (inode/2048)
#define INODE_TO_OFFSET(inode) (inode%2048)
#define POSITION_TO_INODE(block,offset) (((ino_t)(block*2048)|((ino_t)offset)))

static int iso9660_read_directory( void* fs_cookie, void* node, void* file_cookie, struct dirent* entry );

static int iso9660_mount( const char* _device, uint32_t flags, void** fs_cookie, ino_t* root_inode_num ) {
    int fd;
    int error;
    char* block;
    iso9660_cookie_t* cookie;
    iso9660_volume_descriptor_t* root;
    iso9660_directory_entry_t* root_entry;

    /* Open the device */

    fd = open( _device, O_RDONLY );

    if ( fd < 0 ) {
        error = fd;
        goto error1;
    }

    /* Allocate memory for a block */

    block = ( char* )kmalloc( 2048 );

    if ( block == NULL ) {
        error = -ENOMEM;
        goto error2;
    }

    /* Read the block from the device where the iso9660
       volume descriptor should be */

    if ( pread( fd, block, 2048, 0x8000 ) != 2048 ) {
        kprintf( "ISO9660: Failed to read root block\n" );
        error = -EIO;
        goto error3;
    }

    /* Check if this is a valid iso9660 filesystem */

    root = ( iso9660_volume_descriptor_t* )block;

    if ( memcmp( root->identifier, "CD001", 5 ) != 0 ) {
        kprintf( "ISO9660: Not a valid filesystem!\n" );
        error = -EINVAL;
        goto error3;
    }

    if ( root->type != ISO9660_VD_PRIMARY ) {
        error = -EINVAL;
        goto error3;
    }

    /* Create the cookie for the filesystem and parse the
       root node in the filesystem */

    root_entry = ( iso9660_directory_entry_t* )&root->root_dir_record[ 0 ];

    cookie = ( iso9660_cookie_t* )kmalloc( sizeof( iso9660_cookie_t ) );

    if ( cookie == NULL ) {
        error = -ENOMEM;
        goto error3;
    }

    cookie->fd = fd;
    cookie->root_inode_number = POSITION_TO_INODE(root_entry->location_le,0);
    cookie->root_inode.inode_number = cookie->root_inode_number;
    cookie->root_inode.flags = root_entry->flags;
    cookie->root_inode.start_block = root_entry->location_le;
    cookie->root_inode.length = root_entry->length_le;
    /* TODO: Maybe change this to the datetime field of the CD? */
    cookie->root_inode.created = time( NULL );

    /* Free the allocated block memory */

    kfree( block );

    /* Pass our pointers to the VFS */

    *fs_cookie = ( void* )cookie;
    *root_inode_num = cookie->root_inode_number;

    return 0;

error3:
    kfree( block );

error2:
    close( fd );

error1:
    return error;
}

static int iso9660_unmount( void* fs_cookie ) {
    /* TODO */

    return 0;
}

static int iso9660_read_inode( void* fs_cookie, ino_t inode_num, void** node ) {
    iso9660_cookie_t* cookie;

    cookie = ( iso9660_cookie_t* )fs_cookie;

    /* The root inode is stored in the cookie, so we can treat
       it as a special inode. In this case we can load it simply
       with a memcpy. Otherwise we have to load it from the device. */

    if ( inode_num == cookie->root_inode_number ) {
        *node = ( void* )&cookie->root_inode;
    } else {
        char* block;
        iso9660_inode_t* inode;
        iso9660_directory_entry_t* iso_dir;

        block = ( char* )kmalloc( 2048 );

        if ( block == NULL ) {
            return -ENOMEM;
        }

        if ( pread(
            cookie->fd,
            block,
            2048,
            INODE_TO_BLOCK(inode_num) * 2048
        ) != 2048 ) {
            kfree( block );
            return -EIO;
        }

        iso_dir = ( iso9660_directory_entry_t* )( block + INODE_TO_OFFSET(inode_num) );

        inode = ( iso9660_inode_t* )kmalloc( sizeof( iso9660_inode_t ) );

        if ( inode == NULL ) {
            kfree( block );
            return -ENOMEM;
        }

        inode->inode_number = inode_num;
        inode->flags = iso_dir->flags;
        inode->start_block = iso_dir->location_le;
        inode->length = iso_dir->length_le;

        /* datetime is made up of:
        0 number of years since 1900
        1 month, 1=January, 2=February, ...
        2 day of month, ranging 1 to 31
        3 hour, ranging 0 to 23
        4 minute, ranging 0 to 59
        5 second, ranging 0 to 59
        6 offset from Greenwich Mean Time, in 15-minute intervals,
          as a twos complement signed number, positive for time
          zones east of Greenwich, and negative for time zones
          west of Greenwich
        */

        tm_t tm;
        tm.year = 1900 + iso_dir->datetime[ 0 ];
        tm.mon = iso_dir->datetime[ 1 ] - 1;
        tm.mday = iso_dir->datetime[ 2 ];
        tm.hour = iso_dir->datetime[ 3 ];
        tm.min = iso_dir->datetime[ 4 ];
        tm.sec = iso_dir->datetime[ 5 ];
        /* TODO: Timezone */

        inode->created = mktime( &tm );

        kfree( block );

        *node = ( void* )inode;
    }

    return 0;
}

static int iso9660_write_inode( void* fs_cookie, void* node ) {
    iso9660_cookie_t* cookie;

    cookie = ( iso9660_cookie_t* )fs_cookie;

    /* Free the node data only if it isn't the root node */

    if ( node != &cookie->root_inode ) {
        kfree( node );
    }

    return 0;
}

static int iso9660_parse_directory_entry( char* buffer, struct dirent* entry ) {
    iso9660_directory_entry_t* iso_entry;
    int i;

    iso_entry = ( iso9660_directory_entry_t* )buffer;

    if ( ( iso_entry->name_length == 1 ) &&
         ( ( ( char* )( iso_entry + 1 ) )[ 0 ] == 0 ) ) {
        memcpy( entry->name, ".", 2 );
    } else if ( ( iso_entry->name_length == 1 ) &&
                ( ( ( char* )( iso_entry + 1 ) )[ 0 ] == 1 ) ) {
        memcpy( entry->name, "..", 3 );
    } else {
        int length;
        char* semicolon;
        char* name;

        length = MIN( NAME_MAX, iso_entry->name_length );

        name = ( char* )( iso_entry + 1 );
        for(i = 0; i < length; i++ ){
            entry->name[ i ] = tolower( name [ i ] );
        }
        entry->name[ length ] = 0;

        /* Cut the semicolon from the end of the filename if exists */

        semicolon = strchr( entry->name, ';' );

        if ( semicolon != NULL ) {
            *semicolon = 0;
        }

        /* Dunno why filenames are ended with a dot. At the moment
           we just remove it, but we should really investigate this. */

        if ( entry->name[ strlen( entry->name ) - 1 ] == '.' ) {
            entry->name[ strlen( entry->name ) - 1 ] = 0;
        }
    }

    return iso_entry->record_length;
}

static int iso9660_lookup_inode( void* fs_cookie, void* _parent, const char* name, int name_length, ino_t* inode_num ) {
    int error;
    char* block;
    iso9660_inode_t* parent;
    iso9660_cookie_t* iso_cookie;

    uint32_t start_block;
    uint32_t current_block;
    uint32_t block_position;

    struct dirent dir_entry;

    parent = ( iso9660_inode_t* )_parent;
    iso_cookie = ( iso9660_cookie_t* )fs_cookie;

    block = ( char* )kmalloc( 2048 );

    if ( block == NULL ) {
        return -ENOMEM;
    }

    start_block = parent->start_block;
    current_block = start_block;

    error = 0;

    while ( ( current_block - start_block ) * 2048 < parent->length ) {
        if ( pread( iso_cookie->fd, block, 2048, current_block * 2048 ) != 2048 ) {
            error = -EIO;
            goto out;
        }

        block_position = 0;

        while ( 1 ) {
            int size;
            iso9660_directory_entry_t* iso_dir_entry;

            iso_dir_entry = ( iso9660_directory_entry_t* )( block + block_position );
            size = iso9660_parse_directory_entry( block + block_position, &dir_entry );

            if ( size == 0 ) {
                break;
            }

            if ( ( strlen( dir_entry.name ) == name_length ) &&
                 ( strncmp( dir_entry.name, name, name_length ) == 0 ) ) {
                if ( ( name_length == 2 ) &&
                     ( strncmp( name, "..", 2 ) == 0 ) ) {
                    if ( iso_dir_entry->location_le == iso_cookie->root_inode.start_block ) {
                        *inode_num = iso_cookie->root_inode_number;
                    } else {
                        *inode_num = POSITION_TO_INODE( iso_dir_entry->location_le, 0 );
                    }
                } else {
                    *inode_num = POSITION_TO_INODE( current_block, block_position );
                }

                goto out;
            }

            block_position += size;
        }

        current_block++;
    }

    error = -ENOENT;

out:
    kfree( block );

    return error;
}

static int iso9660_open_directory( iso9660_inode_t* node, void** _cookie ) {
    iso9660_dir_cookie_t* cookie;

    cookie = ( iso9660_dir_cookie_t* )kmalloc( sizeof( iso9660_dir_cookie_t ) );

    if ( cookie == NULL ) {
        return -ENOMEM;
    }

    cookie->start_block = node->start_block;
    cookie->current_block = cookie->start_block;
    cookie->block_position = 0;
    cookie->size = node->length;

    *_cookie = cookie;

    return 0;
}

static int iso9660_open( void* fs_cookie, void* _node, int mode, void** file_cookie ) {
    int error;
    iso9660_inode_t* node;

    if ( ( mode & O_WRONLY ) ||
         ( mode & O_RDWR ) ) {
        return -EROFS;
    }

    node = ( iso9660_inode_t* )_node;

    if ( node->flags & ISO9660_FLAG_DIRECTORY ) {
        error = iso9660_open_directory( node, file_cookie );
    } else {
        error = 0;
    }

    return error;
}

static int iso9660_close( void* fs_cookie, void* node, void* file_cookie ) {
    return 0;
}

static int iso9660_free_cookie( void* fs_cookie, void* _node, void* file_cookie ) {
    iso9660_inode_t* node;

    node = ( iso9660_inode_t* )_node;

    if ( node->flags & ISO9660_FLAG_DIRECTORY ) {
        kfree( file_cookie );
    }

    return 0;
}

static int iso9660_read( void* fs_cookie, void* _node, void* file_cookie, void* _buffer, off_t pos, size_t size ) {
    char* buffer;
    size_t saved_size;
    off_t current_pos = pos;
    iso9660_cookie_t* cookie;
    iso9660_inode_t* node;

    cookie = ( iso9660_cookie_t* )fs_cookie;
    node = ( iso9660_inode_t* )_node;

    /* Reading directories is not allowed, use read_directory() */

    if ( node->flags & ISO9660_FLAG_DIRECTORY ) {
        return -EINVAL;
    }

    /* Check the position */

    if ( pos >= node->length ) {
        return 0;
    }

    /* Check the size */

    if ( ( pos + size ) > node->length ) {
        size = node->length - pos;
    }

    buffer = ( char* )_buffer;
    saved_size = size;

    /* Check if the start position is aligned to the blocksize.
       If it isn't aligned we have to read the whole block and copy
       only the interested parts of it to the destination buffer. */

    if ( ( pos % 2048 ) != 0 ) {
        char* block;
        int to_read;
        int rem_block_length;

        block = ( char* )kmalloc( 2048 );

        if ( block == NULL ) {
            return -ENOMEM;
        }

        if ( pread(
            cookie->fd,
            block,
            2048,
            node->start_block * 2048 + ( pos & ~2047 )
        ) != 2048 ) {
            kfree( block );
            return -EIO;
        }

        rem_block_length = 2048 - ( pos % 2048 );

        to_read = MIN( rem_block_length, size );

        memcpy( buffer, block + pos % 2048, to_read );

        kfree( block );

        current_pos = ( ( pos + 2047 ) & ~2047 );

        buffer += to_read;
        size -= to_read;
    }

    /* If the remaining size is at least one block long we
       can read full blocks to the final buffer, do it! */

    if ( size >= 2048 ) {
        int to_read;

        to_read = size & ~2047;

        if ( pread(
            cookie->fd,
            buffer,
            to_read,
            node->start_block * 2048 + ( ( pos + 2047 ) & ~2047 )
        ) != to_read ) {
            return -EIO;
        }

        current_pos = ( ( pos + 2047 ) & ~2047 ) + ( size & ~2047 );

        buffer += to_read;
        size -= to_read;
    }

    /* Handle the case where the size is not block aligned. */

    if ( size > 0 ) {
        char* block;

        block = ( char* )kmalloc( 2048 );

        if ( block == NULL ) {
            return -ENOMEM;
        }

        if ( pread(
            cookie->fd,
            block,
            2048,
            node->start_block * 2048 + current_pos
        ) != 2048 ) {
            kfree( block );
            return -EIO;
        }

        memcpy( buffer, block, size );

        kfree( block );
    }

    return saved_size;
}

static int iso9660_read_stat( void* fs_cookie, void* _node, struct stat* stat ) {
    iso9660_inode_t* node;

    node = ( iso9660_inode_t* )_node;

    stat->st_ino = POSITION_TO_INODE( node->start_block, 0 );
    stat->st_mode = 0;
    stat->st_size = node->length;
    stat->st_blksize = 2048;
    stat->st_blocks = node->length / 2048;
    stat->st_atime = stat->st_ctime = stat->st_mtime = node->created;

    if ( ( node->length % 2048 ) != 0 ) {
        stat->st_blocks++;
    }

    if ( node->flags & ISO9660_FLAG_DIRECTORY ) {
        stat->st_mode |= S_IFDIR;
    }

    return 0;
}

static int iso9660_read_directory( void* fs_cookie, void* node, void* file_cookie, struct dirent* entry ) {
    int error;
    char* block;
    iso9660_cookie_t* iso_cookie;
    iso9660_dir_cookie_t* dir_cookie;
    iso9660_directory_entry_t* dir_entry;

    iso_cookie = ( iso9660_cookie_t* )fs_cookie;
    dir_cookie = ( iso9660_dir_cookie_t* )file_cookie;

    block = ( char* )kmalloc( 2048 );

    if ( block == NULL ) {
        return -ENOMEM;
    }

    while ( true ) {
        if ( pread( iso_cookie->fd, block, 2048, dir_cookie->current_block * 2048 ) != 2048 ) {
            kfree( block );
            return -EIO;
        }

        dir_entry = ( iso9660_directory_entry_t* )( block + dir_cookie->block_position );

        if ( dir_entry->record_length == 0 ) {
            uint32_t data_size;

            dir_cookie->current_block++;
            dir_cookie->block_position = 0;

            /* Check if we reached the end of the directory entries */

            data_size = ( dir_cookie->current_block - dir_cookie->start_block ) * 2048;

            if ( data_size >= dir_cookie->size ) {
                kfree( block );
                return 0;
            }
        } else {
            break;
        }
    }

    error = iso9660_parse_directory_entry( block + dir_cookie->block_position, entry );

    kfree( block );

    if ( error < 0 ) {
        return error;
    }

    /* Zero length directory entry marks the end of the directory */

    if ( error == 0 ) {
        return 0;
    }

    if ( dir_entry->location_le == iso_cookie->root_inode.start_block ) {
        entry->inode_number = iso_cookie->root_inode.inode_number;
    } else {
        entry->inode_number = POSITION_TO_INODE( dir_entry->location_le, 0 );
    }

    dir_cookie->block_position += error;

    return 1;
}

static filesystem_calls_t iso9660_calls = {
    .probe = NULL,
    .mount = iso9660_mount,
    .unmount = iso9660_unmount,
    .read_inode = iso9660_read_inode,
    .write_inode = iso9660_write_inode,
    .lookup_inode = iso9660_lookup_inode,
    .open = iso9660_open,
    .close = iso9660_close,
    .free_cookie = iso9660_free_cookie,
    .read = iso9660_read,
    .write = NULL,
    .ioctl = NULL,
    .read_stat = iso9660_read_stat,
    .read_directory = iso9660_read_directory,
    .create = NULL,
    .mkdir = NULL,
    .isatty = NULL,
    .add_select_request = NULL,
    .remove_select_request = NULL
};

int init_module( void ) {
    int error;

    kprintf( "ISO9660: Registering filesystem driver!\n" );

    error = register_filesystem( "iso9660", &iso9660_calls );

    if ( error < 0 ) {
        kprintf( "ISO9660: Failed to register filesystem driver\n" );
        return error;
    }

    return 0;
}

int destroy_module( void ) {
    return 0;
}
