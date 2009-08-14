/* Common ELF format definitions
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

#include <linker/elf.h>

bool elf_check_header( elf_header_t* header, uint8_t class, uint8_t data, uint16_t type, uint16_t machine ) {
    return ( ( header->ident[ ID_MAGIC0 ] == ELF_MAGIC0 ) &&
             ( header->ident[ ID_MAGIC1 ] == ELF_MAGIC1 ) &&
             ( header->ident[ ID_MAGIC2 ] == ELF_MAGIC2 ) &&
             ( header->ident[ ID_MAGIC3 ] == ELF_MAGIC3 ) &&
             ( header->ident[ ID_CLASS ] == class ) &&
             ( header->ident[ ID_DATA ] == data ) &&
             ( header->type == type ) &&
             ( header->machine == machine ) );

#if 0
    if ( ( header->ident[ ID_MAGIC0 ] != ELF32_MAGIC0 ) ||
         ( header->ident[ ID_MAGIC1 ] != ELF32_MAGIC1 ) ||
         ( header->ident[ ID_MAGIC2 ] != ELF32_MAGIC2 ) ||
         ( header->ident[ ID_MAGIC3 ] != ELF32_MAGIC3 ) ) {
        return false;
    }

    if ( header->ident[ ID_CLASS ] != ELF_CLASS_32 ) {
        return false;
    }

    if ( header->ident[ ID_DATA ] != ELF_DATA_2_LSB ) {
        return false;
    }

    if ( header->machine != ELF_EM_386 ) {
        return false;
    }

    return true;
#endif
}
