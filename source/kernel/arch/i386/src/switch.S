/* Thread switching code
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

.section .text

.global switch_to_thread

/* void switch_to_thread( register_t esp ) */

switch_to_thread:
    movl 4(%esp), %esp
    pop %fs
    pop %es
    pop %ds
    popa
    addl $8, %esp /* skip int_number and error_code */
    iret
.size switch_to_thread,.-switch_to_thread
