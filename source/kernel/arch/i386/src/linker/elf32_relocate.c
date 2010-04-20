/* i386 ELF loader
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

#include <console.h>
#include <errno.h>
#include <lib/string.h>

#include <arch/linker/elf32.h>

static int do_elf32_relocate_i386( elf32_context_t* context, elf32_image_t* image ) {
    uint32_t i;
    elf_reloc_t* reloc;
    my_elf_symbol_t* symbol_table;

    symbol_table = image->info.dyn_symbol_table;

    for ( i = 0, reloc = &image->info.reloc_table[0]; i < image->info.reloc_count; i++, reloc++ ) {
        uint32_t* target;
        my_elf_symbol_t* symbol;

        symbol = &symbol_table[ ELF32_R_SYM( reloc->info ) ];
        target = ( uint32_t* )( image->text_region->address + reloc->offset - image->info.virtual_address );

        switch ( ELF32_R_TYPE( reloc->info ) ) {
            case R_386_32 : {
                elf32_image_t* img;
                my_elf_symbol_t* sym;

                if ( elf32_context_get_symbol( context, symbol->name, &img, &sym ) != 0 ) {
                    kprintf( ERROR, "Symbol %s not found.\n", symbol->name );
                    return -ENOENT;
                }

                *target = *target + img->text_region->address + sym->address - img->info.virtual_address;

                break;
            }

            case R_386_PC32 : {
                elf32_image_t* img;
                my_elf_symbol_t* sym;

                if ( elf32_context_get_symbol( context, symbol->name, &img, &sym ) != 0 ) {
                    kprintf( ERROR, "Symbol %s not found.\n", symbol->name );
                    return -ENOENT;
                }

                *target = *target + img->text_region->address + sym->address - img->info.virtual_address - ( uint32_t )target;

                break;
            }

            case R_386_COPY :
                /* This will be handled by the initialization part of the C library. */
                break;

            case R_386_JMP_SLOT : {
                elf32_image_t* img;
                my_elf_symbol_t* sym;

                if ( elf32_context_get_symbol( context, symbol->name, &img, &sym ) != 0 ) {
                    kprintf( ERROR, "Symbol %s not found.\n", symbol->name );
                    return -ENOENT;
                }

                *target = img->text_region->address + sym->address - img->info.virtual_address;

                break;
            }

            case R_386_RELATIVE :
                *target += image->text_region->address;

                break;

            default :
                kprintf(
                    ERROR, "elf32_relocate_i386(): Unknown reloc type (%d) for symbol: %s.\n",
                    ELF32_R_TYPE( reloc->info ), symbol->name
                );
                return -EINVAL;
        }
    }

    return 0;
}

int elf32_relocate_i386( elf32_context_t* context, elf32_image_t* image ) {
    int error;
    uint32_t i;

    for ( i = 0; i < image->info.needed_count; i++ ) {
        error = elf32_relocate_i386( context, &image->subimages[i] );

        if ( error != 0 ) {
            return error;
        }
    }

    return do_elf32_relocate_i386( context, image );
}

static uint32_t do_elf32_insert_copy_information( elf32_context_t* context, elf32_image_t* image, void** _address ) {
    uint32_t i;
    uint32_t count = 0;
    elf_reloc_t* reloc;
    elf32_i386_copy_info_t* info;
    my_elf_symbol_t* symbol_table;

    for ( i = 0; i < image->info.needed_count; i++ ) {
        count += do_elf32_insert_copy_information( context, &image->subimages[i], _address );
    }

    info = ( elf32_i386_copy_info_t* )*_address;
    symbol_table = image->info.dyn_symbol_table;

    for ( i = 0, reloc = &image->info.reloc_table[0]; i < image->info.reloc_count; i++, reloc++ ) {
        uint32_t* target;
        my_elf_symbol_t* symbol;

        symbol = &symbol_table[ ELF32_R_SYM( reloc->info ) ];
        target = ( uint32_t* )( image->text_region->address + reloc->offset - image->info.virtual_address );

        switch ( ELF32_R_TYPE( reloc->info ) ) {
            case R_386_COPY : {
                elf32_image_t* img;
                my_elf_symbol_t* sym;

                if ( elf32_context_get_symbol( context, symbol->name, &img, &sym ) != 0 ) {
                    kprintf( ERROR, "Symbol %s not found.\n", symbol->name );
                    return -ENOENT;
                }

                info->to = ( uint32_t )target;
                info->from = img->text_region->address + sym->address - img->info.virtual_address;
                info->size = sym->size;

                count++;
                info++;

                break;
            }
        }
    }

    *_address = ( void* )info;

    return count;
}

int elf32_insert_copy_information( elf32_context_t* context, ptr_t address ) {
    void* addr;
    uint32_t count;

    addr = ( void* )( address + sizeof( uint32_t ) );
    count = do_elf32_insert_copy_information( context, &context->main, &addr );
    *( uint32_t* )address = count;

    return 0;
}
