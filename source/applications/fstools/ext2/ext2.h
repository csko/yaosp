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

#ifndef _EXT2_H_
#define _EXT2_H_

#include <sys/types.h>

/* ext2 magic number */

#define EXT2_SUPER_MAGIC        0xEF53

/* s_state value(s) */

#define EXT2_VALID_FS           0x0001

/* s_errors value(s) */

#define EXT2_ERRORS_CONTINUE    0x0001

/* file_type values in ext2_dir_entry_t */

#define EXT2_FT_REG_FILE        0x1
#define EXT2_FT_DIRECTORY       0x2

/* block indexes */

#define EXT2_NDIR_BLOCKS        12                      // direct blocks count
#define EXT2_IND_BLOCK          EXT2_NDIR_BLOCKS        // index of the indirect block (12)
#define EXT2_DIND_BLOCK         (EXT2_IND_BLOCK + 1)    // index of the doubly-indirect block (13)
#define EXT2_TIND_BLOCK         (EXT2_DIND_BLOCK + 1)   // index of the triply-indirect block (14)
#define EXT2_N_BLOCKS           (EXT2_TIND_BLOCK + 1)

#define EXT2_FEATURE_INCOMPAT_FILETYPE 0x0002

typedef struct ext2_super_block {
    uint32_t    s_inodes_count;         /* Inodes count */
    uint32_t    s_blocks_count;         /* Blocks count */
    uint32_t    s_r_blocks_count;           /* Reserved blocks count */
    uint32_t    s_free_blocks_count;        /* Free blocks count */
    uint32_t    s_free_inodes_count;        /* Free inodes count */
    uint32_t    s_first_data_block;         /* First Data Block */
    uint32_t    s_log_block_size;           /* The block size is computed using this 32bit value as the number of bits to shift left the value 1024. This value may only be positive. block size = 1024 << s_log_block_size; */
    uint32_t    s_log_frag_size;            /* Fragment size */
    uint32_t    s_blocks_per_group;     /* # Blocks per group */
    uint32_t    s_frags_per_group;          /* # Fragments per group */
    uint32_t    s_inodes_per_group;     /* # Inodes per group */
    uint32_t    s_mtime;                    /* Mount time */
    uint32_t    s_wtime;                    /* Write time */
    uint16_t    s_mnt_count;                /* Mount count */
    uint16_t    s_max_mnt_count;            /* Maximal mount count */
    uint16_t    s_magic;                    /* Magic signature */
    uint16_t    s_state;                    /* File system state */
    uint16_t    s_errors;                   /* Behaviour when detecting errors */
    uint16_t    s_minor_rev_level;          /* minor revision level */
    uint32_t    s_lastcheck;                /* time of last check */
    uint32_t    s_checkinterval;            /* max. time between checks */
    uint32_t    s_creator_os;               /* OS */
    uint32_t    s_rev_level;                /* Revision level */
    uint16_t    s_def_resuid;               /* Default uid for reserved blocks */
    uint16_t    s_def_resgid;               /* Default gid for reserved blocks */

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

    uint32_t    s_first_ino;                /* First non-reserved inode */
    uint16_t    s_inode_size;               /* size of inode structure */
    uint16_t    s_block_group_nr;           /* block group # of this superblock */
    uint32_t    s_feature_compat;           /* compatible feature set */
    uint32_t    s_feature_incompat;         /* incompatible feature set */
    uint32_t    s_feature_ro_compat;        /* readonly-compatible feature set */
    uint8_t     s_uuid[16];             /* 128-bit uuid for volume */
    char        s_volume_name[16];      /* volume name */
    char        s_last_mounted[64];         /* directory where last mounted */
    uint32_t    s_algorithm_usage_bitmap;   /* For compression */

    /*
     * Performance hints.  Directory preallocation should only
     * happen if the EXT2_COMPAT_PREALLOC flag is on.
     */

    uint8_t s_prealloc_blocks;              /* Nr of blocks to try to preallocate*/
    uint8_t s_prealloc_dir_blocks;          /* Nr to preallocate for dirs */
    uint16_t    s_padding1;

    /*
     * Journaling support valid if EXT3_FEATURE_COMPAT_HAS_JOURNAL set.
     */

    uint8_t s_journal_uuid[16];             /* uuid of journal superblock */
    uint32_t    s_journal_inum;         /* inode number of journal file */
    uint32_t    s_journal_dev;              /* device number of journal file */
    uint32_t    s_last_orphan;              /* start of list of inodes to delete */
    uint32_t    s_hash_seed[4];         /* HTREE hash seed */
    uint8_t     s_def_hash_version;         /* Default hash version to use */
    uint8_t     s_reserved_char_pad;
    uint16_t    s_reserved_word_pad;
    uint32_t    s_default_mount_opts;
    uint32_t    s_first_meta_bg;            /* First metablock block group */
    uint32_t    s_reserved[190];            /* Padding to the end of the block */
} __attribute__(( packed )) ext2_super_block_t;

