/* Filesystem management
 *
 * Copyright (c) 2008, 2009 Zoltan Kovacs
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
#include <kernel.h>
#include <lock/mutex.h>
#include <mm/kmalloc.h>
#include <vfs/filesystem.h>
#include <lib/hashtable.h>
#include <lib/string.h>

static lock_id filesystem_mutex;
static hashtable_t filesystem_table;

static void* filesystem_key( hashitem_t* item ) {
    filesystem_descriptor_t* fs_desc;

    fs_desc = ( filesystem_descriptor_t* )item;

    return ( void* )fs_desc->name;
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

    mutex_lock( filesystem_mutex, LOCK_IGNORE_SIGNAL );

    if ( hashtable_get( &filesystem_table, ( const void* )name ) != NULL ) {
        error = -EEXIST;
    }

    if ( error == 0 ) {
        error = hashtable_add( &filesystem_table, ( hashitem_t* )fs_desc );
    }

    mutex_unlock( filesystem_mutex );

    if ( error < 0 ) {
        kfree( fs_desc->name );
        kfree( fs_desc );
    }

    return error;
}

filesystem_descriptor_t* get_filesystem( const char* name ) {
    filesystem_descriptor_t* fs_desc;

    mutex_lock( filesystem_mutex, LOCK_IGNORE_SIGNAL );

    fs_desc = ( filesystem_descriptor_t* )hashtable_get( &filesystem_table, ( const void* )name );

    mutex_unlock( filesystem_mutex );

    return fs_desc;
}

typedef struct fs_probe_data {
    const char* device;
    filesystem_descriptor_t* fs_desc;
} fs_probe_data_t;

static int probe_fs_iterator( hashitem_t* item, void* _data ) {
    fs_probe_data_t* data;
    filesystem_descriptor_t* desc;

    data = ( fs_probe_data_t* )_data;
    desc = ( filesystem_descriptor_t* )item;

    if ( ( desc->calls->probe != NULL ) &&
         ( desc->calls->probe( data->device ) ) ) {
        data->fs_desc = desc;

        return -1;
    }

    return 0;
}

filesystem_descriptor_t* probe_filesystem( const char* device ) {
    fs_probe_data_t data;

    data.device = device;
    data.fs_desc = NULL;

    mutex_lock( filesystem_mutex, LOCK_IGNORE_SIGNAL );

    hashtable_iterate( &filesystem_table, probe_fs_iterator, ( void* )&data );

    mutex_unlock( filesystem_mutex );

    return data.fs_desc;
}

__init int init_filesystems( void ) {
    int error;

    error = init_hashtable(
        &filesystem_table,
        16,
        filesystem_key,
        hash_str,
        compare_str
    );

    if ( error < 0 ) {
        return error;
    }

    filesystem_mutex = mutex_create( "fs table mutex", MUTEX_NONE );

    if ( filesystem_mutex < 0 ) {
        destroy_hashtable( &filesystem_table );
        return filesystem_mutex;
    }

    return 0;
}
