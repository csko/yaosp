/* FPU state save and restore functions
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

#include <arch/fpu.h>

void load_fpu_state( fpu_state_t* fpu_state ) {
    __asm__ __volatile__(
        "frstor %0\n"
        :
        : "m" ( fpu_state->fsave_data )
    );
}

void save_fpu_state( fpu_state_t* fpu_state ) {
    __asm__ __volatile__(
        "fnsave %0\n"
        "fwait\n"
        : "=m" ( fpu_state->fsave_data )
    );
}
