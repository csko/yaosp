/* PNG image loader
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

#include <errno.h>
#include <stdlib.h>
#include <setjmp.h>
#include <png.h>

#include <ygui/image/imageloader.h>
#include <yutil/blockbuffer.h>

typedef struct png_private {
    png_structp png_struct;
    png_infop png_info;

    int bytes_per_row;
    block_buffer_t output_buffer;
} png_private_t;

static void png_info_callback( png_structp png_ptr, png_infop info );
static void png_row_callback( png_structp png_ptr, png_bytep new_row, png_uint_32 row_num, int pass );
static void png_end_callback( png_structp png_ptr, png_infop info );

static int png_identify( uint8_t* data, size_t size ) {
    if ( size < 4 ) {
        return -EINVAL;
    }

    if ( !png_check_sig( ( png_bytep )data, size ) ) {
        return -EINVAL;
    }

    return 0;
}

static int png_create( void** _private ) {
    int error;
    png_private_t* private;

    private = ( png_private_t* )malloc( sizeof( png_private_t ) );

    if ( private == NULL ) {
        error = -ENOMEM;
        goto error1;
    }

    init_block_buffer( &private->output_buffer, 8192 );

    private->png_struct = png_create_read_struct( PNG_LIBPNG_VER_STRING, NULL, NULL, NULL );

    if ( private->png_struct == NULL ) {
        error = -ENOMEM;
        goto error2;
    }

    private->png_info = png_create_info_struct( private->png_struct );

    if ( private->png_info == NULL ) {
        error = -ENOMEM;
        goto error3;
    }

    if ( setjmp( private->png_struct->jmpbuf ) ) {
        error = -1; /* todo */
        goto error3;
    }

    png_set_progressive_read_fn(
        private->png_struct,
        ( void* )private,
        png_info_callback,
        png_row_callback,
        png_end_callback
    );

    *_private = private;

    return 0;

 error3:
    png_destroy_read_struct( &private->png_struct, &private->png_info, ( png_infopp )NULL );

 error2:
    free( private );

 error1:
    return error;
}

static int png_destroy( void* _private ) {
    png_private_t* private;

    private = ( png_private_t* )_private;

    /* TODO: destroy PNG related stuffs */
    destroy_block_buffer( &private->output_buffer );
    free( private );

    return 0;
}

static int png_add_data( void* _private, uint8_t* data, size_t size, int finalize ) {
    png_private_t* private;

    private = ( png_private_t* )_private;

    if ( setjmp( private->png_struct->jmpbuf ) ) {
        return -1;
    }

    png_process_data(
        private->png_struct,
        private->png_info,
        ( png_bytep )data, size
    );

    return 0;
}

static int png_get_available_size( void* _private ) {
    png_private_t* private;

    private = ( png_private_t* )_private;

    return block_buffer_get_size( &private->output_buffer );
}

static int png_read_data( void* _private, uint8_t* data, size_t size ) {
    png_private_t* private;

    private = ( png_private_t* )_private;

    return block_buffer_read_and_delete( &private->output_buffer, data, size );
}

static void png_info_callback( png_structp png_ptr, png_infop info ) {
    int depth;
    int color_type;
    int interlace_type;
    png_uint_32 width;
    png_uint_32 height;
    png_private_t* private;
    image_info_t img_info;

    private = ( png_private_t* )png_get_progressive_ptr( png_ptr );

    png_get_IHDR(
        private->png_struct,
        private->png_info,
        &width,
        &height,
        &depth,
        &color_type,
        &interlace_type,
        NULL,
        NULL
    );

    img_info.width = width;
    img_info.height = height;
    img_info.color_space = CS_RGB32;

    block_buffer_write( &private->output_buffer, ( void* )&img_info, sizeof( image_info_t ) );

    private->bytes_per_row = width * 4;

    if ( ( color_type == PNG_COLOR_TYPE_PALETTE ) ||
         ( png_get_valid( private->png_struct, private->png_info, PNG_INFO_tRNS ) ) ) {
        png_set_expand( private->png_struct );
    }

    double image_gamma;

    if ( png_get_gAMA( private->png_struct, private->png_info, &image_gamma ) ) {
        png_set_gamma( private->png_struct, 2.2, image_gamma );
    } else {
        png_set_gamma( private->png_struct, 2.2, 0.45 );
    }

    png_set_bgr( private->png_struct );
    png_set_filler( private->png_struct, 0xFF, PNG_FILLER_AFTER );
    png_set_gray_to_rgb( private->png_struct );
    png_set_interlace_handling( private->png_struct );
    png_read_update_info( private->png_struct, private->png_info );
}

static void png_row_callback( png_structp png_ptr, png_bytep new_row, png_uint_32 row_num, int pass ) {
    png_private_t* private;

    private = ( png_private_t* )png_get_progressive_ptr( png_ptr );

    block_buffer_write(
        &private->output_buffer,
        new_row,
        private->bytes_per_row
    );
}

static void png_end_callback( png_structp png_ptr, png_infop info ) {
}

image_loader_t png_loader = {
    .name = "PNG",
    .identify = png_identify,
    .create = png_create,
    .destroy = png_destroy,
    .add_data = png_add_data,
    .get_available_size = png_get_available_size,
    .read_data = png_read_data
};
