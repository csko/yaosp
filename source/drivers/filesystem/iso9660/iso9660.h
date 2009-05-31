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

#ifndef _ISO9660_H_
#define _ISO9660_H_

#include <types.h>
#include <vfs/blockcache.h>

#define ISO9660_VD_PRIMARY 0x01

#define ISO9660_FLAG_DIRECTORY 0x02

typedef struct iso9660_directory_entry {
    uint8_t record_length;
    uint8_t ext_attribute_length;
    uint32_t location_le;
    uint32_t location_be;
    uint32_t length_le;
    uint32_t length_be;
    uint8_t datetime[ 7 ];
    uint8_t flags;
    uint8_t interleaved_unit;
    uint8_t interleaved_gap;
    uint16_t seq_le;
    uint16_t seq_be;
    uint8_t name_length;
} __attribute__(( packed )) iso9660_directory_entry_t;

typedef struct iso9660_volume_descriptor {
    uint8_t type;
    uint8_t identifier[ 5 ];
    uint8_t version;
    uint8_t zero1;
    uint8_t system_identifier[ 32 ];
    uint8_t volume_identifier[ 32 ];
    uint8_t zero2[ 8 ];
    uint32_t sectors_le;
    uint32_t sectors_be;
    uint8_t zero3[ 32 ];
    uint16_t setsize_le;
    uint16_t setsize_be;
    uint16_t seq_number_le;
    uint16_t seq_number_be;
    uint16_t sector_size_le;
    uint16_t sector_size_be;
    uint8_t dunno1[ 8 ];
    uint8_t dunno2[ 4 ];
    uint8_t dunno3[ 4 ];
    uint8_t dunno4[ 4 ];
    uint8_t dunno5[ 4 ];
    uint8_t root_dir_record[ 32 ];
} __attribute__(( packed )) iso9660_volume_descriptor_t;

typedef struct iso9660_inode {
    ino_t inode_number;
    uint8_t flags;
    uint32_t start_block;
    uint32_t length;
    time_t created;
} iso9660_inode_t;

typedef struct iso9660_cookie {
    int fd;
    block_cache_t* block_cache;
    ino_t root_inode_number;
    iso9660_inode_t root_inode;
} iso9660_cookie_t;

typedef struct iso9660_dir_cookie {
    uint32_t start_block;
    uint32_t current_block;
    uint32_t block_position;
    uint32_t size;
} iso9660_dir_cookie_t;

#endif /* _ISO9660_H_ */
