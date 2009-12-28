/* DHCP client
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

#include <string.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include "dhcpclient.h"

void init_packet(dhcp_msg_t *msg, uint8_t type) {
    memset(msg, 0, sizeof(dhcp_msg_t));
    switch(type){
        case DHCPDISCOVER:
        case DHCPREQUEST:
        case DHCPRELEASE:
        case DHCPINFORM:
            msg->op = BOOTREQUEST;
            break;
    }
    msg->htype = ETH_10MB;
    msg->hlen = ETH_10MB_LEN;
    msg->cookie = htonl(DHCP_MAGIC);

    /* The required DHCP message type option and the ending byte */

    msg->options[0] = DHCP_MSG_TYPE;
    msg->options[0] = DHCP_MSG_TYPE_LEN;
    msg->options[0] = type;
    msg->options[0] = DHCP_END;

    set_chaddr(msg);

    // TODO: set client ID, FQDN, hostname, vendor

}

void set_chaddr(dhcp_msg_t *msg){
    int fd;
    struct ifreq ifr;

    fd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);

    ifr.ifr_addr.sa_family = AF_INET;
    strcpy(ifr.ifr_name, "eth0"); // TODO

    if(ioctl(fd, SIOCGIFHWADDR, &ifr) == 0){
        memcpy(msg->chaddr, ifr.ifr_hwaddr.sa_data, 6);
        dbprintf("dhcp: client HW address: %02x:%02x:%02x:%02x:%02x:%02x\n",
            msg->chaddr[0], msg->chaddr[1], msg->chaddr[2],
            msg->chaddr[3], msg->chaddr[4], msg->chaddr[5]);
    }else{
        dbprintf("dhcp: Failed to get HW address.\n");
    }

}

int send_packet(dhcp_msg_t *msg, uint32_t source_ip, int source_port,
        uint32_t dest_ip, int dest_port, uint8_t *dest_arp, int if_index){
    // TODO
    return 0;
}

void send_discover(){
    dhcp_msg_t msg;

    init_packet(&msg, DHCPDISCOVER);

    dbprintf("dhcp: sending DISCOVER.\n");

    // TODO: register packet

    int if_index = 0; // TODO

    send_packet(&msg, INADDR_ANY, CLIENT_PORT, INADDR_BROADCAST,
        SERVER_PORT, MAC_BCAST_ADDR, if_index);
}

int main(){
    dbprintf("dhcp: started.\n");
    send_discover();
    return 0;
}
