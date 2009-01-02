/* i386 specific part of the system call handler
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

#include <types.h>
#include <syscall.h>

void system_call_entry( registers_t* regs ) {
    uint32_t parameters[ 5 ] = {
        regs->ebx,
        regs->ecx,
        regs->edx,
        regs->esi,
        regs->edi
    };

    regs->eax = handle_system_call( regs->eax, parameters, ( void* )regs );
}
