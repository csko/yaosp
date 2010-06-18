/* Process utilities
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

#include <stdlib.h>
#include <string.h>
#include <yaosp/sysinfo.h>

#include <yutil/process.h>

int process_id_of( const char* process_name ) {
    int id;
    uint32_t i;
    uint32_t proc_count;
    process_info_t* proc_table;

    proc_count = get_process_count();

    if ( proc_count == 0 ) {
        return -1;
    }

    proc_table = ( process_info_t* )malloc( sizeof( process_info_t ) * proc_count );

    if ( proc_table == NULL ) {
        return -1;
    }

    id = -1;
    proc_count = get_process_info( proc_table, proc_count );

    for ( i = 0; i < proc_count; i++ ) {
        if ( strcmp( proc_table[ i ].name, process_name ) == 0 ) {
            id = proc_table[ i ].id;
            break;
        }
    }

    free( proc_table );

    return id;
}

int process_count_of( const char* process_name ) {
    int count;
    uint32_t i;
    uint32_t proc_count;
    process_info_t* proc_table;

    proc_count = get_process_count();

    if ( proc_count == 0 ) {
        return 0;
    }

    proc_table = ( process_info_t* )malloc( sizeof( process_info_t ) * proc_count );

    if ( proc_table == NULL ) {
        return 0;
    }

    count = 0;
    proc_count = get_process_info( proc_table, proc_count );

    for ( i = 0; i < proc_count; i++ ) {
        if ( strcmp( proc_table[ i ].name, process_name ) == 0 ) {
            count++;
        }
    }

    free( proc_table );

    return count;

}
