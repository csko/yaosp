/* Module loader and manager
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

#include <module.h>
#include <console.h>
#include <errno.h>
#include <semaphore.h>
#include <bootmodule.h>
#include <mm/kmalloc.h>
#include <vfs/vfs.h>
#include <lib/string.h>

typedef struct file_module_reader {
    int fd;
    char* name;
} file_module_reader_t;

module_loader_t* module_loader;

static hashtable_t module_table;
static semaphore_id module_lock;

static int do_load_module( const char* name );

static module_t* create_module( const char* name ) {
    module_t* module;

    module = ( module_t* )kmalloc( sizeof( module_t ) );

    if ( module == NULL ) {
        return NULL;
    }

    memset( module, 0, sizeof( module_t ) );

    module->name = strdup( name );

    if ( module->name == NULL ) {
        kfree( module );
        return NULL;
    }

    return module;
}

static void destroy_module( module_t* module ) {
    kfree( module->name );
    kfree( module );
}

int read_module_data( module_reader_t* reader, void* buffer, off_t offset, int size ) {
    return reader->read( reader->private, buffer, offset, size );
}

size_t get_module_size( module_reader_t* reader ) {
    return reader->get_size( reader->private );
}

char* get_module_name( module_reader_t* reader ) {
    return reader->get_name( reader->private );
}

uint32_t get_loaded_module_count( void ) {
    uint32_t count;

    LOCK( module_lock );

    count = hashtable_get_item_count( &module_table );

    UNLOCK( module_lock );

    return count;
}

static int file_module_read( void* private, void* data, off_t offset, int size ) {
    file_module_reader_t* reader;

    reader = ( file_module_reader_t* )private;

    return pread( reader->fd, data, size, offset );
}

static size_t file_module_get_size( void* private ) {
    int error;
    struct stat st;
    file_module_reader_t* reader;

    reader = ( file_module_reader_t* )private;

    error = fstat( reader->fd, &st );

    if ( error < 0 ) {
        return 0;
    }

    return ( size_t )st.st_size;
}

static char* file_module_get_name( void* private ) {
    file_module_reader_t* reader;

    reader = ( file_module_reader_t* )private;

    return reader->name;
}

static module_reader_t* get_file_module_reader_helper( const char* directory, const char* module_name ) {
    int file;
    char path[ 128 ];
    module_reader_t* reader;
    file_module_reader_t* file_reader;

    snprintf( path, sizeof( path ), "/yaosp/system/module/%s/%s", directory, module_name );

    file = open( path, O_RDONLY );

    if ( file < 0 ) {
        goto error1;
    }

    reader = ( module_reader_t* )kmalloc( sizeof( module_reader_t ) + sizeof( file_module_reader_t ) );

    if ( reader == NULL ) {
        goto error2;
    }

    file_reader = ( file_module_reader_t* )( reader + 1 );

    file_reader->name = strdup( module_name );

    if ( file_reader->name == NULL ) {
        goto error3;
    }

    file_reader->fd = file;

    reader->private = ( void* )file_reader;
    reader->read = file_module_read;
    reader->get_size = file_module_get_size;
    reader->get_name = file_module_get_name;

    return reader;

error3:
    kfree( reader );

error2:
    close( file );

error1:
    return NULL;
}

static module_reader_t* get_file_module_reader( const char* name ) {
    int dir;
    dirent_t entry;
    module_reader_t* reader;

    dir = open( "/yaosp/system/module", O_RDONLY );

    if ( dir < 0 ) {
        return NULL;
    }

    while ( getdents( dir, &entry ) == 1 ) {
        if ( ( strcmp( entry.name, "." ) == 0 ) ||
             ( strcmp( entry.name, ".." ) == 0 ) ) {
            continue;
        }

        reader = get_file_module_reader_helper( entry.name, name );

        if ( reader != NULL ) {
            break;
        }
    }

    close( dir );

    return reader;
}

static void put_file_module_reader( module_reader_t* reader ) {
    file_module_reader_t* file_reader;

    file_reader = ( file_module_reader_t* )reader->private;

    close( file_reader->fd );
    kfree( file_reader->name );
    kfree( reader );
}

static int do_load_module_dependencies( module_t* module, char** dependencies, size_t count ) {
    int i;
    int error;

    for ( i = 0; i < count; i++ ) {
        error = do_load_module( dependencies[ i ] );

        if ( error == -ELOOP ) {
            kprintf(
                "Detected a loop in module dependencies while loading %s module!\n",
                dependencies[ i ]
            );
        }

        if ( error < 0 ) {
            return error;
        }
    }

    return 0;
}

static int do_load_module( const char* name ) {
    int i;
    int error;
    bool found;
    module_t* module;
    bootmodule_t* bootmodule;
    module_reader_t* reader;
    bool is_bootmodule;
    module_dependencies_t module_deps;

    reader = NULL;
    is_bootmodule = false;

    /* Try bootmodules first */

    for ( i = 0; i < get_bootmodule_count(); i++ ) {
        bootmodule = get_bootmodule_at( i );

        if ( strcmp( bootmodule->name, name ) == 0 ) {
            reader = get_bootmodule_reader( i );
            is_bootmodule = true;
            break;
        }
    }

    /* Try to load the module from a simple file */

    if ( reader == NULL ) {
        reader = get_file_module_reader( name );
    }

    /* If we didn't find anything we can't load the module :( */

    if ( reader == NULL ) {
        error = -ENOENT;

        goto error1;
    }

    /* Check if this is a valid module */

    if ( !module_loader->check_module( reader ) ) {
        error = -EINVAL;

        goto error2;
    }

    LOCK( module_lock );

    module = ( module_t* )hashtable_get( &module_table, ( const void* )name );

    if ( module != NULL ) {
        switch ( module->status ) {
            case MODULE_LOADING :
                error = -ELOOP;
                break;

            default :
                error = 0;
                break;
        }

        UNLOCK( module_lock );

        goto error2;
    }

    module = create_module( name );

    if ( module == NULL ) {
        UNLOCK( module_lock );

        error = -ENOMEM;

        goto error2;
    }

    module->status = MODULE_LOADING;

    hashtable_add( &module_table, ( hashitem_t* )module );

    UNLOCK( module_lock );

    /* Load the module */

    error = module_loader->load_module( module, reader );

    if ( error < 0 ) {
        goto error3;
    }

    found = module_loader->get_symbol( module, "init_module", ( ptr_t* )&module->init );

    if ( !found ) {
        goto error4;
    }

    found = module_loader->get_symbol( module, "destroy_module", ( ptr_t* )&module->destroy );

    if ( !found ) {
        goto error4;
    }

    /* Load the dependencies */

    error = module_loader->get_dependencies( module, &module_deps );

    if ( error < 0 ) {
        goto error4;
    }

    error = do_load_module_dependencies(
        module,
        module_deps.dep_table,
        module_deps.dep_count
    );

    if ( error < 0 ) {
        goto error4;
    }

    do_load_module_dependencies(
        module,
        module_deps.optional_dep_table,
        module_deps.optional_dep_count
    );

    error = module->init();

    if ( error < 0 ) {
        goto error4;
    }

    LOCK( module_lock );

    module->status = MODULE_LOADED;

    UNLOCK( module_lock );

    return 0;

