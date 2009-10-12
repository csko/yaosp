/* RAM disk driver
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

#include <console.h>
#include <errno.h>
#include <macros.h>
#include <vfs/devfs.h>
#include <vfs/vfs.h>
#include <lock/mutex.h>
#include <mm/pages.h>
#include <mm/kmalloc.h>
#include <lib/string.h>
#include <lib/hashtable.h>

#include "ramdisk.h"

static lock_id ramdisk_lock;
static int ramdisk_id_counter = 0;
static hashtable_t ramdisk_table;

static int ramdisk_read( void* node, void* cookie, void* buffer, off_t position, size_t size ) {
    ramdisk_node_t* ramdisk;

    ramdisk = ( ramdisk_node_t* )node;

    if ( ( position < 0 ) || ( position >= ramdisk->size ) ) {
        return -EINVAL;
    }

    if ( position + size > ramdisk->size ) {
        size = ramdisk->size - position;
    }

    if ( size == 0 ) {
        return 0;
    }

    memcpy( buffer, ramdisk->data + position, size );

    return size;
}

static int ramdisk_write( void* node, void* cookie, const void* buffer, off_t position, size_t size ) {
    ramdisk_node_t* ramdisk;

    ramdisk = ( ramdisk_node_t* )node;

    if ( ( position < 0 ) || ( position >= ramdisk->size ) ) {
        return -EINVAL;
    }

    if ( position + size > ramdisk->size ) {
        size = ramdisk->size - position;
    }

    if ( size == 0 ) {
        return 0;
    }

    memcpy( ramdisk->data + position, buffer, size );

    return size;
}

static device_calls_t ramdisk_calls = {
    .open = NULL,
    .close = NULL,
    .ioctl = NULL,
    .read = ramdisk_read,
    .write = ramdisk_write,
    .add_select_request = NULL,
    .remove_select_request = NULL
};

ramdisk_node_t* create_ramdisk_node( ramdisk_create_info_t* info ) {
    int error;
    uint64_t mod;
    uint64_t aligned_size;
    char path[ 64 ];
    ramdisk_node_t* node;

    /* Do some checks before starting to create the node */

    if ( info->size == 0 ) {
        goto error1;
    }

    /* Align the size to page size boundary */

    mod = info->size % PAGE_SIZE;
    aligned_size = info->size + ( PAGE_SIZE - mod );

    ASSERT( ( aligned_size % PAGE_SIZE ) == 0 );

    node = ( ramdisk_node_t* )kmalloc( sizeof( ramdisk_node_t ) );

    if ( node == NULL ) {
        goto error1;
    }

    node->data = alloc_pages( aligned_size / PAGE_SIZE, MEM_COMMON );

    if ( node->data == NULL ) {
        goto error2;
    }

    node->size = info->size;

    mutex_lock( ramdisk_lock, LOCK_IGNORE_SIGNAL );

    do {
        node->id = ramdisk_id_counter++;

        if ( ramdisk_id_counter < 0 ) {
            ramdisk_id_counter = 0;
        }
    } while ( hashtable_get( &ramdisk_table, ( const void* )node ) != NULL );

    hashtable_add( &ramdisk_table, ( hashitem_t* )node );

    mutex_unlock( ramdisk_lock );

    snprintf( path, sizeof( path ), "storage/ram%d", node->id );

    error = create_device_node( path, &ramdisk_calls, ( void* )node );

    if ( error < 0 ) {
        goto error3;
    }

    /* Load an image file to the ramdisk (if requested) */

    if ( info->load_from_file ) {
        int fd;
        int data_size;
        struct stat st;
        size_t to_read;

        kprintf( INFO, "Loading ramdisk data from file: %s\n", info->image_file );

        fd = open( info->image_file, O_RDONLY );

        if ( fd < 0 ) {
            goto error4;
        }

        if ( fstat( fd, &st ) != 0 ) {
            close( fd );
            goto error4;
        }

        to_read = MIN( info->size, st.st_size );

        data_size = pread( fd, node->data, to_read, 0 );

        close( fd );

        if ( data_size != to_read ) {
            goto error4;
        }
    }

    return node;

error4:
    /* TODO: destroy device node */

error3:
    /* TODO */

error2:
    kfree( node );

error1:
    return NULL;
}

static void* ramdisk_key( hashitem_t* item ) {
    ramdisk_node_t* node;

    node = ( ramdisk_node_t* )item;

    return ( void* )node->id;
}

static uint32_t ramdisk_hash( const void* key ) {
    return hash_number( ( uint8_t* )&key, sizeof( int ) );
}

static bool ramdisk_compare( const void* key1, const void* key2 ) {
    return ( key1 == key2 );
}

int init_module( void ) {
    int error;

    ramdisk_lock = mutex_create( "ramdisk mutex", MUTEX_NONE );

    if ( ramdisk_lock < 0 ) {
        error = ramdisk_lock;
        goto error1;
    }

    error = init_hashtable(
        &ramdisk_table,
        32,
        ramdisk_key,
        ramdisk_hash,
        ramdisk_compare
    );

    if ( error < 0 ) {
        goto error2;
    }

    error = init_ramdisk_control_device();

    if ( error < 0 ) {
        goto error3;
    }

    return 0;

error3:
    destroy_hashtable( &ramdisk_table );

error2:
    mutex_destroy( ramdisk_lock );

error1:
    return error;
}

int destroy_module( void ) {
   return 0;
}
