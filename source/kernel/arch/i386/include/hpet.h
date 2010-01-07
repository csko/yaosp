/* High Precision Event Timer
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

#ifndef _I386_HPET_H_
#define _I386_HPET_H_

#include <types.h>

#define HPET_ID      0x000
#define HPET_PERIOD  0x004
#define HPET_CONFIG  0x010
#define HPET_STATUS  0x020
#define HPET_COUNTER 0x0F0

#define HPET_CONFIG_ENABLE 0x001

#define HPET_MIN_PERIOD 100000UL
#define HPET_MAX_PERIOD 100000000UL

extern int hpet_present;
extern uint32_t hpet_address;

uint32_t hpet_readl( uint32_t reg );

int hpet_init( void );

#endif /* _I386_HPET_H_ */