error4:
    /* TODO: unload the module */

error3:
    LOCK( module_lock );
    hashtable_remove( &module_table, ( const void* )name );
    UNLOCK( module_lock );

    destroy_module( module );

error2:
    if ( is_bootmodule ) {
        put_bootmodule_reader( reader );
    } else {
        put_file_module_reader( reader );
    }

error1:
    return error;
}

int load_module( const char* name ) {
    return do_load_module( name );
}

int sys_load_module( const char* name ) {
    return do_load_module( name );
}

void set_module_loader( module_loader_t* loader ) {
    module_loader = loader;
    kprintf( "Using %s module loader.\n", module_loader->name );
}

static void* module_key( hashitem_t* item ) {
    module_t* module;

    module = ( module_t* )item;

    return ( void* )module->name;
}

static uint32_t module_hash( const void* key ) {
    return hash_string( ( uint8_t* )key, strlen( ( const char* )key ) );
}

static bool module_compare( const void* key1, const void* key2 ) {
    return ( strcmp( ( const char* )key1, ( const char* )key2 ) == 0 );
}

int init_module_loader( void ) {
    int error;

    module_loader = NULL;

    error = init_hashtable(
        &module_table,
        32,
        module_key,
        module_hash,
        module_compare
    );

    if ( error < 0 ) {
        return error;
    }

    module_lock = create_semaphore( "module lock", SEMAPHORE_BINARY, 0, 1 );

    if ( module_lock < 0 ) {
        destroy_hashtable( &module_table );
        return module_lock;
    }

    return 0;
}
