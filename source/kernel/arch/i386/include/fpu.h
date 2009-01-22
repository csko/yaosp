/* FPU structure definitions
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

/* The FPU structures are based on the information found on these sites:
 *   - http://www.website.masmforum.com/tutorials/fptute/fpuchap3.htm#fsave
 *   - http://www.rz.uni-karlsruhe.de/rz/docs/VTune/reference/vc129.htm#Layout
 */

#ifndef _ARCH_FPU_H_
#define _ARCH_FPU_H_

#include <types.h>

typedef struct fsave_data {
    uint16_t control;
    uint16_t unused1;
    uint16_t status;
    uint16_t unused2;
    uint16_t tag;
    uint16_t unused3;
    uint32_t instruction_pointer;
    uint16_t code_segment;
    uint16_t unused4;
    uint32_t operand_address;
    uint16_t data_segment;
    uint16_t unused5;
    uint8_t st_registers[ 80 ];
} fsave_data_t;

typedef struct fxsave_data {
    uint16_t fcw;
    uint16_t fsw;
    uint16_t ftw;
    uint16_t fop;
    uint32_t ip;
    uint16_t cs;
    uint16_t reserved1;
    uint32_t dp;
    uint16_t ds;
    uint16_t reserved2;
    uint32_t mxcsr;
    uint32_t reserved3;
    uint8_t st_registers[ 128 ];
    uint8_t xmm_registers[ 128 ];
    uint8_t reserved4[ 224 ];
} fxsave_data_t;

typedef union fpu_state {
    fsave_data_t fsave_data;
    fxsave_data_t fxsave_data;
} fpu_state_t;

void load_fpu_state( fpu_state_t* fpu_state );
void save_fpu_state( fpu_state_t* fpu_state );

#endif // _ARCH_FPU_H_
