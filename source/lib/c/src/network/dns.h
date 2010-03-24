/* DNS resolv functions
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

#ifndef _INTERNAL_DNS_H_
#define _INTERNAL_DNS_H_

#include <pthread.h>

#define DNS_RECURSION_DESIRED 0x100

typedef enum {
    DNS_SERVER_UDP,
    DNS_SERVER_TCP
} dns_server_type_t;

typedef struct dns_server {
    char* name;
    dns_server_type_t type;
    struct dns_server* next;
} dns_server_t;

typedef struct dns_header {
    uint16_t id;
    uint16_t flags;
    uint16_t q_count;
    uint16_t a_count;
    uint16_t n_count;
    uint16_t d_count;
} __attribute__(( packed )) dns_header_t;

typedef struct dns_question_end {
    uint16_t type;
    uint16_t class;
} __attribute__(( packed )) dns_question_end_t;

typedef struct dns_answer {
    uint16_t name;
    uint16_t type;
    uint16_t cls;
    uint32_t ttl;
    uint16_t length;
} __attribute__(( packed )) dns_answer_t;

typedef struct dns_request {
    char* hostname;
    struct dns_request* next;

    int current_req_id;
    dns_server_t* server_first;
    dns_server_t* server_current;

    pthread_cond_t result_sync;
    struct in_addr* result_v4;
    size_t result_v4_cnt;
} dns_request_t;

/* DNS server */

dns_server_t* dns_server_create( char* name, dns_server_type_t type );
int dns_server_free( dns_server_t* server );

/* DNS request */

dns_request_t* dns_request_create( char* hostname );
int dns_request_add_server( dns_request_t* request, dns_server_t* server );

/* DNS resolv */

int dns_resolv( char* hostname, struct in_addr** v4_table, size_t* v4_cnt );

#endif /* _INTERNAL_DNS_H_ */