typedef struct ext2_group_desc {
    uint32_t bg_block_bitmap;        /* Blocks bitmap block */
    uint32_t bg_inode_bitmap;        /* Inodes bitmap block */
    uint32_t bg_inode_table;     /* Inodes table block */
    uint16_t bg_free_blocks_count;   /* Free blocks count */
    uint16_t bg_free_inodes_count;   /* Free inodes count */
    uint16_t bg_used_dirs_count; /* Directories count */
    uint16_t bg_pad;
    uint32_t bg_reserved[ 3 ];
} __attribute__(( packed )) ext2_group_desc_t;

/*
 * Structure of an inode on the disk
 */

typedef struct ext2_fs_inode {
    uint16_t i_mode;         /* File mode */
    uint16_t i_uid;          /* Low 16 bits of Owner Uid */
    uint32_t i_size;         /* Size in bytes */
    uint32_t i_atime;        /* Access time */
    uint32_t i_ctime;        /* Creation time */
    uint32_t i_mtime;        /* Modification time */
    uint32_t i_dtime;        /* Deletion Time */
    uint16_t i_gid;          /* Low 16 bits of Group Id */
    uint16_t i_links_count;  /* Links count */
    uint32_t i_blocks;       /* Blocks count */
    uint32_t i_flags;        /* File flags */
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
    } osd1;             /* OS dependent 1 */
    uint32_t    i_block[EXT2_N_BLOCKS];/* Pointers to blocks */
    uint32_t    i_generation;           /* File version (for NFS) */
    uint32_t    i_file_acl;             /* File ACL */
    uint32_t    i_dir_acl;              /* Directory ACL */
    uint32_t    i_faddr;                /* Fragment address */
    union {
        struct {
            uint8_t l_i_frag;   /* Fragment number */
            uint8_t l_i_fsize;  /* Fragment size */
            uint16_t    i_pad1;
            uint16_t    l_i_uid_high;   /* these 2 fields    */
            uint16_t    l_i_gid_high;   /* were reserved2[0] */
            uint32_t    l_i_reserved2;
        } linux2;
        struct {
            uint8_t h_i_frag;   /* Fragment number */
            uint8_t h_i_fsize;  /* Fragment size */
            uint16_t    h_i_mode_high;
            uint16_t    h_i_uid_high;
            uint16_t    h_i_gid_high;
            uint32_t    h_i_author;
        } hurd2;
        struct {
            uint8_t m_i_frag;   /* Fragment number */
            uint8_t m_i_fsize;  /* Fragment size */
            uint16_t    m_pad1;
            uint32_t    m_i_reserved2[2];
        } masix2;
    } osd2;             /* OS dependent 2 */
} __attribute__(( packed )) ext2_fs_inode_t;

typedef struct ext2_dir_entry {
    uint32_t    inode;              /* Inode number */
    uint16_t    rec_len;                /* Directory entry length */
    uint8_t     name_len;           /* Name length */
    uint8_t     file_type;
} __attribute__(( packed )) ext2_dir_entry_t;

#endif /* _EXT2_H_ */
