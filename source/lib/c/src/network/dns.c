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

#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <yaosp/debug.h>
#include <yaosp/config.h>

#include "dns.h"

static int dns_socket = -1;
static int dns_next_req_id = 0;
static int dns_initialized = 0;
static dns_request_t* dns_req_list = NULL;

static pthread_t dns_thread;
static pthread_mutex_t dns_lock = PTHREAD_MUTEX_INITIALIZER;

/* DNS server */

dns_server_t* dns_server_create( char* name, dns_server_type_t type ) {
    size_t length;
    dns_server_t* server;

    length = sizeof(dns_server_t) + strlen(name) + 1;
    server = (dns_server_t*)malloc(length);

    if ( server == NULL ) {
        return NULL;
    }

    server->name = (char*)( server + 1 );
    server->type = type;
    server->next = NULL;
    strcpy( server->name, name );

    return server;
}

int dns_server_free( dns_server_t* server ) {
    free(server);
    return 0;
}

/* DNS request */

dns_request_t* dns_request_create( char* hostname ) {
    size_t length;
    dns_request_t* request;

    length = sizeof(dns_request_t) + strlen(hostname) + 1;
    request = (dns_request_t*)malloc(length);

    if ( request == NULL ) {
        return NULL;
    }

    if ( pthread_cond_init( &request->result_sync, NULL ) != 0 ) {
        free(request);
        return NULL;
    }

    request->hostname = (char*)( request + 1 );
    request->server_first = NULL;
    request->server_current = NULL;
    request->next = NULL;
    request->result_v4 = NULL;
    request->result_v4_cnt = 0;
    strcpy( request->hostname, hostname );

    return request;
}

int dns_request_destroy( dns_request_t* request ) {
    dns_server_t* server;

    server = request->server_first;

    while ( server != NULL ) {
        dns_server_t* todel = server;
        server = server->next;

        dns_server_free(todel);
    }

    free(request->result_v4);
    free(request);

    return 0;
}

int dns_request_add_server( dns_request_t* request, dns_server_t* server ) {
    if ( request->server_first == NULL ) {
        request->server_first = server;
        request->server_current = server;
    } else {
        dns_server_t* current = request->server_first;

        while ( current->next != NULL ) {
            current = current->next;
        }

        current->next = server;
    }

    return 0;
}

int dns_request_insert( dns_request_t* request ) {
    request->next = dns_req_list;
    dns_req_list = request;

    return 0;
}

int dns_request_remove( dns_request_t* request ) {
    dns_request_t* prev = NULL;
    dns_request_t* current = dns_req_list;

    while ( current != NULL ) {
        if ( current == request ) {
            if ( prev == NULL ) {
                dns_req_list = current->next;
            } else {
                prev->next = current->next;
            }

            return 0;
        }

        prev = current;
        current = current->next;
    }

    return -1;
}

dns_request_t* dns_request_get_by_id( int id ) {
    dns_request_t* current;

    current = dns_req_list;

    while ( current != NULL ) {
        if ( current->current_req_id == id ) {
            return current;
        }

        current = current->next;
    }

    return NULL;
}

static int dns_convert_name( char* buffer, const char* name ) {
    char* to;
    char* from;
    int part_len;
    size_t i;
    size_t length;

    length = strlen(name);
    from = (char*)name + length;
    length += 1;
    to = buffer + length;

    part_len = -1; /* hack for the ending 0 */

    for ( i = 0; i < length; i++, to--, from-- ) {
        if ( *from == '.' ) {
            *to = part_len;
            part_len = 0;
        } else {
            *to = *from;
            part_len++;
        }
    }

    *to = part_len;

    return length + 1;
}

static size_t dns_calc_question_len( uint8_t* data ) {
    size_t length = 0;

    while ( *data != 0 ) {
        uint8_t len = *data;
        length += len + 1;
        data += len + 1;
    }

    return length + 1; /* +1 for the ending zero */
}

static int dns_handle_packet( char* buffer, int size ) {
    int i;
    uint8_t* data;
    uint16_t answers;
    uint16_t requests;
    dns_header_t* header;
    dns_request_t* request;

    header = (dns_header_t*)buffer;
    requests = htons(header->q_count);
    answers = htons(header->a_count);

    if ( answers == 0 ) {
        return 0;
    }

    request = dns_request_get_by_id(header->id);

    if ( request == NULL ) {
        return 0;
    }

    /* Run through the requests */

    data = (uint8_t*)( header + 1 );

    for ( i = 0; i < requests; i++ ) {
        size_t len = dns_calc_question_len(data);

        data += len;
        data += sizeof(dns_question_end_t);
    }

    /* Parse the answers */

    for ( i = 0; i < answers; i++ ) {
        uint16_t type;
        uint16_t length;
        dns_answer_t* answer = (dns_answer_t*)data;

        type = htons(answer->type);
        length = htons(answer->length);

        switch ( type ) {
            case 0x0001 : { /* A record */
                if ( length == 4 ) {
                    struct in_addr* res_v4;

                    res_v4 = (struct in_addr*)realloc(
                        request->result_v4, sizeof(struct in_addr) * ( request->result_v4_cnt + 1 )
                    );

                    if ( res_v4 != NULL ) {
                        uint8_t* ip = (uint8_t*)(answer+1);
                        memcpy( &res_v4[request->result_v4_cnt], ip, 4 );

                        request->result_v4 = res_v4;
                        request->result_v4_cnt++;
                    }
                }

                break;
            }
        }

        data += sizeof(dns_answer_t);
        data += length;
    }

    pthread_cond_broadcast(&request->result_sync);

    return 0;
}

