/* Blockbuffer implementation
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

#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/param.h>

#include <yutil/blockbuffer.h>

static block_t* block_buffer_get_free_block( block_buffer_t* buffer ) {
    block_t* block;

    if ( list_get_size( &buffer->free_blocks ) > 0 ) {
        block = ( block_t* )list_pop_head( &buffer->free_blocks );
    } else {
        block = ( block_t* )malloc( sizeof( block_t ) + buffer->block_size );
    }

    if ( block != NULL ) {
        block->size = 0;
    }

    return block;
}

static void block_buffer_put_free_block( block_buffer_t* buffer, block_t* block ) {
    if ( list_get_size( &buffer->free_blocks ) < buffer->max_free ) {
        list_add_item( &buffer->free_blocks, ( void* )block );
    } else {
        free( block );
    }
}

int block_buffer_read_and_delete( block_buffer_t* buffer, void* _data, size_t size ) {
    uint8_t* data;
    int read_size;

    data = ( uint8_t* )_data;
    read_size = 0;

    while ( ( size > 0 ) &&
            ( list_get_size( &buffer->used_blocks ) > 0 ) ) {
        int to_read;
        block_t* block;
        uint8_t* block_data;

        block = ( block_t* )list_get_head( &buffer->used_blocks );
        block_data = ( uint8_t* )( block + 1 );

        to_read = MIN( size, block->size );

        memcpy(
            data,
            block_data,
            to_read
        );

        if ( to_read == block->size ) {
            list_pop_head( &buffer->used_blocks );
            block_buffer_put_free_block( buffer, block );
        } else {
            memmove(
                block_data,
                block_data + to_read,
                block->size - to_read
            );

            block->size -= to_read;
        }

        data += to_read;
        read_size += to_read;
        size -= to_read;
    }

    return read_size;
}

int block_buffer_write( block_buffer_t* buffer, void* _data, size_t size ) {
    uint8_t* data;

    data = ( uint8_t* )_data;

    /* Fill the remaining of the last block, if possible ... */

    if ( list_get_size( &buffer->used_blocks ) > 0 ) {
        int to_copy;
        block_t* block;

        block = ( block_t* )list_get_tail( &buffer->used_blocks );
        to_copy = MIN( buffer->block_size - block->size, size );

        if ( to_copy > 0 ) {
            memcpy(
                ( uint8_t* )( block + 1 ) + block->size,
                data,
                to_copy
            );

            data += to_copy;
            size -= to_copy;
            block->size += to_copy;
            buffer->size += to_copy;
        }
    }

    while ( size > 0 ) {
        int to_copy;
        block_t* block;

        to_copy = MIN( buffer->block_size, size );

        block = block_buffer_get_free_block( buffer );

        if ( block == NULL ) {
            return -ENOMEM;
        }

        memcpy(
            ( void* )( block + 1 ),
            data,
            to_copy
        );

        block->size = to_copy;

        list_add_item( &buffer->used_blocks, ( void* )block );

        data += to_copy;
        size -= to_copy;
        buffer->size += to_copy;
    }

    return 0;
}

int block_buffer_get_size( block_buffer_t* buffer ) {
    return buffer->size;
}

int block_buffer_set_max_free( block_buffer_t* buffer, int max_free ) {
    int diff;

    buffer->max_free = max_free;

    diff = buffer->max_free - list_get_size( &buffer->free_blocks );

    if ( diff > 0 ) {
        int i;

        for ( i = 0; i < diff; i++ ) {
            block_t* block;

            block = ( block_t* )malloc( sizeof( block_t ) + buffer->block_size );

            if ( block == NULL ) {
                return -ENOMEM;
            }

            block->size = 0;

            list_add_item( &buffer->free_blocks, ( void* )block );
        }
    }

    return 0;
}

int init_block_buffer( block_buffer_t* buffer, int block_size ) {
    int error;

    error = init_list( &buffer->used_blocks );

    if ( error < 0 ) {
        goto error1;
    }

    error = init_list( &buffer->free_blocks );

    if ( error < 0 ) {
        goto error2;
    }

    list_set_max_free( &buffer->used_blocks, 32 );
    list_set_max_free( &buffer->free_blocks, 8 );

    buffer->size = 0;
    buffer->max_free = 0;
    buffer->block_size = block_size;

 error2:
    destroy_list( &buffer->used_blocks );

 error1:
    return error;
}

int destroy_block_buffer( block_buffer_t* buffer ) {
    /* Free used blocks */

    while ( list_get_size( &buffer->used_blocks ) > 0 ) {
        free( list_pop_head( &buffer->used_blocks ) );
    }

    destroy_list( &buffer->used_blocks );

    /* Free cached blocks */

    while ( list_get_size( &buffer->free_blocks ) > 0 ) {
        free( list_pop_head( &buffer->free_blocks ) );
    }

    destroy_list( &buffer->free_blocks );

    return 0;
}
