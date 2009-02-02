/* ISO9660 filesystem driver
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

#ifndef _ROCKRIDGE_H_
#define _ROCKRIDGE_H_

#include <types.h>

#define RR_MAKE_TAG(c1,c2) (((c2)<<8)|(c1))

#define RR_TAG_PX RR_MAKE_TAG('P','X')
#define RR_TAG_PN RR_MAKE_TAG('P','N')
#define RR_TAG_SL RR_MAKE_TAG('S','L')
#define RR_TAG_NM RR_MAKE_TAG('N','M')
#define RR_TAG_CL RR_MAKE_TAG('C','L')
#define RR_TAG_PL RR_MAKE_TAG('P','L')
#define RR_TAG_RE RR_MAKE_TAG('R','E')
#define RR_TAG_TF RR_MAKE_TAG('T','F')
#define RR_TAG_RR RR_MAKE_TAG('R','R')

#define RR_NM_CONTINUE 0x01

typedef struct rr_header {
    uint16_t tag;
    uint8_t length;
} __attribute__(( packed )) rr_header_t;

typedef struct rr_nm_data {
    uint8_t version;
    uint8_t flags;
} __attribute__(( packed )) rr_nm_data_t;

#endif /* _ROCKRIDGE_H_ */
