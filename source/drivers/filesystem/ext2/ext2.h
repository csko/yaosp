/* ext2 filesystem driver
 *
 * Copyright (c) 2009 Attila Magyar, Zoltan Kovacs
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
#include <vfs/inode.h>

#define EXT2_MIN_BLOCK_SIZE 1024                    // minimum block size
#define EXT2_NAME_LEN       255                     // maximum file name

/* block indexes */

#define EXT2_NDIR_BLOCKS 12                      // direct blocks count
#define EXT2_IND_BLOCK   EXT2_NDIR_BLOCKS        // index of the indirect block (12)
#define EXT2_DIND_BLOCK  ( EXT2_IND_BLOCK + 1 )  // index of the doubly-indirect block (13)
#define EXT2_TIND_BLOCK  ( EXT2_DIND_BLOCK + 1 ) // index of the triply-indirect block (14)
#define EXT2_N_BLOCKS    ( EXT2_TIND_BLOCK + 1 )

/* FS states */

#define EXT2_VALID_FS 0x0001 /* Unmounted cleanly */
#define EXT2_ERROR_FS 0x0002 /* Errors detected */

#define EXT2_ROOT_INO    2 /* Root inode number */

#define EXT2_SUPER_MAGIC 0xEF53 /* Magic indentifier */

/* file types */

#define EXT2_FT_REG_FILE  0x1
#define EXT2_FT_DIRECTORY 0x2
#define EXT2_FT_SYMLINK   0x7

typedef struct ext2_super_block {
    uint32_t s_inodes_count; /* Inodes count */
    uint32_t s_blocks_count; /* Blocks count */
    uint32_t s_r_blocks_count; /* Reserved blocks count */
    uint32_t s_free_blocks_count; /* Free blocks count */
    uint32_t s_free_inodes_count; /* Free inodes count */
    uint32_t s_first_data_block; /* First Data Block */
    uint32_t s_log_block_size; /* The block size is computed using this 32bit value as the number of bits to shift left the value 1024. */
    uint32_t s_log_frag_size; /* Fragment size */
    uint32_t s_blocks_per_group; /* # Blocks per group */
    uint32_t s_frags_per_group; /* # Fragments per group */
    uint32_t s_inodes_per_group; /* # Inodes per group */
    uint32_t s_mtime; /* Mount time */
    uint32_t s_wtime; /* Write time */
    uint16_t s_mnt_count; /* Mount count */
    uint16_t s_max_mnt_count; /* Maximal mount count */
    uint16_t s_magic; /* Magic signature */
    uint16_t s_state; /* File system state */
    uint16_t s_errors; /* Behaviour when detecting errors */
    uint16_t s_minor_rev_level; /* minor revision level */
    uint32_t s_lastcheck; /* time of last check */
    uint32_t s_checkinterval; /* max. time between checks */
    uint32_t s_creator_os; /* OS */
    uint32_t s_rev_level; /* Revision level */
    uint16_t s_def_resuid; /* Default uid for reserved blocks */
    uint16_t s_def_resgid; /* Default gid for reserved blocks */

    /*
     * These fields are for EXT2_DYNAMIC_REV superblocks only.
     *
     * Note: the difference between the compatible feature set and
     * the incompatible feature set is that if there is a bit set
     * in the incompatible feature set that the kernel doesn't
     * know about, it should refuse to mount the filesystem.
     *
     * e2fsck's requirements are more strict; if it doesn't know
     * about a feature in either the compatible or incompatible
     * feature set, it must abort and not try to meddle with
     * things it doesn't understand...
     */

    uint32_t s_first_ino; /* First non-reserved inode */
    uint16_t s_inode_size; /* size of inode structure */
    uint16_t s_block_group_nr; /* block group # of this superblock */
    uint32_t s_feature_compat; /* compatible feature set */
    uint32_t s_feature_incompat; /* incompatible feature set */
    uint32_t s_feature_ro_compat; /* readonly-compatible feature set */
    uint8_t s_uuid[ 16 ]; /* 128-bit uuid for volume */
    char s_volume_name[ 16 ]; /* volume name */
    char s_last_mounted[ 64 ]; /* directory where last mounted */
    uint32_t s_algorithm_usage_bitmap; /* For compression */

    /*
     * Performance hints.  Directory preallocation should only
     * happen if the EXT2_COMPAT_PREALLOC flag is on.
     */

    uint8_t s_prealloc_blocks; /* Nr of blocks to try to preallocate*/
    uint8_t s_prealloc_dir_blocks; /* Nr to preallocate for dirs */
    uint16_t s_padding1;

    /*
     * Journaling support valid if EXT3_FEATURE_COMPAT_HAS_JOURNAL set.
     */

    uint8_t s_journal_uuid[ 16 ]; /* uuid of journal superblock */
    uint32_t s_journal_inum; /* inode number of journal file */
    uint32_t s_journal_dev; /* device number of journal file */
    uint32_t s_last_orphan; /* start of list of inodes to delete */
    uint32_t s_hash_seed[ 4 ]; /* HTREE hash seed */
    uint8_t s_def_hash_version; /* Default hash version to use */
    uint8_t s_reserved_char_pad;
    uint16_t s_reserved_word_pad;
    uint32_t s_default_mount_opts;
    uint32_t s_first_meta_bg; /* First metablock block group */
    uint32_t s_reserved[ 190 ]; /* Padding to the end of the block */
} __attribute__(( packed )) ext2_super_block_t;

