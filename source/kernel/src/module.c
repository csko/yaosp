/* Module loader and manager
 *
 * Copyright (c) 2008, 2009 Zoltan Kovacs
 * Copyright (c) 2009 Kornel Csernai
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
#include <bootmodule.h>
#include <kernel.h>
#include <mm/kmalloc.h>
#include <vfs/vfs.h>
#include <lock/mutex.h>
#include <lib/string.h>

typedef struct file_module_reader {
    int fd;
    char* name;
} file_module_reader_t;

typedef struct module_info_iter_data {
    uint32_t curr_index;
    uint32_t max_count;
    module_info_t* info_table;
} module_info_iter_data_t;

module_loader_t* module_loader;

static lock_id module_mutex;
static hashtable_t module_table;

static int do_load_module( const char* name );

static module_t* create_module( const char* name ) {
    module_t* module;

    module = ( module_t* )kmalloc( sizeof( module_t ) );

    if ( module == NULL ) {
        goto error1;
    }

    memset( module, 0, sizeof( module_t ) );

    module->name = strdup( name );

    if ( module->name == NULL ) {
        goto error2;
    }

    return module;

error2:
    kfree( module );

error1:
    return NULL;
}

static void destroy_module( module_t* module ) {
    kfree( module->name );
    kfree( module );
}

static int file_module_read( void* private, void* data, off_t offset, int size ) {
    file_module_reader_t* reader;

    reader = ( file_module_reader_t* )private;

    return pread( reader->fd, data, size, offset );
}

static char* file_module_get_name( void* private ) {
    file_module_reader_t* reader;

    reader = ( file_module_reader_t* )private;

    return reader->name;
}

static binary_loader_t* get_file_module_reader_helper( const char* directory, const char* module_name ) {
    int file;
    char path[ 128 ];
    binary_loader_t* loader;
    file_module_reader_t* file_reader;

    snprintf( path, sizeof( path ), "/yaosp/system/module/%s/%s", directory, module_name );

    file = open( path, O_RDONLY );

    if ( file < 0 ) {
        goto error1;
    }

    loader = ( binary_loader_t* )kmalloc( sizeof( binary_loader_t ) + sizeof( file_module_reader_t ) );

    if ( loader == NULL ) {
        goto error2;
    }

    file_reader = ( file_module_reader_t* )( loader + 1 );

    file_reader->name = strdup( module_name );

    if ( file_reader->name == NULL ) {
        goto error3;
    }

    file_reader->fd = file;

    loader->private = ( void* )file_reader;
    loader->read = file_module_read;
    loader->get_name = file_module_get_name;
    loader->get_fd = NULL;

    return loader;

error3:
    kfree( loader );

error2:
    close( file );

error1:
    return NULL;
}

static binary_loader_t* get_file_module_loader( const char* name ) {
    int dir;
    dirent_t entry;
    binary_loader_t* loader = NULL;

    dir = open( "/yaosp/system/module", O_RDONLY );

    if ( dir < 0 ) {
        return NULL;
    }

    while ( getdents( dir, &entry, sizeof( dirent_t ) ) == 1 ) {
        if ( ( strcmp( entry.name, "." ) == 0 ) ||
             ( strcmp( entry.name, ".." ) == 0 ) ) {
            continue;
        }

        loader = get_file_module_reader_helper( entry.name, name );

        if ( loader != NULL ) {
            break;
        }
    }

    close( dir );

    return loader;
}

static void put_file_module_loader( binary_loader_t* reader ) {
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
                WARNING,
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
    binary_loader_t* loader;
    bool is_bootmodule;
    module_dependencies_t module_deps;

    loader = NULL;
    is_bootmodule = false;

    /* Try bootmodules first */

    for ( i = 0; i < get_bootmodule_count(); i++ ) {
        bootmodule = get_bootmodule_at( i );

        if ( strcmp( bootmodule->name, name ) == 0 ) {
            loader = get_bootmodule_loader( i );
            is_bootmodule = true;
            break;
        }
    }

    /* Try to load the module from a simple file */

    if ( loader == NULL ) {
        loader = get_file_module_loader( name );
    }

    /* If we didn't find anything we can't load the module :( */

    if ( loader == NULL ) {
        error = -ENOENT;

        goto error1;
    }

    /* Check if this is a valid module */

    if ( !module_loader->check_module( loader ) ) {
        error = -EINVAL;

        goto error2;
    }

    mutex_lock( module_mutex, LOCK_IGNORE_SIGNAL );

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

        mutex_unlock( module_mutex );

        goto error2;
    }

    module = create_module( name );

    if ( module == NULL ) {
        mutex_unlock( module_mutex );

        error = -ENOMEM;

        goto error2;
    }

    module->status = MODULE_LOADING;

    hashtable_add( &module_table, ( hashitem_t* )module );

    mutex_unlock( module_mutex );

    /* Load the module */

    error = module_loader->load_module( module, loader );

    if ( error < 0 ) {
        goto error3;
    }

    found = module_loader->get_symbol( module, "init_module", ( ptr_t* )&module->init );

    if ( !found ) {
        kprintf( ERROR, "Module %s doesn't export init_module function!\n", name );
        error = -ENOENT;
        goto error4;
    }

    found = module_loader->get_symbol( module, "destroy_module", ( ptr_t* )&module->destroy );

    if ( !found ) {
        kprintf( ERROR, "Module %s doesn't export destroy_module function!\n", name );
        error = -ENOENT;
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

    mutex_lock( module_mutex, LOCK_IGNORE_SIGNAL );

    module->status = MODULE_LOADED;

    mutex_unlock( module_mutex );

    return 0;

error4:
    /* TODO: unload the module */

error3:
    mutex_lock( module_mutex, LOCK_IGNORE_SIGNAL );
    hashtable_remove( &module_table, ( const void* )name );
    mutex_unlock( module_mutex );

    destroy_module( module );

error2:
    if ( is_bootmodule ) {
        put_bootmodule_loader( loader );
    } else {
        put_file_module_loader( loader );
    }

error1:
    return error;
}

