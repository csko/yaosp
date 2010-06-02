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
#include <png.h>
#include <setjmp.h>
#include <unistd.h>
#include <assert.h>

#include <imgloader.h>

typedef struct png_data_t {
    png_structp png_struct;
    png_infop png_info;

    bitmap_t* bitmap;
} png_data_t;

int png_loader_identify( int fd ) {
    int size;
    char buffer[ 32 ];

    if ( lseek( fd, 0, SEEK_SET ) != 0 ) {
        return -EINVAL;
    }

    size = read( fd, buffer, sizeof( buffer ) );

    if ( size < 0 ) {
        return -EIO;
    }

    if ( !png_check_sig( ( png_bytep )buffer, size ) ) {
        return -EINVAL;
    }

    return 0;
}

static void png_info_callback( png_structp png_ptr, png_infop info ) {
    int depth;
    int color_type;
    int interlace_type;
    png_uint_32 width;
    png_uint_32 height;
    png_data_t* data;

    data = ( png_data_t* )png_get_progressive_ptr( png_ptr );

    png_get_IHDR(
        data->png_struct, data->png_info,
        &width, &height, &depth,
        &color_type, &interlace_type,
        NULL, NULL
    );

    assert( data->bitmap == NULL );
    data->bitmap = create_bitmap( width, height, CS_RGB32 );

    if ( ( color_type == PNG_COLOR_TYPE_PALETTE ) ||
         ( png_get_valid( data->png_struct, data->png_info, PNG_INFO_tRNS ) ) ) {
        png_set_expand( data->png_struct );
    }

    double image_gamma;

    if ( png_get_gAMA( data->png_struct, data->png_info, &image_gamma ) ) {
        png_set_gamma( data->png_struct, 2.2, image_gamma );
    } else {
        png_set_gamma( data->png_struct, 2.2, 0.45 );
    }

    png_set_bgr( data->png_struct );
    png_set_filler( data->png_struct, 0xFF, PNG_FILLER_AFTER );
    png_set_gray_to_rgb( data->png_struct );
    png_set_interlace_handling( data->png_struct );
    png_read_update_info( data->png_struct, data->png_info );
}

static void png_row_callback( png_structp png_ptr, png_bytep new_row, png_uint_32 row_num, int pass ) {
    bitmap_t* bitmap;
    png_data_t* data;

    data = ( png_data_t* )png_get_progressive_ptr( png_ptr );
    bitmap = data->bitmap;

    memcpy(
        reinterpret_cast<uint8_t*>(bitmap->buffer) + row_num * bitmap->width * 4,
        new_row,
        bitmap->width * 4
    );
}

static void png_end_callback( png_structp png_ptr, png_infop info ) {
}

bitmap_t* png_loader_load( int fd ) {
    int size;
    char buffer[ 8192 ];
    png_data_t data = {
        NULL, NULL, NULL
    };

    if ( lseek( fd, 0, SEEK_SET ) != 0 ) {
        goto error1;
    }

    data.png_struct = png_create_read_struct( PNG_LIBPNG_VER_STRING, NULL, NULL, NULL );

    if ( data.png_struct == NULL ) {
        goto error1;
    }

    data.png_info = png_create_info_struct( data.png_struct );

    if ( data.png_info == NULL ) {
        goto error2;
    }

    if ( setjmp( png_jmpbuf(data.png_struct) ) ) {
        goto error2;
    }

    png_set_progressive_read_fn(
        data.png_struct,
        ( void* )&data,
        png_info_callback,
        png_row_callback,
        png_end_callback
    );

    do {
        size = read( fd, buffer, sizeof( buffer ) );

        if ( size < 0 ) {
            goto error3;
        } else if ( size > 0 ) {
            if ( setjmp( png_jmpbuf(data.png_struct) ) ) {
                goto error3;
            }

            png_process_data(
                data.png_struct,
                data.png_info,
                ( png_bytep )buffer, size
            );
        }
    } while ( size > 0 );

    return data.bitmap;

 error3:
    if ( data.bitmap != NULL ) {
        bitmap_put( data.bitmap );
    }

 error2:
    png_destroy_read_struct( &data.png_struct, &data.png_info, NULL );

 error1:
    return NULL;
}

image_loader_t png_loader = {
    png_loader_identify, png_loader_load
};
