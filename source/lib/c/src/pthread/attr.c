/* pthread attr functions
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
#include <pthread.h>

int pthread_attr_init( pthread_attr_t* attr ) {
    attr->name = NULL;

    return 0;
}

int pthread_attr_destroy( pthread_attr_t* attr ) {
    if ( attr->name != NULL ) {
        free( attr->name );
        attr->name = NULL;
    }

    return 0;
}

int pthread_attr_getname( pthread_attr_t* attr, char** name ) {
    *name = attr->name;

    return 0;
}

int pthread_attr_setname( pthread_attr_t* attr, char* name ) {
    attr->name = strdup( name );

    return 0;
}