int load_module( const char* name ) {
    return do_load_module( name );
}

typedef struct module_sym_info {
    ptr_t address;
    symbol_info_t* symbol_info;
} module_sym_info_t;

static int module_sym_info_iterator( hashitem_t* item, void* data ) {
    module_t* module;
    module_sym_info_t* info;

    module = ( module_t* )item;
    info = ( module_sym_info_t* )data;

    if ( module_loader->get_symbol_info( module, info->address, info->symbol_info ) == 0 ) {
        return -1;
    }

    return 0;
}

int get_module_symbol_info( ptr_t address, symbol_info_t* info ) {
    int error;
    module_sym_info_t sym_info;

    sym_info.address = address;
    sym_info.symbol_info = info;

    error = hashtable_iterate( &module_table, module_sym_info_iterator, ( void* )&sym_info );

    if ( error == 0 ) {
        return -EINVAL;
    }

    return 0;
}

int sys_load_module( const char* name ) {
    return do_load_module( name );
}

uint32_t sys_get_module_count( void ) {
    uint32_t result;

    mutex_lock( module_mutex, LOCK_IGNORE_SIGNAL );

    result = hashtable_get_item_count( &module_table );

    mutex_unlock( module_mutex );

    return result;
}

static int get_module_info_iterator( hashitem_t* item, void* _data ) {
    module_t* module;
    module_info_t* current;
    module_info_iter_data_t* data;

    data = ( module_info_iter_data_t* )_data;

    if ( data->curr_index >= data->max_count ) {
        return 0;
    }

    module = ( module_t* )item;
    current = ( module_info_t* )&data->info_table[ data->curr_index ];

    current->dependency_count = 0;
    strncpy( current->name, module->name, MAX_MODULE_NAME_LENGTH );
    current->name[ MAX_MODULE_NAME_LENGTH - 1 ] = 0;

    data->curr_index++;

    return 0;
}

int sys_get_module_info( module_info_t* module_info, uint32_t max_count ) {
    int error;
    module_info_iter_data_t data;

    data.curr_index = 0;
    data.max_count = max_count;
    data.info_table = module_info;

    mutex_lock( module_mutex, LOCK_IGNORE_SIGNAL );

    error = hashtable_iterate( &module_table, get_module_info_iterator, ( void* )&data );

    mutex_unlock( module_mutex );

    return error;
}

void set_module_loader( module_loader_t* loader ) {
    module_loader = loader;

    kprintf( INFO, "Registered module loader: %s.\n", module_loader->name );
}

static void* module_key( hashitem_t* item ) {
    module_t* module;

    module = ( module_t* )item;

    return ( void* )module->name;
}

__init int init_module_loader( void ) {
    int error;

    module_loader = NULL;

    error = init_hashtable(
        &module_table,
        32,
        module_key,
        hash_str,
        compare_str
    );

    if ( error < 0 ) {
        goto error1;
    }

    module_mutex = mutex_create( "module mutex", MUTEX_NONE );

    if ( module_mutex < 0 ) {
        error = module_mutex;
        goto error2;
    }

    return 0;

error2:
    destroy_hashtable( &module_table );

error1:
    return error;
}
