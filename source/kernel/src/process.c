/* Process implementation
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

#include <process.h>
#include <lib/hashtable.h>

static hashtable_t process_table;

static void* process_key( hashitem_t* item ) {
    process_t* process;

    process = ( process_t* )item;

    return ( void* )process->id;
}

static uint32_t process_hash( const void* key ) {
    return ( uint32_t )key;
}

static bool process_compare( const void* key1, const void* key2 ) {
    return ( key1 == key2 );
}

int init_processes( void ) {
    int error;

    error = init_hashtable(
                &process_table,
                256,
                process_key,
                process_hash,
                process_compare
    );

    if ( error < 0 ) {
        return error;
    }

    return 0;
}
