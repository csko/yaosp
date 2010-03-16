/* DHCP client
 *
 * Copyright (c) 2010 Zoltan Kovacs
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
#ifndef _DHCPCLIENT_H_
#define _DHCPCLIENT_H_

#include <sys/types.h>
#include <yaosp/debug.h>

/* RFC: http://tools.ietf.org/html/rfc2131 */

/* Port numbers */

#define SERVER_PORT     67
#define CLIENT_PORT     68

/* Protocol related */

#define DHCP_MAGIC 0x63825363

#define BOOTREQUEST     1
#define BOOTREPLY       2

#define ETH_10MB        1
#define ETH_10MB_LEN    6

/* DHCP message types */

#define DHCPDISCOVER    1
#define DHCPOFFER       2
#define DHCPREQUEST     3
#define DHCPDECLINE     4
#define DHCPACK         5
#define DHCPNAK         6
#define DHCPRELEASE     7
#define DHCPINFORM      8

/* DHCP message structure */

typedef struct dhcp_msg {
    uint8_t op;      /* Message op code / message type. */
    uint8_t htype;   /* Hardware address type */
    uint8_t hlen;    /* Hardware address length */
    uint8_t hops;    /* Client sets to zero, optionally used by
                        relay agents when booting via a relay agent.
                     */
    uint32_t xid;    /* Transaction ID, a random number chosen by the
                       client, used by the client and server to associate
                       messages and responses between a client and a server.
                     */
    uint16_t secs;   /* Filled in by client, seconds elapsed since client
                        began address acquisition or renewal process.
                     */
    uint16_t flags;  /* Flags */
    uint32_t ciaddr; /* Client IP address; only filled in if client is in
                        BOUND, RENEW or REBINDING state and can respond
                        to ARP requests.
                     */
    uint32_t yiaddr; /* 'your' (client) IP address. */
    uint32_t siaddr; /* IP address of next server to use in bootstrap;
                        returned in DHCPOFFER, DHCPACK by server.
                     */
    uint32_t giaddr; /* Relay agent IP address, used in booting via a
                        relay agent.
                     */
    uint8_t chaddr[16];/* Client hardware address. */
    uint8_t sname[64]; /* Optional server host name, null terminated string. */
    uint8_t file[128]; /* Boot file name, null terminated string; "generic"
                          name or null in DHCPDISCOVER, fully qualified
                          directory-path name in DHCPOFFER.
                       */
    uint32_t cookie;
    uint8_t options[308]; /* Optional parameters field not including the cookie. */
} __attribute__(( packed )) dhcp_msg_t;

typedef struct dhcp_info {
    uint32_t server_addr;
    uint32_t ip_addr;
    uint32_t broadcast;
    uint32_t netmask;

    uint32_t* routers;
    size_t router_count;

    uint32_t* name_servers;

    uint32_t lease_time;
    uint32_t arp_cache_timeout;
} dhcp_info_t;

/*
 * DHCP message options
 * RFC: http://tools.ietf.org/html/rfc2132
*/

#define DHCP_MSG_TYPE       53
#define DHCP_MSG_TYPE_LEN   1

#define DHCP_SUBNET_MASK        1
#define DHCP_SUBNET_MASK_LEN    4

#define DHCP_ROUTERS        3

#define DHCP_NAME_SERVERS   5

#define DHCP_HOSTNAME       12

#define DHCP_DOMAIN_NAME    15

#define DHCP_IP_TTL         23
#define DHCP_IP_LEN         1

#define DHCP_IF_MTU         26
#define DHCP_IF_MTU_LEN     2

#define DHCP_BROADCAST      28
#define DHCP_BROADCAST_LEN  4

#define DHCP_ARP_CACHE_TIMEOUT      35
#define DHCP_ARP_CACHE_TIMEOUT_LEN  4

#define DHCP_TCP_TTL        37
#define DHCP_TCP_TTL_LEN    1

#define DHCP_TCP_KEEPALIVE      38
#define DHCP_TCP_KEEPALIVE_LEN  4

#define DHCP_REQUESTED_IP       50
#define DHCP_REQUESTED_IP_LEN   4

#define DHCP_SERVER_ADDR      54
#define DHCP_SERVER_ADDR_LEN  4

#define DHCP_REQUEST_PARAMS 55

/* DHCP options special bytes */
#define DHCP_END        255
#define DHCP_PADDING    0

#define MAC_BCAST_ADDR      ((uint8_t *) "\xff\xff\xff\xff\xff\xff")

void create_packet(dhcp_msg_t *msg, int type);
void send_discover( void );
void set_chaddr(dhcp_msg_t *msg);
int send_packet(dhcp_msg_t *msg, uint32_t source_ip, int source_port,
        uint32_t dest_ip, int dest_port, uint8_t *dest_arp);
void send_request( dhcp_msg_t *msg, dhcp_info_t *info );

#endif /* _DHCPCLIENT_H_ */
