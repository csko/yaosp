/* DHCP client
 *
 * Copyright (c) 2009 Kornel Csernai, Zoltan Kovacs
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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <time.h>

#include "dhcpclient.h"

static char* argv0 = NULL;
static char* device = NULL;

static int sock = -1;
static char in_buffer[ 8192 ];
static uint8_t my_hw_addr[16];

enum {DISCOVER, REQUEST, DONE} status;

/* These functions are from ifconfig.c
 * TODO: We should use a common library instead.
*/
static int if_set_ip_address( struct ifreq* req, uint32_t ip ) {
    struct sockaddr_in* addr;

    addr = ( struct sockaddr_in* )&req->ifr_ifru.ifru_addr;
    addr->sin_addr.s_addr = ip;

    if ( ioctl( sock, SIOCSIFADDR, req ) != 0 ) {
        fprintf( stderr, "%s: failed to set IP address.\n", argv0 );
    }

    return 0;
}

static int if_set_netmask( struct ifreq* req, uint32_t netmask ) {
    struct sockaddr_in* addr;

    addr = ( struct sockaddr_in* )&req->ifr_ifru.ifru_netmask;
    addr->sin_addr.s_addr = netmask;

    if ( ioctl( sock, SIOCSIFNETMASK, req ) != 0 ) {
        fprintf( stderr, "%s: failed to set network mask.\n", argv0 );
    }

    return 0;
}

static int if_set_broadcast( struct ifreq* req, uint32_t broadcast ) {
    struct sockaddr_in* addr;

    addr = ( struct sockaddr_in* )&req->ifr_ifru.ifru_broadaddr;
    addr->sin_addr.s_addr = broadcast;

    if ( ioctl( sock, SIOCSIFBRDADDR, req ) != 0 ) {
        fprintf( stderr, "%s: failed to set broadcast address.\n", argv0 );
    }

    return 0;
}


void init_packet( dhcp_msg_t* msg, uint8_t type ) {
    memset( msg, 0, sizeof( dhcp_msg_t ) );

    switch ( type ) {
        case DHCPDISCOVER :
        case DHCPREQUEST :
        case DHCPRELEASE :
        case DHCPINFORM :
            msg->op = BOOTREQUEST;
            break;
    }

    msg->htype = ETH_10MB;
    msg->hlen = ETH_10MB_LEN;
    msg->cookie = htonl(DHCP_MAGIC);

    /* Set the chaddr field */
    memcpy( msg->chaddr, my_hw_addr, 6 );

    /* The required DHCP message type option and the ending byte */

    int i = 0;
    msg->options[ i++ ] = DHCP_MSG_TYPE;
    msg->options[ i++ ] = DHCP_MSG_TYPE_LEN;
    msg->options[ i++ ] = type;
    msg->options[ i ] = DHCP_END;

    // TODO: client ID, FQDN, hostname, vendor
}

int send_packet( dhcp_msg_t* msg, uint32_t source_ip, int source_port,
                 uint32_t dest_ip, int dest_port, uint8_t* dest_arp) {
    int ret;
    struct sockaddr_in dest;

    if ( sock < 0 ) {
        return -1;
    }

    dest.sin_family = AF_INET;
    dest.sin_addr.s_addr = dest_ip;
    dest.sin_port = htons( dest_port );

    ret = sendto(
        sock, msg, sizeof( dhcp_msg_t ), MSG_NOSIGNAL,
        ( struct sockaddr* )&dest, sizeof( struct sockaddr_in )
    );

//    printf( "%s: sendto() returned: %d.\n", argv0, ret );

    return 0;
}

int create_socket( void ) {
    struct sockaddr_in addr;

    sock = socket( AF_INET, SOCK_DGRAM, 0 );

    if ( sock < 0 ) {
        goto error1;
    }

    memset( &addr, 0, sizeof( struct sockaddr_in ) );

    addr.sin_family = AF_INET;
    addr.sin_port = htons( CLIENT_PORT );

    if ( bind( sock, ( struct sockaddr* )&addr, sizeof( struct sockaddr_in ) ) != 0 ) {
        goto error2;
    }

    if ( setsockopt( sock, SOL_SOCKET, SO_BINDTODEVICE, device, strlen( device ) ) != 0 ) {
        goto error2;
    }

    return 0;

 error2:
    close( sock );
    sock = -1;

 error1:
    return -1;
}

void send_discover( void ) {
    dhcp_msg_t msg;

    init_packet( &msg, DHCPDISCOVER );

    // TODO: register packet

    /* Set the random transaction ID */
    srand(time(0));
    msg.xid = rand();

    printf( "%s: sending DISCOVER.\n", argv0 );

    send_packet(
        &msg, INADDR_ANY, CLIENT_PORT, INADDR_BROADCAST,
        SERVER_PORT, MAC_BCAST_ADDR
    );
}

