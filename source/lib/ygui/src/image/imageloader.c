/* Image loader handling
 *
 * Copyright (c) 2009, 2010 Zoltan Kovacs
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
#include <dirent.h>
#include <stdio.h>
#include <dlfcn.h>

#include <ygui/image/imageloader.h>
#include <yutil/array.h>

typedef int imgldr_get_count_t( void );
typedef image_loader_t* imgldr_get_at_t( int index );

static int image_loader_initialized = 0;
static array_t image_loaders;

static int image_loader_load_all( void ) {
    DIR* dir;
    struct dirent* entry;

    init_array( &image_loaders );
    array_set_realloc_size( &image_loaders, 16 );

    dir = opendir( "/system/lib/imageloader" );

    while ( ( entry = readdir( dir ) ) != NULL ) {
        int i;
        int count;
        char path[ 256 ];
        void* handle;
        imgldr_get_count_t* get_count;
        imgldr_get_at_t* get_at;

        if ( ( strcmp( entry->d_name, "." ) == 0 ) ||
             ( strcmp( entry->d_name, ".." ) == 0 ) ) {
            continue;
        }

        snprintf( path, sizeof(path), "/system/lib/imageloader/%s", entry->d_name );

        handle = dlopen( path, RTLD_NOW );

        if ( handle == NULL ) {
            continue;
        }

        get_count = ( imgldr_get_count_t* )dlsym( handle, "image_loader_get_count" );
        get_at = ( imgldr_get_at_t* )dlsym( handle, "image_loader_get_at" );

        if ( ( get_count == NULL ) ||
             ( get_at == NULL ) ) {
            dlclose( handle );
            continue;
        }

        count = get_count();

        for ( i = 0; i < count; i++ ) {
            image_loader_t* loader;

            loader = get_at( i );

            if ( loader != NULL ) {
                array_add_item( &image_loaders, ( void* )loader );
            }
        }
    }

    closedir( dir );

    return 0;
}

int image_loader_find( uint8_t* data, size_t size, image_loader_t** _loader, void** _private ) {
    int i;
    int count;
    image_loader_t* loader;

    if ( !image_loader_initialized ) {
        image_loader_initialized = 1;
        image_loader_load_all();
    }

    if ( ( _loader == NULL ) ||
         ( _private == NULL ) ) {
        return -EINVAL;
    }

    count = array_get_size( &image_loaders );

    for ( i = 0; i < count; i++ ) {
        loader = ( image_loader_t* )array_get_item( &image_loaders, i );

        if ( loader->identify( data, size ) == 0 ) {
            goto found;
        }
    }

    return -1;

 found:
    *_loader = loader;

    return loader->create( _private );
}
