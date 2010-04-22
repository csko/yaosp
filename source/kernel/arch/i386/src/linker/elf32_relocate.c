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

                if ( elf32_context_get_symbol( context, symbol->name, 0, &img, &sym ) != 0 ) {
                    kprintf( ERROR, "Symbol %s not found.\n", symbol->name );
                    return -ENOENT;
                }

                *target = *target + img->text_region->address + sym->address - img->info.virtual_address;

                break;
            }

            case R_386_PC32 : {
                elf32_image_t* img;
                my_elf_symbol_t* sym;

                if ( elf32_context_get_symbol( context, symbol->name, 0, &img, &sym ) != 0 ) {
                    kprintf( ERROR, "Symbol %s not found.\n", symbol->name );
                    return -ENOENT;
                }

                *target = *target +
                    img->text_region->address + sym->address - img->info.virtual_address - ( uint32_t )target;

                break;
            }

            case R_386_COPY : {
                elf32_image_t* img;
                my_elf_symbol_t* sym;

                if ( elf32_context_get_symbol( context, symbol->name, 1, &img, &sym ) != 0 ) {
                    kprintf( ERROR, "Symbol %s not found.\n", symbol->name );
                    return -ENOENT;
                }

                ASSERT( sym->size == 4 );
                *target = *( uint32_t* )( img->text_region->address + sym->address - img->info.virtual_address );

                break;
            }

            case R_386_JMP_SLOT : {
                elf32_image_t* img;
                my_elf_symbol_t* sym;

                if ( elf32_context_get_symbol( context, symbol->name, 0, &img, &sym ) != 0 ) {
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