void send_request( dhcp_msg_t *msg, dhcp_info_t *info) {
    dhcp_msg_t reply;

    init_packet(&reply, DHCPREQUEST);

    reply.xid = msg->xid;

    int i = 3;

    reply.options[ i++ ] = DHCP_REQUESTED_IP;
    reply.options[ i++ ] = DHCP_REQUESTED_IP_LEN;
    memcpy(&(reply.options[ i ]), &(msg->yiaddr), DHCP_REQUESTED_IP_LEN);
    i += DHCP_REQUESTED_IP_LEN;

    reply.options[ i++ ] = DHCP_SERVER_ADDR;
    reply.options[ i++ ] = DHCP_SERVER_ADDR_LEN;
    memcpy(&(reply.options[ i ]), &(info->server_addr), DHCP_SERVER_ADDR_LEN);
    i += DHCP_SERVER_ADDR_LEN;

    /* Request the broadcast, router, name server addresses */

    reply.options[ i++ ] = DHCP_REQUEST_PARAMS;
    reply.options[ i++ ] = 3;
    reply.options[ i++ ] = DHCP_BROADCAST;
    reply.options[ i++ ] = DHCP_ROUTERS;
    reply.options[ i++ ] = DHCP_NAME_SERVERS;

    reply.options[ i ] = DHCP_END;

    // TODO: register request

    printf( "%s: sending REQUEST.\n", argv0 );

    send_packet(
        &reply, INADDR_ANY, CLIENT_PORT, INADDR_BROADCAST,
        SERVER_PORT, MAC_BCAST_ADDR
    );
}

void parse_message(dhcp_msg_t *msg, dhcp_info_t *info){
    uint8_t *options;
    uint8_t msgtype = 255;
    int broadcast_set = 0;

    info->ip_addr = msg->yiaddr;

    for(options = msg->options; *options != DHCP_END; ){
        switch(*options){

            case DHCP_PADDING:
                options++;
                break;

            case DHCP_MSG_TYPE:
                options += 2;
                msgtype = *options;
                options += DHCP_MSG_TYPE_LEN;
                break;

            case DHCP_SUBNET_MASK:
                options += 2;
                info->netmask = *(uint32_t*) options;
                options += DHCP_SUBNET_MASK_LEN;
                break;

            case DHCP_SERVER_ADDR:
                options += 2;
                info->server_addr = *(uint32_t*) options;
                options += DHCP_SERVER_ADDR_LEN;
                break;

            case DHCP_BROADCAST:
                broadcast_set = 1;
                options += 2;
                info->broadcast = *(uint32_t*) options;
                options += DHCP_BROADCAST_LEN;
                break;

            case DHCP_ROUTERS: {
                uint8_t size = *(options+1);
                options += 2;
                memcpy(&(info->routers), options, size);
                options += size;
                break;
                }

            // TODO
            case DHCP_NAME_SERVERS:
            case DHCP_HOSTNAME:
            case DHCP_DOMAIN_NAME:
            case DHCP_IP_TTL:
            case DHCP_IF_MTU:
            case DHCP_ARP_CACHE_TIMEOUT:
            case DHCP_TCP_TTL:
            case DHCP_TCP_KEEPALIVE:
            default:
                options += 2 + *(options+1);
                break;
        }
    }

    // TODO: see if the transaction IDs match

     if(memcmp(msg->chaddr, my_hw_addr, 6) == 0 && msg->op == BOOTREPLY){ /* A packet for me, TODO: check UDP dest addr instead */
        if(msgtype == DHCPOFFER && status == DISCOVER){
            uint8_t *ip = (uint8_t*) &(info->ip_addr);
            uint8_t *nm = (uint8_t*) &(info->netmask);
            uint8_t *sip = (uint8_t*) &(info->server_addr);
            printf("%s: accepting DHCPOFFER, server: %d.%d.%d.%d, ", argv0, sip[0], sip[1], sip[2], sip[3]);
            printf("ip: %d.%d.%d.%d, netmask: %d.%d.%d.%d\n",
            ip[0], ip[1], ip[2], ip[3], nm[0], nm[1], nm[2], nm[3]);

            /* Policy: accept the first offer */

            status = REQUEST;
            send_request(msg, info);
        }else if(msgtype == DHCPACK && status == REQUEST){
            status = DONE;

            uint8_t *ip = (uint8_t*) &(info->ip_addr);
            uint8_t *nm = (uint8_t*) &(info->netmask);
            uint8_t *sip = (uint8_t*) &(info->server_addr);
            uint8_t *r = (uint8_t*) &(info->routers);
            uint8_t *bc = (uint8_t*) &(info->broadcast);
            uint8_t *ns = (uint8_t*) &(info->name_servers);

            printf("%s: got ACK from %d.%d.%d.%d, now using\n", argv0, sip[0], sip[1], sip[2], sip[3]);
            printf("  ip: %d.%d.%d.%d,\n", ip[0], ip[1], ip[2], ip[3]);
            printf("  netmask: %d.%d.%d.%d,\n", nm[0], nm[1], nm[2], nm[3]);

            // TODO: use more than one routers
            if(info->routers != NULL){
                printf("  router: %d.%d.%d.%d,\n", r[0], r[1], r[2], r[3]);
            }

            if(broadcast_set == 1){
                printf("  broadcast: %d.%d.%d.%d,\n", bc[0], bc[1], bc[2], bc[3]);
            }

            // TODO: use more than one name servers
            if(info->name_servers != NULL){
                printf("  nameserver: %d.%d.%d.%d\n", ns[0], ns[1], ns[2], ns[3]);
            }

            struct ifreq req;

            strncpy( req.ifr_name, device, IFNAMSIZ );
            req.ifr_name[ IFNAMSIZ - 1 ] = 0;

            if_set_ip_address(&req, info->ip_addr);
            if_set_netmask(&req, info->netmask);
            if_set_broadcast(&req, info->broadcast);

        }else if(msgtype == DHCPNAK && status == REQUEST){
            status = DISCOVER;
            send_discover(); // TODO: be careful not to get in an infinite loop
        }
    }    
}

