/* 32bit ELF context handling
 *
 * Copyright (c) 2010 Zoltan Kovacs
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
#include <console.h>
#include <smp.h>
#include <vfs/vfs.h>
#include <mm/kmalloc.h>
#include <linker/elf32.h>

static binary_loader_t* elf32_get_image_loader( char* filename ) {
    int fd;
    char path[256];

    snprintf( path, sizeof(path), "/yaosp/system/lib/%s", filename );

    fd = sys_open( path, O_RDONLY );

    if ( fd < 0 ) {
        return NULL;
    }

    return get_app_binary_loader( filename, fd );
}

static int elf32_image_init( elf32_image_t* image ) {
    memset( image, 0, sizeof( elf32_image_t ) );

    return 0;
}

static int elf32_image_destroy( elf32_image_t* image ) {
    uint32_t i;

    for ( i = 0; i < image->info.needed_count; i++ ) {
        elf32_image_destroy( &image->subimages[i] );
    }

    elf32_destroy_image_info( &image->info );
    memory_region_put( image->text_region );
    memory_region_put( image->data_region );
    kfree( image->subimages );

    return 0;
}

static int elf32_image_map( elf32_image_t* image, binary_loader_t* loader, elf_binary_type_t type ) {
    uint32_t i;
    elf_section_header_t* section_header;

    bool text_found = false;
    uint32_t text_start = 0;
    uint32_t text_end = 0;
    uint32_t text_size;
    uint32_t text_offset = 0;

    bool data_found = false;
    uint32_t data_start = 0;
    uint32_t data_end = 0;
    uint32_t data_size;
    uint32_t data_offset = 0;

    uint32_t bss_end = 0;
    uint32_t data_size_with_bss;

    for ( i = 0; i < image->info.header.shnum; i++ ) {
        section_header = &image->info.section_headers[i];

        /* Check if the current section occupies memory during execution */

        if ( section_header->flags & SF_ALLOC ) {
            if ( section_header->flags & SF_WRITE ) {
                if ( !data_found ) {
                    data_found = true;
                    data_start = section_header->address;
                    data_offset = section_header->offset;
                }

                switch ( section_header->type ) {
                    case SHT_NOBITS :
                        bss_end = section_header->address + section_header->size - 1;
                        break;

                    default :
                        data_end = section_header->address + section_header->size - 1;
                        bss_end = data_end;
                        break;
                }
            } else {
                if ( !text_found ) {
                    text_found = true;
                    text_start = section_header->address;
                    text_offset = section_header->offset;
                }

                text_end = section_header->address + section_header->size - 1;
            }
        }
    }

    if ( !text_found ) {
        return -EINVAL;
    }

    text_offset -= ( text_start & ~PAGE_MASK );
    text_start &= PAGE_MASK;
    text_size = text_end - text_start + 1;

    data_offset -= ( data_start & ~PAGE_MASK );
    data_start &= PAGE_MASK;
    data_size = data_end - data_start + 1;
    data_size_with_bss = bss_end - data_start + 1;

    if ( ( type == ELF_APPLICATION ) ||
         ( type == ELF_LIBRARY ) ) {
        char name[128];
        uint32_t flags;

        snprintf( name, sizeof(name), "%s_ro", loader->get_name( loader->private ) );
        flags = REGION_READ | REGION_EXECUTE;

        if ( type == ELF_LIBRARY ) {
            flags |= REGION_CALL_FROM_USERSPACE; /* todo */
        }

        image->text_region = memory_region_create(
            name, PAGE_ALIGN( text_size ), flags
        );

        if ( image->text_region == NULL ) {
            return -1;
        }

        memory_region_map_to_file(
            image->text_region, loader->get_fd( loader->private ),
            text_offset, text_size
        );
    } else if ( type == ELF_KERNEL_MODULE ) {
        /* todo */
    }

    if ( data_found > 0 ) {
        if ( ( type == ELF_APPLICATION ) ||
             ( type == ELF_LIBRARY ) ) {
            char name[128];
            uint32_t flags;

            snprintf( name, sizeof(name), "%s_rw", loader->get_name( loader->private ) );
            flags = REGION_READ | REGION_WRITE;

            if ( type == ELF_LIBRARY ) {
                flags |= REGION_CALL_FROM_USERSPACE; /* todo */
            }

            image->data_region = memory_region_create(
                name, PAGE_ALIGN( data_size_with_bss ), flags
            );

            if ( image->data_region == NULL ) {
                /* TODO: delete text region */
                return -1;
            }

            if ( ( data_end != 0 ) && ( data_size > 0 ) ) {
                memory_region_map_to_file(
                    image->data_region, loader->get_fd( loader->private ),
                    data_offset, data_size
                );
            } else {
                kprintf( WARNING, "%s(): nothing has been mapped to data region!\n", __FUNCTION__ );
            }
        } else if ( type == ELF_KERNEL_MODULE ) {
            /* todo */
        }
    }

    return 0;
}

