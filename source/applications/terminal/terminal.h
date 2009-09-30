/* Terminal application
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

#ifndef _TERMINAL_H_
#define _TERMINAL_H_

#include <pthread.h>
#include <sys/types.h>

#include "buffer.h"

#define TERMINAL_MAX_PARAMS 10

typedef enum terminal_state {
    STATE_NONE,
    STATE_ESCAPE,
    STATE_BRACKET,
    STATE_SQUARE_BRACKET,
    STATE_QUESTION
} terminal_state_t;

typedef struct terminal {
    pthread_mutex_t lock;

    int first_number;
    int parameter_count;
    int parameters[ TERMINAL_MAX_PARAMS ];
    terminal_state_t state;

    terminal_buffer_t buffer;
} terminal_t;

int terminal_handle_data( terminal_t* terminal, uint8_t* data, int size );

terminal_t* create_terminal( int width, int height );
int destroy_terminal( terminal_t* terminal );

#endif /* _TERMINAL_H_ */
