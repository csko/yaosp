/* Inter Process Communication
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

#include <ipc.h>
#include <config.h>
#include <macros.h>

static hashtable_t ipc_port_table;

static void* ipc_port_key( hashitem_t* item ) {
    ipc_port_t* port;

    port = ( ipc_port_t* )item;

    return ( void* )&port->id;
}

static uint32_t ipc_port_hash( const void* key ) {
    return hash_number( ( uint8_t* )key, sizeof( ipc_port_id ) );
}

static bool ipc_port_compare( const void* key1, const void* key2 ) {
    ipc_port_id id1;
    ipc_port_id id2;

    id1 = *( ( ipc_port_id* )key1 );
    id2 = *( ( ipc_port_id* )key2 );

    return ( id1 == id2 );
}

__init int init_ipc( void ) {
    int error;

    error = init_hashtable(
        &ipc_port_table,
        64,
        ipc_port_key,
        ipc_port_hash,
        ipc_port_compare
    );

    if ( error < 0 ) {
        return error;
    }

    return 0;
}