int interface_up( void ) {
    struct ifreq req;

    /* Bring up the interface if it's down */

    strncpy( req.ifr_name, device, IFNAMSIZ );
    req.ifr_name[ IFNAMSIZ - 1 ] = 0;

    if ( ioctl( sock, SIOCGIFFLAGS, &req ) != 0 ) {
        fprintf( stderr, "%s: failed to ioctl() interface %s: %s\n", argv0, device, strerror( errno ) );
        return -1;
    }

    if ( !( req.ifr_flags & IFF_UP ) ) {
        req.ifr_flags |= IFF_UP;

        if ( ioctl( sock, SIOCSIFFLAGS, &req ) != 0 ) {
            fprintf( stderr, "%s: failed to bring up interface %s: %s\n", argv0, device, strerror( errno ) );
            return -1;
        }
    }

    /* Get the hardware address */

    if ( ioctl( sock, SIOCGIFHWADDR, &req ) != 0 ) {
        fprintf( stderr, "%s: failed to ioctl() interface %s: %s\n", argv0, device, strerror( errno ) );
        return -1;
    }

    memcpy( my_hw_addr, req.ifr_hwaddr.sa_data, 6 );

    printf(
        "%s: client HW address: %02x:%02x:%02x:%02x:%02x:%02x\n",
        argv0,
        my_hw_addr[ 0 ], my_hw_addr[ 1 ], my_hw_addr[ 2 ],
        my_hw_addr[ 3 ], my_hw_addr[ 4 ], my_hw_addr[ 5 ]
    );

    return 0;
}

int dhcp_mainloop( void ) {
    int size;
    socklen_t addr_len;
    struct sockaddr addr;

    while ( 1 ) {

        if( status == DONE ){
            // TODO: fork, wait for the lease time to expire
            break;
        }

        addr_len = sizeof( struct sockaddr );

        size = recvfrom( sock, in_buffer, sizeof( in_buffer ), MSG_NOSIGNAL, &addr, &addr_len );

        if ( size < 0 ) {
            fprintf( stderr, "%s: failed to recv incoming data.\n", argv0 );
            break;
        }

//        printf( "%s: received %d bytes.\n", argv0, size );

        // TODO: use proper reading, packets may come in multiple pieces
        if( size >= sizeof(dhcp_msg_t)){
            dhcp_info_t info;
            dhcp_msg_t* msg = (dhcp_msg_t*) in_buffer;
            parse_message(msg, &info);
        }

    }

    return 0;
}

int main( int argc, char** argv ) {
    if ( argc != 2 ) {
        printf( "Usage: %s device\n", argv[ 0 ] );
        return EXIT_FAILURE;
    }

    argv0 = argv[ 0 ];
    device = argv[ 1 ];

    printf( "%s: started.\n", argv0 );

    if ( create_socket() != 0 ) {
        fprintf( stderr, "%s: failed to create the UDP socket.\n", argv0 );
        return EXIT_FAILURE;
    }

    if(interface_up() < 0){
        return EXIT_FAILURE;
    }

    status = DISCOVER;

    send_discover();
    dhcp_mainloop();

    return 0;
}