typedef struct ext2_group_desc {
    uint32_t bg_block_bitmap; /* Blocks bitmap block */
    uint32_t bg_inode_bitmap; /* Inodes bitmap block */
    uint32_t bg_inode_table; /* Inodes table block */
    uint16_t bg_free_blocks_count; /* Free blocks count */
    uint16_t bg_free_inodes_count; /* Free inodes count */
    uint16_t bg_used_dirs_count; /* Directories count */
    uint16_t bg_pad;
    uint32_t bg_reserved[ 3 ];
} __attribute__(( packed )) ext2_group_desc_t;

typedef struct ext2_group {
    ext2_group_desc_t descriptor;
    uint32_t* inode_bitmap;
    uint32_t* block_bitmap;
    uint32_t flags;
} ext2_group_t;

enum {
    EXT2_INODE_BITMAP_DIRTY = ( 1 << 0 ),
    EXT2_BLOCK_BITMAP_DIRTY = ( 1 << 1 )
};

/*
 * Structure of an inode on the disk
 */

typedef struct ext2_fs_inode {
    uint16_t i_mode; /* File mode */
    uint16_t i_uid; /* Low 16 bits of Owner Uid */
    uint32_t i_size; /* Size in bytes */
    uint32_t i_atime; /* Access time */
    uint32_t i_ctime; /* Creation time */
    uint32_t i_mtime; /* Modification time */
    uint32_t i_dtime; /* Deletion Time */
    uint16_t i_gid; /* Low 16 bits of Group Id */
    uint16_t i_links_count; /* Links count */
    uint32_t i_blocks; /* Blocks count */
    uint32_t i_flags; /* File flags */

    union {
        struct {
            uint32_t  l_i_reserved1;
        } linux1;

        struct {
            uint32_t  h_i_translator;
        } hurd1;

        struct {
            uint32_t  m_i_reserved1;
        } masix1;
    } osd1; /* OS dependent 1 */

    uint32_t i_block[ EXT2_N_BLOCKS ]; /* Pointers to blocks */
    uint32_t i_generation; /* File version (for NFS) */
    uint32_t i_file_acl; /* File ACL */
    uint32_t i_dir_acl; /* Directory ACL */
    uint32_t i_faddr; /* Fragment address */

    union {
        struct {
            uint8_t l_i_frag; /* Fragment number */
            uint8_t l_i_fsize; /* Fragment size */
            uint16_t i_pad1;
            uint16_t l_i_uid_high; /* these 2 fields    */
            uint16_t l_i_gid_high; /* were reserved2[0] */
            uint32_t l_i_reserved2;
        } linux2;

        struct {
            uint8_t h_i_frag; /* Fragment number */
            uint8_t h_i_fsize; /* Fragment size */
            uint16_t h_i_mode_high;
            uint16_t h_i_uid_high;
            uint16_t h_i_gid_high;
            uint32_t h_i_author;
        } hurd2;

        struct {
            uint8_t m_i_frag; /* Fragment number */
            uint8_t m_i_fsize; /* Fragment size */
            uint16_t m_pad1;
            uint32_t m_i_reserved2[ 2 ];
        } masix2;
    } osd2; /* OS dependent 2 */
} __attribute__(( packed )) ext2_fs_inode_t;