int elf32_image_load( elf32_image_t* image, binary_loader_t* loader, ptr_t virtual_address, elf_binary_type_t type ) {
    int error;
    uint32_t i;
    uint16_t elf_type;

    switch ( type ) {
        case ELF_KERNEL_MODULE : elf_type = ELF_TYPE_DYN; break;
        case ELF_APPLICATION : elf_type = ELF_TYPE_EXEC; break;
        case ELF_LIBRARY : elf_type = ELF_TYPE_DYN; break;
        default : return -EINVAL;
    }

    memset( image, 0, sizeof( elf32_image_t ) );

    error = elf32_init_image_info( &image->info, virtual_address );

    if ( error != 0 ) {
        goto error1;
    }

    error = elf32_load_and_validate_header( &image->info, loader, elf_type );

    if ( error != 0 ) {
        goto error2;
    }

    error = elf32_load_section_headers( &image->info, loader );

    if ( error != 0 ) {
        goto error2;
    }

    error = elf32_parse_section_headers( &image->info, loader );

    if ( error != 0 ) {
        goto error2;
    }

    error = elf32_image_map( image, loader, type );

    if ( error != 0 ) {
        goto error2;
    }

    elf32_free_section_headers( &image->info );

    /* Load the required subimages. */

    if ( image->info.needed_count > 0 ) {
        image->subimages = ( elf32_image_t* )kmalloc( sizeof( elf32_image_t ) * image->info.needed_count );

        if ( image->subimages == NULL ) {
            goto error2;
        }

        for ( i = 0; i < image->info.needed_count; i++ ) {
            int fd;
            binary_loader_t* img_loader = elf32_get_image_loader( image->info.needed_table[i] );

            if ( img_loader == NULL ) {
                kprintf( WARNING, "%s(): unable to find binary loader for %s.\n",
                         __FUNCTION__, image->info.needed_table[i] );
                error = -EINVAL;
                goto error2; /* todo: better error handling. */
            }

            error = elf32_image_load( &image->subimages[i], img_loader, 0x00000000, ELF_LIBRARY );

            fd = img_loader->get_fd( img_loader->private );
            put_app_binary_loader( img_loader );
            sys_close( fd );

            if ( error != 0 ) {
                goto error2; /* todo: better error handling. */
            }
        }
    }

    return 0;

 error2:
    elf32_destroy_image_info( &image->info );

 error1:
    return error;
}

int elf32_context_init( elf32_context_t* context, elf32_relocate_t* relocate ) {
    int error;

    error = array_init( &context->libraries );

    if ( error != 0 ) {
        goto error1;
    }

    context->lock = mutex_create( "ELF32 context", MUTEX_NONE );

    if ( context->lock < 0 ) {
        goto error2;
    }

    array_set_realloc_size( &context->libraries, 32 );

    context->relocate = relocate;
    elf32_image_init( &context->main );

    return 0;

 error2:
    array_destroy( &context->libraries );

 error1:
    return error;
}

int elf32_context_destroy( elf32_context_t* context ) {
    int i;
    int size;

    size = array_get_size( &context->libraries );

    for ( i = 0; i < size; i++ ) {
        elf32_image_t* image;

        image = ( elf32_image_t* )array_get_item( &context->libraries, i );
        elf32_image_destroy( image );
    }

    elf32_image_destroy( &context->main );
    array_destroy( &context->libraries );
    mutex_destroy( context->lock );

    return 0;
}

static int elf32_context_get_symbol_helper( elf32_image_t* image, const char* name, int dynsym_only,
                                            elf32_image_t** img, void** sym ) {
    uint32_t i;
    void* symbol;
    elf32_image_info_t* info;

    info = &image->info;

    if ( dynsym_only ) {
        symbol = ( void* )elf32_get_symbol( info, name );
    } else {
        symbol = ( void* )elf32_get_symbol2( info, name );
    }

    if ( symbol != NULL ) {
        *img = image;
        *sym = symbol;

        return 0;
    }

    for ( i = 0; i < info->needed_count; i++ ) {
        if ( elf32_context_get_symbol_helper( &image->subimages[i], name, dynsym_only, img, sym ) == 0 ) {
            return 0;
        }
    }

    return -ENOENT;
}

