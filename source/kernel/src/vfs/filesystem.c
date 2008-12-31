/* Filesystem management
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

#include <errno.h>
#include <semaphore.h>
#include <mm/kmalloc.h>
#include <vfs/filesystem.h>
#include <lib/hashtable.h>
#include <lib/string.h>

static hashtable_t filesystem_table;
static semaphore_id filesystem_lock;

static void* filesystem_key( hashitem_t* item ) {
    filesystem_descriptor_t* fs_desc;

    fs_desc = ( filesystem_descriptor_t* )item;

    return ( void* )fs_desc->name;
}

static uint32_t filesystem_hash( const void* key ) {
    return hash_string( ( uint8_t* )key, strlen( ( const char* )key ) );
}

static bool filesystem_compare( const void* key1, const void* key2 ) {
    return ( strcmp( ( const char* )key1, ( const char* )key2 ) == 0 );
}

int register_filesystem( const char* name, filesystem_calls_t* calls ) {
    int error = 0;
    filesystem_descriptor_t* fs_desc;

    fs_desc = ( filesystem_descriptor_t* )kmalloc( sizeof( filesystem_descriptor_t ) );

    if ( fs_desc == NULL ) {
        return -ENOMEM;
    }

    fs_desc->name = strdup( name );

    if ( fs_desc->name == NULL ) {
        kfree( fs_desc );
        return -ENOMEM;
    }

    fs_desc->calls = calls;

    LOCK( filesystem_lock );

    if ( hashtable_get( &filesystem_table, ( const void* )name ) != NULL ) {
        error = -EEXIST;
    }

    if ( error == 0 ) {
        error = hashtable_add( &filesystem_table, ( hashitem_t* )fs_desc );
    }

    UNLOCK( filesystem_lock );

    return error;
}

filesystem_descriptor_t* get_filesystem( const char* name ) {
    filesystem_descriptor_t* fs_desc;

    LOCK( filesystem_lock );

    fs_desc = ( filesystem_descriptor_t* )hashtable_get( &filesystem_table, ( const void* )name );

    UNLOCK( filesystem_lock );

    return fs_desc;
}

int init_filesystems( void ) {
    int error;

    error = init_hashtable(
        &filesystem_table,
        16,
        filesystem_key,
        filesystem_hash,
        filesystem_compare
    );

    if ( error < 0 ) {
        return error;
    }

    filesystem_lock = create_semaphore( "filesystem lock", SEMAPHORE_BINARY, 0, 1 );

    if ( filesystem_lock < 0 ) {
        return filesystem_lock;
    }

    return 0;
}
