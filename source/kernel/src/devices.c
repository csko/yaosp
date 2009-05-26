/* Device management
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

#include <devices.h>
#include <semaphore.h>
#include <time.h>
#include <errno.h>
#include <kernel.h>
#include <macros.h>
#include <mm/kmalloc.h>
#include <lib/string.h>

static hashtable_t bus_table;
static semaphore_id bus_table_lock = -1;

int register_bus_driver( const char* name, void* bus ) {
    int error;
    bus_driver_t* driver;

    driver = ( bus_driver_t* )kmalloc( sizeof( bus_driver_t ) );

    if ( __unlikely( driver == NULL ) ) {
        error = -ENOMEM;
        goto error1;
    }

    driver->name = strdup( name );

    if ( __unlikely( driver->name == NULL ) ) {
        error = -ENOMEM;
        goto error2;
    }

    driver->bus = bus;

    error = LOCK( bus_table_lock );

    if ( __unlikely( error < 0 ) ) {
        goto error2;
    }

    if ( hashtable_get( &bus_table, ( const void* )name ) == NULL ) {
        error = hashtable_add( &bus_table, ( hashitem_t* )driver );
    } else {
        error = -EEXIST;
    }

    UNLOCK( bus_table_lock );

    if ( __unlikely( error < 0 ) ) {
        goto error2;
    }

    return 0;

error2:
    kfree( driver );

error1:
    return error;
}

int unregister_bus_driver( const char* name ) {
    int error;
    bus_driver_t* driver;

    error = LOCK( bus_table_lock );

    if ( __unlikely( error < 0 ) ) {
        return error;
    }

    driver = ( bus_driver_t* )hashtable_get( &bus_table, ( const void* )name );

    if ( __likely( driver != NULL ) ) {
        hashtable_remove( &bus_table, ( const void* )name );
    }

    UNLOCK( bus_table_lock );

    if ( driver == NULL ) {
        return -EINVAL;
    }

    kfree( driver->name );
    kfree( driver );

    return 0;
}

void* get_bus_driver( const char* name ) {
    int error;
    void* bus;
    bus_driver_t* driver;

    error = LOCK( bus_table_lock );

    if ( __unlikely( error < 0 ) ) {
        return NULL;
    }

    driver = ( bus_driver_t* )hashtable_get( &bus_table, ( const void* )name );

    if ( __unlikely( driver == NULL ) ) {
        bus = NULL;
    } else {
        bus = driver->bus;
    }

    UNLOCK( bus_table_lock );

    return bus;
}

static void* bus_driver_key( hashitem_t* item ) {
    bus_driver_t* driver;

    driver = ( bus_driver_t* )item;

    return ( void* )driver->name;
}

static uint32_t bus_driver_hash( const void* key ) {
    return hash_string( ( uint8_t* )key, strlen( ( const char* )key ) );
}

static bool bus_driver_compare( const void* key1, const void* key2 ) {
    return ( strcmp( ( const char* )key1, ( const char* )key2 ) == 0 );
}

__init int init_devices( void ) {
    int error;

    /* Initialize bus driver table */

    error = init_hashtable(
        &bus_table,
        32,
        bus_driver_key,
        bus_driver_hash,
        bus_driver_compare
    );

    if ( error < 0 ) {
        goto error1;
    }

    /* Create the bus driver table lock */

    bus_table_lock = create_semaphore( "bus table lock", SEMAPHORE_BINARY, 0, 1 );

    if ( bus_table_lock < 0 ) {
        error = bus_table_lock;
        goto error2;
    }

    return 0;

error2:
    destroy_hashtable( &bus_table );

error1:
    return error;
}