int elf32_context_get_symbol( elf32_context_t* context, const char* name, int skip_main, int dynsym_only,
                              elf32_image_t** image, void** symbol ) {
    int i;
    int size;
    int error = -EINVAL;

    if ( skip_main ) {
        uint32_t j;
        elf32_image_info_t* info;

        info = &context->main.info;

        for ( j = 0; j < info->needed_count; j++ ) {
            error = elf32_context_get_symbol_helper( &context->main.subimages[j], name, dynsym_only, image, symbol );

            if ( error == 0 ) {
                return 0;
            }
        }
    } else {
        error = elf32_context_get_symbol_helper( &context->main, name, dynsym_only, image, symbol );

        if ( error == 0 ) {
            return 0;
        }
    }

    size = array_get_size( &context->libraries );

    for ( i = 0; i < size; i++ ) {
        elf32_image_t* library;

        library = ( elf32_image_t* )array_get_item( &context->libraries, i );

        error = elf32_context_get_symbol_helper( library, name, dynsym_only, image, symbol );

        if ( error == 0 ) {
            break;
        }
    }

    return error;
}

static binary_loader_t* get_dlopen_loader( const char* path ) {
    int fd;
    char* filename;

    /* Extract the filename from the full path */

    filename = strrchr( path, '/' );

    if ( filename != NULL ) {
        filename++;
    }

    fd = sys_open( path, O_RDONLY );

    if ( fd < 0 ) {
        return NULL;
    }

    return get_app_binary_loader( filename, fd );
}

void* sys_dlopen( const char* filename, int flag ) {
    int error;
    elf32_image_t* image;
    elf32_context_t* context;
    binary_loader_t* loader;

    context = ( elf32_context_t* )current_process()->loader_data;
    image = ( elf32_image_t* )kmalloc( sizeof( elf32_image_t ) );

    if ( image == NULL ) {
        goto error1;
    }

    elf32_image_init( image );

    loader = get_dlopen_loader( filename );

    if ( loader == NULL ) {
        goto error2;
    }

    error = elf32_image_load( image, loader, 0x00000000, ELF_LIBRARY );

    if ( error != 0 ) {
        goto error3;
    }

    mutex_lock( context->lock, LOCK_IGNORE_SIGNAL );

    array_add_item( &context->libraries, image );
    error = context->relocate( context, image );

    mutex_unlock( context->lock );

    if ( error != 0 ) {
        goto error4;
    }

    return ( void* )image;

 error4:
    /* todo */

 error3:
    /* todo: close fd! */
    put_app_binary_loader( loader );

 error2:
    kfree( image );

 error1:
    return NULL;
}

int sys_dlclose( void* handle ) {
    int index;
    int error = 0;
    elf32_image_t* image;
    elf32_context_t* context;

    context = ( elf32_context_t* )current_process()->loader_data;

    mutex_lock( context->lock, LOCK_IGNORE_SIGNAL );

    index = array_index_of( &context->libraries, handle );

    if ( index < 0 ) {
        error = -EINVAL;
        goto out;
    }

    array_remove_item_from( &context->libraries, index );

 out:
    mutex_unlock( context->lock );

    if ( error == 0 ) {
        image = ( elf32_image_t* )handle;
        elf32_image_destroy( image );
        kfree( image );
    }

    return error;
}

void* sys_dlsym( void* handle, const char* symname ) {
    void* ptr = NULL;
    elf32_image_t* image;
    my_elf_symbol_t* symbol;
    elf32_context_t* context;

    context = ( elf32_context_t* )current_process()->loader_data;

    mutex_lock( context->lock, LOCK_IGNORE_SIGNAL );

    if ( array_index_of( &context->libraries, handle ) < 0 ) {
        goto out;
    }

    image = ( elf32_image_t* )handle;
    symbol = elf32_get_symbol( &image->info, symname );

    if ( symbol == NULL ) {
        goto out;
    }

    ptr = ( void* )( image->text_region->address + symbol->address - image->info.virtual_address );

 out:
    mutex_unlock( context->lock );

    return ptr;
}

int sys_dlgetglobalinit(void* handle, uint32_t* table, int size) {
    int ret;
    uint32_t i;
    elf32_image_t* image;
    elf32_context_t* context;

    if (size == 0) {
        return 0;
    }

    context = (elf32_context_t*)current_process()->loader_data;

    mutex_lock(context->lock, LOCK_IGNORE_SIGNAL);

    if (array_index_of(&context->libraries, handle) < 0) {
        ret = -EINVAL;
        goto out;
    }

    ret = 0;
    image = (elf32_image_t*)handle;

    for (i = 0; i < image->info.symbol_count; i++) {
        elf_symbol_t* symbol = &image->info.symbol_table[i];

        if (strncmp(image->info.string_table + symbol->name, "_GLOBAL__I__", 12) == 0) {
            table[ret] = image->text_region->address + symbol->value - image->info.virtual_address;

            if (++ret == size) {
                break;
            }
        }
    }

out:
    mutex_unlock(context->lock);

    return ret;
}
