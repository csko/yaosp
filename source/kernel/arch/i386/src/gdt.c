/* Global Descriptor Table handling
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

#include <arch/gdt.h>

gdt_descriptor_t gdt[ GDT_ENTRIES ] = {
  /* NULL descriptor */
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  /* Kernel CS */
  { 0xFFFF, 0, 0, 0x9A, 0xF, 0, 0, 1, 1, 0 },
  /* Kernel DS */
  { 0xFFFF, 0, 0, 0x92, 0xF, 0, 0, 1, 1, 0 },
  /* User CS */
  { 0xFFFF, 0, 0, 0xFE, 0xF, 0, 0, 1, 1, 0 },
  /* User DS */
  { 0xFFFF, 0, 0, 0xF2, 0xF, 0, 0, 1, 1, 0 },
};

int init_gdt( void ) {
    gdt_t gdtp;

    gdtp.size = sizeof( gdt ) - 1;
    gdtp.base = ( uint32_t )&gdt;

    __asm__ __volatile__(
        "lgdt %0\n"
        :
        : "m" ( gdtp )
    );

    reload_segment_descriptors();

    return 0;
}