typedef struct ext2_dir_entry {
    uint32_t inode; /* Inode number */
    uint16_t rec_len; /* Directory entry length */
    uint8_t name_len; /* Name length */
    uint8_t file_type;
} __attribute__(( packed )) ext2_dir_entry_t;

typedef struct ext2_cookie {
    int fd;
    uint32_t ngroups; /* Number of block groups */
    uint32_t blocksize; /* Size of one block */
    uint32_t sectors_per_block;
    uint32_t ptr_per_block; /* Pointer per block */
    uint32_t doubly_indirect_block_count;
    uint32_t triply_indirect_block_count;
    ext2_super_block_t super_block; /* Superblock */
    ext2_group_t* groups;
    uint32_t flags; /* Mount flags */
} ext2_cookie_t;

typedef struct ext2_dir_cookie {
    uint32_t dir_offset;
} ext2_dir_cookie_t;

typedef struct ext2_file_cookie {
    int open_flags;
} ext2_file_cookie_t;

typedef struct ext2_inode {
    ino_t inode_number;
    ext2_fs_inode_t fs_inode;
} ext2_inode_t;

typedef struct ext2_lookup_data {
    char* name;
    int name_length;
    ino_t inode_number;
} ext2_lookup_data_t;

typedef bool ext2_walk_callback_t( ext2_dir_entry_t* entry, void* data );

int ext2_calc_block_num( ext2_cookie_t *cookie, ext2_inode_t *vinode, uint32_t block_num, uint32_t* out );

/* Inode handling functions */

int ext2_do_read_inode( ext2_cookie_t* cookie, ext2_inode_t* inode );
int ext2_do_write_inode( ext2_cookie_t* cookie, ext2_inode_t* inode );
int ext2_do_alloc_inode( ext2_cookie_t* cookie, ext2_inode_t* inode, bool for_directory );
int ext2_do_free_inode( ext2_cookie_t* cookie, ext2_inode_t* inode, bool is_directory );
int ext2_do_free_inode_blocks( ext2_cookie_t* cookie, ext2_inode_t* inode );

int ext2_do_read_inode_block( ext2_cookie_t* cookie, ext2_inode_t* inode, uint32_t block_number, void* buffer );
int ext2_do_write_inode_block( ext2_cookie_t* cookie, ext2_inode_t* inode, uint32_t block_number, void* buffer );

int ext2_do_get_new_inode_block( ext2_cookie_t* cookie, ext2_inode_t* inode, uint32_t* new_block_number );

/* Block handling functions */

int ext2_do_alloc_block( ext2_cookie_t* cookie, uint32_t* block_number );
int ext2_do_free_block( ext2_cookie_t* cookie, uint32_t block_number );

/* Directory handling functions */

int ext2_do_walk_directory( ext2_cookie_t* cookie, ext2_inode_t* parent, ext2_walk_callback_t* callback, void* data );
int ext2_do_insert_entry( ext2_cookie_t* cookie, ext2_inode_t* parent, ext2_dir_entry_t* new_entry, int new_entry_size );
int ext2_do_remove_entry( ext2_cookie_t* cookie, ext2_inode_t* parent, ino_t inode_number );
int ext2_is_directory_empty( ext2_cookie_t* cookie, ext2_inode_t* parent );

ext2_dir_entry_t* ext2_do_alloc_dir_entry( int name_length );

#endif /* _EXT2_H_ */