static void* dns_thread_entry( void* arg ) {
    while ( 1 ) {
        int ret;
        fd_set readers;

        FD_ZERO(&readers);
        FD_SET(dns_socket, &readers);

        ret = select( dns_socket + 1, &readers, NULL, NULL, NULL );

        if ( ret > 0 ) {
            int size;
            char buffer[ 1024 ];
            struct sockaddr_in from_addr;
            socklen_t addr_len = sizeof(struct sockaddr_in);

            size = recvfrom( dns_socket, buffer, sizeof(buffer), 0, (struct sockaddr*)&from_addr, &addr_len );

            if ( size > 0 ) {
                dns_handle_packet( buffer, size );
            }
        } else if ( ret < 0 ) {
            dbprintf( "dns_thread_entry(): select returned error: %d (%s).\n", errno, strerror(errno) );
            break;
        }
    }

    return NULL;
}

static int dns_initialize( void ) {
    dns_socket = socket( AF_INET, SOCK_DGRAM, 0 );

    if ( dns_socket == -1 ) {
        return -1;
    }

    pthread_create( &dns_thread, NULL, dns_thread_entry, NULL );

    return 0;
}

static int dns_send_request( dns_request_t* request ) {
    int ret;
    char* buffer;
    size_t length;
    dns_header_t* header;
    dns_question_end_t* q_end;
    struct sockaddr_in addr;

    length = sizeof(dns_header_t) + strlen(request->hostname) + 2 + sizeof(dns_question_end_t);
    buffer = (char*)malloc(length);

    if ( buffer == NULL ) {
        return -ENOMEM;
    }

    header = (dns_header_t*)buffer;

    header->id = dns_next_req_id++;
    header->flags = htons(DNS_RECURSION_DESIRED);
    header->q_count = htons(1);
    header->a_count = 0;
    header->n_count = 0;
    header->d_count = 0;

    dns_convert_name( (char*)( header + 1 ), request->hostname );

    q_end = (dns_question_end_t*)( buffer + sizeof(dns_header_t) + strlen(request->hostname) + 2 );
    q_end->type = htons(1);
    q_end->class = htons(1);

    /* Build the DNS server address */

    addr.sin_family = AF_INET;
    inet_pton( AF_INET, request->server_current->name, &addr.sin_addr );
    addr.sin_port = htons(53);

    /* Bump to the next server and store the request ID */

    request->current_req_id = header->id;
    request->server_current = request->server_current->next;

    /* Send the packet out */

    ret = sendto( dns_socket, buffer, length, 0, (struct sockaddr*)&addr, sizeof(struct sockaddr_in) );

    if ( ret < 0 ) {
        return ret;
    }

    dns_request_insert(request);

    return 0;
}

int dns_resolv( char* hostname, struct in_addr** v4_table, size_t* v4_cnt ) {
    int i;
    int ns_count;
    char** ns_names;
    dns_request_t* request;

    /* Initialize the configserver connection if it's not done already. */

    ycfg_init();

    /* List all the name servers. */

    if ( ( ycfg_list_children( "network/nameservers", &ns_names ) != 0 ) ||
         ( ns_names[0] == NULL ) ) {
        return -1;
    }

    request = dns_request_create(hostname);

    if ( request == NULL ) {
        return -ENOMEM;
    }

    ns_count = 0;

    for ( i = 0; ns_names[i] != NULL; i++ ) {
        char path[256];
        char* address;
        dns_server_t* server;

        snprintf( path, sizeof(path), "network/nameservers/%s", ns_names[i] );
        free( ns_names[i] );

        if ( ycfg_get_ascii_value( path, "address", &address ) != 0 ) {
            continue;
        }

        server = dns_server_create( address, DNS_SERVER_UDP );

        if ( server != NULL ) {
            dns_request_add_server( request, server );
            ns_count++;
        }

        free( address );
    }

    free( ns_names );

    if ( ns_count == 0 ) {
        dns_request_destroy(request);
        return -1;
    }

    pthread_mutex_lock(&dns_lock);

    if ( !dns_initialized ) {
        dns_initialize();
        dns_initialized = 1;
    }

    while ( ( request->server_current != NULL ) &&
            ( request->result_v4_cnt == 0 ) ) {
        if ( dns_send_request(request) == 0 ) {
            pthread_cond_wait( &request->result_sync, &dns_lock );
            dns_request_remove(request);
        }
    }

    pthread_mutex_unlock(&dns_lock);

    if ( request->result_v4_cnt == 0 ) {
        return -1;
    }

    *v4_table = request->result_v4;
    *v4_cnt = request->result_v4_cnt;

    dns_request_destroy(request);

    return 0;
}
