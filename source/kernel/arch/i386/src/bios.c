/* BIOS access
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

#include <kernel.h>
#include <console.h>
#include <errno.h>
#include <mm/region.h>
#include <lib/string.h>

#include <arch/io.h>
#include <arch/bios.h>
#include <arch/cpu.h>

#include "x86emu/x86emu.h"

static uint8_t* below_1mb = NULL;
static memory_region_t* below_1mb_region = NULL;

static u8 x86emu_rdb( u32 address ) {
    return *( below_1mb + address );
}

static u16 x86emu_rdw( u32 address ) {
    return *( ( uint16_t* )( below_1mb + address ) );
}

static u32 x86emu_rdl( u32 address ) {
    return *( ( u32* )( below_1mb + address ) );
}

static void x86emu_wrb( u32 address, u8 value ) {
    *( below_1mb + address ) = value;
}

static void x86emu_wrw( u32 address, u16 value ) {
    *( ( u16* )( below_1mb + address ) ) = value;
}

static void x86emu_wrl( u32 address, u32 value ) {
    *( ( u32* )( below_1mb + address ) ) = value;
}

static X86EMU_memFuncs x86emu_mem_funcs = {
    .rdb = x86emu_rdb,
    .rdw = x86emu_rdw,
    .rdl = x86emu_rdl,
    .wrb = x86emu_wrb,
    .wrw = x86emu_wrw,
    .wrl = x86emu_wrl
};

static uint8_t x86emu_inb( uint16_t port ) {
    return inb( port );
}

static void x86emu_outb( uint16_t port, uint8_t data ) {
    outb( data, port );
}

static uint16_t x86emu_inw( uint16_t port ) {
    return inw( port );
}

static void x86emu_outw( uint16_t port, uint16_t data ) {
    outw( data, port );
}

static unsigned long x86emu_inl( uint16_t port ) {
    return inl( port );
}

static void x86emu_outl( uint16_t port, unsigned long data ) {
    outl( data, port );
}

static X86EMU_pioFuncs x86emu_pio_funcs = {
    .inb  = x86emu_inb,
    .outb = x86emu_outb,
    .inw  = x86emu_inw,
    .outw = x86emu_outw,
    .inl  = x86emu_inl,
    .outl = x86emu_outl,
};

int call_bios_interrupt( int num, bios_regs_t* regs ) {
    /* Clear everything */

    memset( &M, 0, sizeof( M ) );

    /* Copy registers from the caller */

    M.x86.R_AX = regs->ax;
    M.x86.R_BX = regs->bx;
    M.x86.R_CX = regs->cx;
    M.x86.R_DX = regs->dx;
    M.x86.R_SI = regs->si;
    M.x86.R_DI = regs->di;
    M.x86.R_DS = regs->ds;
    M.x86.R_ES = regs->es;

    /* Setup our own halt code at 0x1000 */

    *( ( uint8_t* )( below_1mb + 0x1000 ) ) = 0xF4; /* hlt instruction */

    M.x86.R_CS = 0x0000;
    M.x86.R_IP = 0x1000;

    /* Setup the stack */

    M.x86.R_SS = 0x0000;
    M.x86.R_SP = 0xFFFE;

    /* Other stuffs */

    M.x86.R_EFLG = EFLAG_IF | 0x3000;

    X86EMU_prepareForInt( num );
    X86EMU_exec();

    /* Copy the registers to the caller */

    regs->ax = M.x86.R_AX;
    regs->bx = M.x86.R_BX;
    regs->cx = M.x86.R_CX;
    regs->dx = M.x86.R_DX;
    regs->si = M.x86.R_SI;
    regs->di = M.x86.R_DI;
    regs->ds = M.x86.R_DS;
    regs->es = M.x86.R_ES;

    return 0;
}

__init int init_bios_access( void ) {
    below_1mb_region = do_create_memory_region(
        &kernel_memory_context,
        "below 1mb",
        1 * 1024 * 1024,
        REGION_READ | REGION_WRITE | REGION_KERNEL
    );

    if ( below_1mb_region == NULL ) {
        return -ENOMEM;
    }

    do_memory_region_remap_pages( below_1mb_region, 0x0 );
    memory_region_insert( &kernel_memory_context, below_1mb_region );

    below_1mb = ( uint8_t* )below_1mb_region->address;

    X86EMU_setupMemFuncs( &x86emu_mem_funcs );
    X86EMU_setupPioFuncs( &x86emu_pio_funcs );

    return 0;
}
