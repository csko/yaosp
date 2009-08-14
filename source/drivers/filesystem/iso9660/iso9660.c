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
#include <time.h>

#include "iso9660.h"
#include "rockridge.h"

#define BLOCK_SIZE 2048
#define INODE_TO_BLOCK(inode) ( inode / BLOCK_SIZE )
#define INODE_TO_OFFSET(inode) ( inode % BLOCK_SIZE )
#define POSITION_TO_INODE(block,offset) ( ( ( ino_t ) ( block * BLOCK_SIZE ) | ( ( ino_t ) offset ) ) )

static bool iso9660_probe( const char* device ) {
    int fd;
    int error;
    char* block;
    bool valid_fs;
    iso9660_volume_descriptor_t* desc;

    fd = open( device, O_RDONLY );

    if ( __unlikely( fd < 0 ) ) {
        goto error1;
    }

    block = ( char* )kmalloc( BLOCK_SIZE );

    if ( __unlikely( block == NULL ) ) {
        goto error2;
    }

    error = pread( fd, block, BLOCK_SIZE, 0x8000 );

    if ( __unlikely( error != BLOCK_SIZE ) ) {
        goto error3;
    }

    close( fd );

    desc = ( iso9660_volume_descriptor_t* )block;

    valid_fs = ( ( memcmp( desc->identifier, "CD001", 5 ) == 0 ) &&
                 ( desc->type == ISO9660_VD_PRIMARY ) );

    kfree( block );

    return valid_fs;

error3:
    kfree( block );

error2:
    close( fd );

error1:
    return false;
}

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

    block = ( char* )kmalloc( BLOCK_SIZE );

    if ( block == NULL ) {
        error = -ENOMEM;
        goto error2;
    }

    /* Read the block from the device where the iso9660
       volume descriptor should be */

    if ( pread( fd, block, BLOCK_SIZE, 0x8000 ) != BLOCK_SIZE ) {
        error = -EIO;
        goto error3;
    }

    /* Check if this is a valid iso9660 filesystem */

    root = ( iso9660_volume_descriptor_t* )block;

    if ( memcmp( root->identifier, "CD001", 5 ) != 0 ) {
        kprintf( WARNING, "iso9660: Not a valid filesystem!\n" );
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

    /* TODO: Calculate real block count here! */
    cookie->block_cache = init_block_cache( fd, BLOCK_SIZE, 1000000000ULL );

    if ( cookie->block_cache == NULL ) {
        error = -ENOMEM;
        goto error4;
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

error4:
    kfree( cookie );

error3:
    kfree( block );

error2:
    close( fd );

error1:
    return error;
}

static int iso9660_unmount( void* fs_cookie ) {
    iso9660_cookie_t* cookie;

    cookie = ( iso9660_cookie_t* )fs_cookie;

    close( cookie->fd );
    kfree( cookie );

    return 0;
}

static int iso9660_read_inode( void* fs_cookie, ino_t inode_number, void** node ) {
    iso9660_cookie_t* cookie;

    cookie = ( iso9660_cookie_t* )fs_cookie;

    /* The root inode is stored in the cookie, so we can treat
       it as a special inode. In this case we can load it simply
       with a memcpy. Otherwise we have to load it from the device. */

    if ( inode_number == cookie->root_inode_number ) {
        *node = ( void* )&cookie->root_inode;
    } else {
        tm_t tm;
        int error;
        char* block;
        uint64_t block_index;
        iso9660_inode_t* inode;
        iso9660_directory_entry_t* iso_dir;

        block_index = INODE_TO_BLOCK( inode_number );

        error = block_cache_get_block( cookie->block_cache, block_index, ( void** )&block );

        if ( error < 0 ) {
            return error;
        }

        iso_dir = ( iso9660_directory_entry_t* )( block + INODE_TO_OFFSET( inode_number ) );

        inode = ( iso9660_inode_t* )kmalloc( sizeof( iso9660_inode_t ) );

        if ( inode == NULL ) {
            block_cache_put_block( cookie->block_cache, block_index );
            return -ENOMEM;
        }

        inode->inode_number = inode_number;
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

        tm.tm_year = 1900 + iso_dir->datetime[ 0 ];
        tm.tm_mon = iso_dir->datetime[ 1 ] - 1;
        tm.tm_mday = iso_dir->datetime[ 2 ];
        tm.tm_hour = iso_dir->datetime[ 3 ];
        tm.tm_min = iso_dir->datetime[ 4 ];
        tm.tm_sec = iso_dir->datetime[ 5 ];
        /* TODO: Timezone */

        block_cache_put_block( cookie->block_cache, block_index );

        inode->created = mktime( &tm );

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

static void iso9660_parse_rr_extension( iso9660_directory_entry_t* entry, struct dirent* dir_entry ) {
    bool done;
    uint8_t* data;
    uint8_t rem_size;
    rr_header_t* header;

    int alt_name_size;

    data = ( uint8_t* )( entry + 1 );
    data += entry->name_length;

    if ( ( entry->name_length % 2 ) == 0 ) {
        data++;
    }

    done = false;
    alt_name_size = 0;

    rem_size = entry->record_length;
    rem_size -= ( ( void* )data - ( void* )entry );

    while ( ( !done ) && ( rem_size > 0 ) ) {
        header = ( rr_header_t* )data;

        switch ( header->tag ) {
            case RR_TAG_PX :
                /* Posix stat structure */
                break;

            case RR_TAG_PN :
                break;

            case RR_TAG_SL :
                /* Symbolic link information */
                break;

            case RR_TAG_NM : {
                int to_copy;
                uint8_t cur_name_size;
                rr_nm_data_t* nm_data;

                nm_data = ( rr_nm_data_t* )( header + 1 );
                cur_name_size = ( header->length - ( sizeof( rr_header_t ) + sizeof( rr_nm_data_t ) ) );

                to_copy = MIN( NAME_MAX - alt_name_size, cur_name_size );

                if ( to_copy > 0 ) {
                    memcpy( dir_entry->name + alt_name_size, nm_data + 1, cur_name_size );
                    alt_name_size += to_copy;
                }

                dir_entry->name[ alt_name_size ] = 0;

                break;
            }

            case RR_TAG_CL :
                break;

            case RR_TAG_PL :
                break;

            case RR_TAG_RE :
                /* Relocated directory */
                break;

            case RR_TAG_TF :
                break;

            case RR_TAG_RR :
                break;

            default :
                done = true;
                break;
        }

        data += header->length;
        rem_size -= header->length;
    }
}

static int iso9660_parse_directory_entry( char* buffer, struct dirent* entry ) {
    iso9660_directory_entry_t* iso_entry;

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

        memcpy( entry->name, name, length );
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

    iso9660_parse_rr_extension( iso_entry, entry );

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

    start_block = parent->start_block;
    current_block = start_block;

    error = 0;

    while ( ( current_block - start_block ) * BLOCK_SIZE < parent->length ) {
        error = block_cache_get_block( iso_cookie->block_cache, current_block, ( void** )&block );

        if ( error < 0 ) {
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

                block_cache_put_block( iso_cookie->block_cache, current_block );

                goto out;
            }

            block_position += size;
        }

        block_cache_put_block( iso_cookie->block_cache, current_block );

        current_block++;
    }

    error = -ENOENT;

out:
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

    if ( mode & O_WRONLY ) {
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
    int error;
    char* block;
    char* buffer;
    size_t saved_size;
    uint64_t block_index;
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

    block_index = node->start_block + ( pos / BLOCK_SIZE );

    /* Check if the start position is aligned to the blocksize.
       If it isn't aligned we have to read the whole block and copy
       only the interested parts of it to the destination buffer. */

    if ( ( pos % BLOCK_SIZE ) != 0 ) {
        int to_read;
        int rem_block_length;

        error = block_cache_get_block( cookie->block_cache, block_index, ( void** )&block );

        if ( error < 0 ) {
            return error;
        }

        rem_block_length = BLOCK_SIZE - ( pos % BLOCK_SIZE );
        to_read = MIN( rem_block_length, size );

        memcpy( buffer, block + pos % BLOCK_SIZE, to_read );

        block_cache_put_block( cookie->block_cache, block_index );

        block_index++;

        buffer += to_read;
        size -= to_read;
    }

    /* If the remaining size is at least one block long we
       can read full blocks to the final buffer, do it! */

    if ( size >= BLOCK_SIZE ) {
        size_t i;
        size_t block_count;

        block_count = size / BLOCK_SIZE;

        for ( i = 0; i < block_count; i++, block_index++ ) {
            error = block_cache_get_block( cookie->block_cache, block_index, ( void** )&block );

            if ( error < 0 ) {
                return error;
            }

            memcpy( buffer, block, BLOCK_SIZE );

            block_cache_put_block( cookie->block_cache, block_index );

            buffer += BLOCK_SIZE;
            size -= BLOCK_SIZE;
        }
    }

    /* Handle the case where the size is not block aligned. */

    if ( size > 0 ) {
        error = block_cache_get_block( cookie->block_cache, block_index, ( void** )&block );

        if ( error < 0 ) {
            return error;
        }

        memcpy( buffer, block, size );

        block_cache_put_block( cookie->block_cache, block_index );
    }

    return saved_size;
}

static int iso9660_read_stat( void* fs_cookie, void* _node, struct stat* stat ) {
    iso9660_inode_t* node;

    node = ( iso9660_inode_t* )_node;

    stat->st_ino = POSITION_TO_INODE( node->start_block, 0 );
    stat->st_mode = 0777;
    stat->st_size = node->length;
    stat->st_blksize = BLOCK_SIZE;
    stat->st_blocks = node->length / BLOCK_SIZE;
    stat->st_atime = stat->st_ctime = stat->st_mtime = node->created;

    if ( ( node->length % BLOCK_SIZE ) != 0 ) {
        stat->st_blocks++;
    }

    if ( node->flags & ISO9660_FLAG_DIRECTORY ) {
        stat->st_mode |= S_IFDIR;
    } else {
        stat->st_mode |= S_IFREG;
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

    while ( true ) {
        error = block_cache_get_block( iso_cookie->block_cache, dir_cookie->current_block, ( void** )&block );

        if ( error < 0 ) {
            return error;
        }

        dir_entry = ( iso9660_directory_entry_t* )( block + dir_cookie->block_position );

        if ( dir_entry->record_length == 0 ) {
            uint32_t data_size;

            block_cache_put_block( iso_cookie->block_cache, dir_cookie->current_block );

            dir_cookie->current_block++;
            dir_cookie->block_position = 0;

            /* Check if we reached the end of the directory entries */

            data_size = ( dir_cookie->current_block - dir_cookie->start_block ) * BLOCK_SIZE;

            if ( data_size >= dir_cookie->size ) {
                return 0;
            }
        } else {
            break;
        }
    }

    error = iso9660_parse_directory_entry( ( char* )dir_entry, entry );

    block_cache_put_block( iso_cookie->block_cache, dir_cookie->current_block );

    if ( error < 0 ) {
        return error;
    }

    /* Zero length directory entry marks the end of the directory */

    if ( error == 0 ) {
        return 0;
    }

    ino_t tmp_inode_number;
    iso9660_inode_t* tmp_inode;

    tmp_inode_number = POSITION_TO_INODE( dir_cookie->current_block, dir_cookie->block_position );

    iso9660_read_inode( fs_cookie, tmp_inode_number, ( void** )&tmp_inode );
    entry->inode_number = POSITION_TO_INODE( tmp_inode->start_block, 0 );
    iso9660_write_inode( fs_cookie, tmp_inode );

    dir_cookie->block_position += error;

    return 1;
}

static int iso9660_rewind_directory( void* fs_cookie, void* node, void* file_cookie ) {
    iso9660_dir_cookie_t* dir_cookie;

    dir_cookie = ( iso9660_dir_cookie_t* )file_cookie;

    dir_cookie->current_block = dir_cookie->start_block;
    dir_cookie->block_position = 0;

    return 0;
}

static filesystem_calls_t iso9660_calls = {
    .probe = iso9660_probe,
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
    .write_stat = NULL,
    .read_directory = iso9660_read_directory,
    .rewind_directory = iso9660_rewind_directory,
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

    kprintf( INFO, "iso9660: Registering filesystem driver!\n" );

    error = register_filesystem( "iso9660", &iso9660_calls );

    if ( error < 0 ) {
        kprintf( ERROR, "iso9660: Failed to register filesystem driver\n" );
        return error;
    }

    return 0;
}

int destroy_module( void ) {
    return 0;
}
