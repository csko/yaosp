/* yaosp C library
 *
 * Copyright (c) 2009 Kornel Csernai
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

#ifndef _NETINET_IP_H_
#define _NETINET_IP_H_

#include <endian.h>

struct ip {
#if __BYTE_ORDER == __LITTLE_ENDIAN
    uint32_t ip_hl:4;                   /* header length */
    uint32_t ip_v:4;                    /* version */
#endif
#if __BYTE_ORDER == __BIG_ENDIAN
    uint32_t ip_v:4;                    /* version */
    uint32_t ip_hl:4;                   /* header length */
#endif
    uint8_t ip_tos;                     /* type of service */
    uint16_t ip_len;                    /* total length */
    uint16_t ip_id;                     /* identification */
    uint16_t ip_off;                    /* fragment offset field */
#define IP_RF 0x8000                    /* reserved fragment flag */
#define IP_DF 0x4000                    /* dont fragment flag */
#define IP_MF 0x2000                    /* more fragments flag */
#define IP_OFFMASK 0x1fff               /* mask for fragmenting bits */
    uint8_t ip_ttl;                     /* time to live */
    uint8_t ip_p;                       /* protocol */
    uint16_t ip_sum;                    /* checksum */
    struct in_addr ip_src, ip_dst;      /* source and dest address */
};

struct iphdr {
#if __BYTE_ORDER == __LITTLE_ENDIAN
    uint32_t ihl:4;
    uint32_t version:4;
#elif __BYTE_ORDER == __BIG_ENDIAN
    uint32_t version:4;
    uint32_t ihl:4;
#endif
    uint8_t tos;
    uint16_t tot_len;
    uint16_t id;
    uint16_t frag_off;
    uint8_t ttl;
    uint8_t protocol;
    uint16_t check;
    uint32_t saddr;
    uint32_t daddr;
    /* The options start here. */
};

#endif /* _NETINET_IP_H_ */
