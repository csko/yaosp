/* ext2 filesystem driver
 *
 * Copyright (c) 2009 Kornel Csernai, Zoltan Kovacs
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

#ifndef _EXT2_H_
#define _EXT2_H_

#include <types.h>

#define EXT2_NAME_MAX 255
#define EXT2_SB_SIZE sizeof( ext2_superblock_t )
#define EXT2_MAGIC 0xEF53
#define EXT2_BLOCK_NUM 15

/* Inode flags */
#define EXT2_SECRM_FL           0x00000001
#define EXT2_UNRM_FL            0x00000002
#define EXT2_COMPR_FL           0x00000004
#define EXT2_SYNC_FL            0x00000008
#define EXT2_IMMUTABLE_FL       0x00000010
#define EXT2_APPEND_FL          0x00000020
#define EXT2_NODUMP_FL          0x00000040
#define EXT2_NOATIME_FL         0x00000080

#define EXT2_DIRTY_FL           0x00000100
#define EXT2_COMPRBLK_FL        0x00000200
#define EXT2_NOCOMP_FL          0x00000400
#define EXT2_ECOMPR_FL          0x00000800

#define EXT2_BTREE_FL           0x00001000
#define EXT2_RESERVED_FL        0x80000000

#define EXT2_FL_USER_VISIBLE        0x00001FFF
#define EXT2_FL_USER_MODIFIABLE     0x000000FF


/* Mount options */
#define EXT2_MOUNT_CHECK            0x0001
#define EXT2_MOUNT_GRPID            0x0004
#define EXT2_MOUNT_DEBUG            0x0008
#define EXT2_MOUNT_ERRORS_CONT      0x0010
#define EXT2_MOUNT_ERRORS_RO        0x0020
#define EXT2_MOUNT_ERRORS_PANIC     0x0040
#define EXT2_MOUNT_MINIX_DF         0x0080
#define EXT2_MOUNT_NO_UID32         0x0200

/* FS states */
#define EXT2_VALID_FS           0x0001  /* Unmounted cleanly */
#define EXT2_ERROR_FS           0x0002  /* Errors detected */

/* File types */
enum {
    EXT2_FT_UNKNOWN,
    EXT2_FT_REG_FILE,
    EXT2_FT_DIR,
    EXT2_FT_CHRDEV,
    EXT2_FT_BLKDEV,
    EXT2_FT_FIFO,
    EXT2_FT_SOCK,
    EXT2_FT_SYMLINK,
    EXT2_FT_MAX
};

typedef struct ext2_superblock {
    uint32_t inodes_count;
    uint32_t blocks_count;
    uint32_t r_blocks_count;
    uint32_t free_blocks_count;
    uint32_t free_inodes_count;
    uint32_t first_data_block;
    uint32_t log_block_size;
    uint32_t log_frag_size;
    uint32_t blocks_per_group;
    uint32_t frags_per_group;
    uint32_t inodes_per_group;
    uint32_t mtime;
    uint32_t wtime;
    uint16_t mnt_count;
    uint16_t max_mnt_count;
    uint16_t magic;
    uint16_t state;
    uint16_t errors;
    uint16_t minor_rev_level;
    uint32_t lastcheck;
    uint32_t checkinterval;
    uint32_t creator_os;
    uint32_t rev_level;
    uint16_t def_resuid;
    uint16_t def_resgid;
    uint32_t first_ino;
    uint16_t inode_size;
    uint16_t block_group_nr;
    uint32_t feature_compat;
    uint32_t feature_incompat;
    uint32_t feature_ro_compat;
    uint64_t uuid[ 2 ];
    char volume_name[ 16 ];
    char last_mounted[ 64 ];
    uint32_t algo_bitmap;
    uint8_t prealloc_blocks;
    uint8_t prealloc_dir_blocks;
    uint16_t alignment;
    char journal_uuid[ 16 ];
    uint32_t journal_inum;
    uint32_t journal_dev;
    uint32_t last_orphan;
    char padding[ 788 ];
} __attribute__(( packed )) ext2_superblock_t;

typedef struct osd2 {
    uint64_t dummy;
    uint32_t dummy2;
} __attribute__(( packed )) osd2_t;

typedef struct ext2_inode {
    uint16_t i_mode;
    uint16_t i_uid;
    uint32_t i_size;
    uint32_t i_atime;
    uint32_t i_ctime;
    uint32_t i_mtime;
    uint32_t i_dtime;
    uint16_t i_gid;
    uint16_t i_links_count;
    uint32_t i_blocks;
    uint32_t i_flags;
    uint32_t i_osd1;
    uint32_t i_block[EXT2_BLOCK_NUM];
    uint32_t i_generation;
    uint32_t i_file_acl;
    uint32_t i_dir_acl;
    uint32_t i_faddr;
    osd2_t i_osd2;
} __attribute__(( packed )) ext2_inode_t;

typedef struct ext2_dir_entry {
    uint32_t inode;
    uint16_t rec_len;
    uint8_t name_len;
    uint8_t file_type;
    char name[ EXT2_NAME_MAX ];
} __attribute__(( packed )) ext2_dirent_t;

typedef struct ext2_group_desc {
    uint32_t bg_block_bitmap;
    uint32_t bg_inode_bitmap;
    uint32_t bg_inode_table;
    uint16_t bg_free_blocks_count;
    uint16_t bg_free_inodes_count;
    uint16_t bg_used_dirs_count;
    uint16_t bg_pad;
    uint32_t bg_reserved[3];
} __attribute__(( packed )) ext2_group_desc_t;

typedef struct ext2_cookie {
    int fd;
    ext2_superblock_t superblock;
} ext2_cookie_t;

#endif // _EXT2_H_
