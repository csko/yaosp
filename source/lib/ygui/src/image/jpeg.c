/* JPEG image loader
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

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <setjmp.h>
#include <string.h>
#include <jpeglib.h>
#include <sys/param.h>
#include <yaosp/debug.h>

#include <ygui/image/imageloader.h>
#include <yutil/blockbuffer.h>

#define JPEG_BUF_SIZE 65536

struct my_jpeg_error_mgr {
    struct jpeg_error_mgr pub;
    jmp_buf setjmp_buffer;
};

struct my_jpeg_source_mgr {
    struct jpeg_source_mgr pub;
    int eof;
    int data_size;
    int data_to_skip;
    uint8_t buffer[JPEG_BUF_SIZE];
    int final_pass;
    int decode_done;
};

enum load_state {
    INIT,
    START,
    DECOMPRESS,
    OUTPUT_PROCESS,
    FINISH
};

typedef struct jpeg_private {
    struct jpeg_decompress_struct decinfo;
    struct my_jpeg_error_mgr error;
    struct my_jpeg_source_mgr source;
    enum load_state state;
    uint8_t* line_jpeg;
    uint32_t* line_output;
    block_buffer_t output_buffer;
} jpeg_private_t;

static int jpeg_loader_identify( uint8_t* data, size_t size ) {
    if ( size < 3 ) {
        return -1;
    }

    if ( ( data[0] != 255 ) &&
         ( data[1] != 216 ) &&
         ( data[2] != 255 ) ) {
        return -1;
    }

    return 0;
}

static void jpeg_loader_error_exit( j_common_ptr info ) {
    char buf[8192];
    struct my_jpeg_error_mgr* error;

    (*info->err->format_message)(info, buf);
    dbprintf( "jpeg_loader_error_exit(): %s\n", buf );

    error = (struct my_jpeg_error_mgr*)info->err;
    longjmp( error->setjmp_buffer, 1 );
}

static void jpeg_loader_dummy( j_decompress_ptr p ) {
}

static boolean jpeg_loader_fill_input_buffer( j_decompress_ptr p ) {
    struct jpeg_source_mgr* jsrc;
    struct my_jpeg_source_mgr* source;

    source = (struct my_jpeg_source_mgr*)p->src;
    jsrc = (struct jpeg_source_mgr*)source;

    if ( source->eof ) {
        source->buffer[0] = 0xFF;
        source->buffer[1] = JPEG_EOI;
        jsrc->bytes_in_buffer = 2;

        return TRUE;
    }

    /* I/O suspension */

    return FALSE;
}

static void jpeg_loader_skip_input_data( j_decompress_ptr p, long len ) {
    int to_skip;
    struct jpeg_source_mgr* jsrc;
    struct my_jpeg_source_mgr* source;

    if ( len <= 0 ) {
        return;
    }

    source = (struct my_jpeg_source_mgr*)p->src;
    jsrc = (struct jpeg_source_mgr*)source;

    source->data_to_skip += len;
    to_skip = MIN(jsrc->bytes_in_buffer, source->data_to_skip);

    if ( to_skip < jsrc->bytes_in_buffer ) {
        memmove( source->buffer, jsrc->next_input_byte + to_skip, jsrc->bytes_in_buffer - to_skip );
    }

    source->data_size = jsrc->bytes_in_buffer - to_skip;
    source->data_to_skip -= to_skip;

    jsrc->next_input_byte = (JOCTET*)source->buffer;
    jsrc->bytes_in_buffer = source->data_size;
}

static int jpeg_loader_create( void** _private ) {
    jpeg_private_t* private;

    private = (jpeg_private_t*)malloc( sizeof(jpeg_private_t) );

    if ( private == NULL ) {
        goto error1;
    }

    memset( private, 0, sizeof(jpeg_private_t) );

    private->decinfo.err = jpeg_std_error(&private->error.pub);
    private->error.pub.error_exit = jpeg_loader_error_exit;

    if ( setjmp(private->error.setjmp_buffer) ) {
        /* todo: destroy resources here */
        return -1;
    }

    jpeg_create_decompress(&private->decinfo);

    private->decinfo.src = (struct jpeg_source_mgr*)&private->source;
    private->source.pub.init_source = jpeg_loader_dummy;
    private->source.pub.fill_input_buffer = jpeg_loader_fill_input_buffer;
    private->source.pub.skip_input_data = jpeg_loader_skip_input_data;
    private->source.pub.resync_to_restart = jpeg_resync_to_restart;
    private->source.pub.term_source = jpeg_loader_dummy;

    private->state = INIT;

    init_block_buffer( &private->output_buffer, 8192 );

    *_private = (void*)private;

    return 0;

 error1:
    return -1;
}

static int jpeg_loader_destroy( void* _private ) {
    jpeg_private_t* private;

    private = (jpeg_private_t*)_private;

    jpeg_destroy_decompress(&private->decinfo);

    if ( private->line_jpeg != NULL ) {
        free(private->line_jpeg);
    }

    if ( private->line_output != NULL ) {
        free(private->line_output);
    }

    destroy_block_buffer(&private->output_buffer);
    free(private);

    return 0;
}

static int jpeg_loader_add_data( void* _private, uint8_t* data, size_t size, int finalize ) {
    jpeg_private_t* private;
    struct jpeg_source_mgr* jsrc;
    struct my_jpeg_source_mgr* source;
    struct jpeg_decompress_struct* decinfo;

    private = (jpeg_private_t*)_private;
    decinfo = &private->decinfo;
    source = &private->source;
    jsrc = (struct jpeg_source_mgr*)source;

    if ( source->eof ) {
        return 0;
    }

    if ( setjmp(private->error.setjmp_buffer) ) {
        dbprintf( "%s(): JPEG error :(\n", __FUNCTION__ );
        return -1;
    }

    while ( ( size > 0 ) ||
            ( finalize && !source->eof ) ) {
        /* Calculate the free data size in the buffer. */

        int free_data = MIN(size, JPEG_BUF_SIZE - source->data_size);

        /* If there is a little free space in the buffer, then copy the new data into the buffer. */

        if ( free_data > 0 ) {
            size -= free_data;

            memcpy( source->buffer + source->data_size, data, free_data );
            source->data_size += free_data;
        }

        /* Skip some data in the buffer if it is requested. */

        if ( source->data_to_skip > 0 ) {
            int to_skip = MIN(source->data_size, source->data_to_skip);

            /* Do we have to move the data inside the buffer? */

            if ( to_skip < source->data_size ) {
                memmove( source->buffer, source->buffer + to_skip, source->data_size - to_skip );
            }

            /* Ok, the data is skipped now. */

            source->data_size -= to_skip;
            source->data_to_skip -= to_skip;

            /* If we still need to skip some data,
               return to the caller, so we can get more. :) */

            if ( source->data_to_skip > 0 ) {
                return 0;
            }
        }

        /* Tell the JPEG decompressor which is the next byte in the
           buffer and how many data is available for parsing. */

        decinfo->src->next_input_byte = (JOCTET*)source->buffer;
        decinfo->src->bytes_in_buffer = (size_t)source->data_size;

        if ( private->state == INIT ) {
            int ret;

            ret = jpeg_read_header(decinfo, TRUE);

            if ( ret == JPEG_SUSPENDED ) {
                if ( finalize ) {
                    source->eof = 1;
                }
            } else {
                private->state = START;
            }
        }

        /* Put the image_info_t structure to the output buffer in the START state.
           At this point, the JPEG header is already read and parsed. */

        if ( private->state == START ) {
            boolean ret;

            decinfo->buffered_image = TRUE;
            decinfo->do_fancy_upsampling = FALSE;
            decinfo->do_block_smoothing = FALSE;
            decinfo->dct_method = JDCT_FASTEST;

            ret = jpeg_start_decompress(decinfo);

            if ( ret ) {
                image_info_t img_info;

                img_info.width = decinfo->output_width;
                img_info.height = decinfo->output_height;
                img_info.color_space = CS_RGB32;

                block_buffer_write( &private->output_buffer, &img_info, sizeof(image_info_t) );

                private->state = DECOMPRESS;
                private->line_jpeg = (uint8_t*)malloc( decinfo->output_width * decinfo->output_components );
                private->line_output = (uint32_t*)malloc( decinfo->output_width * sizeof(uint32_t) );
                /* todo: error checking */
            } else {
                if ( finalize ) {
                    source->eof = 1;
                }
            }
        }

        /* Do the decompression in the DECOMPRESS state. */

        if ( private->state == DECOMPRESS ) {
            decinfo->buffered_image = TRUE;
            jpeg_start_output(decinfo, decinfo->input_scan_number);
            private->state = OUTPUT_PROCESS;
        }

        /* Process all the scanlines and put them to the output buffer in OUTPUT_PROCESS state. */

        if ( private->state == OUTPUT_PROCESS ) {
            while ( decinfo->output_scanline < decinfo->output_height ) {
                /* Ask for the next line. */

                if ( jpeg_read_scanlines( decinfo, &private->line_jpeg, 1 ) != 1 ) {
                    break;
                }

                /* Parse the output of libjpeg according to the color space of the image. */

                switch ( decinfo->jpeg_color_space ) {
                    case JCS_YCbCr :
                    case JCS_RGB : {
                        int i;

                        for ( i = 0; i < decinfo->output_width; i++ ) {
                            private->line_output[i] =
                                ( private->line_jpeg[i*3+2]       ) |
                                ( private->line_jpeg[i*3+1] <<  8 ) |
                                ( private->line_jpeg[i*3]   << 16 ) |
                                0xFF000000;
                        }

                        block_buffer_write(
                            &private->output_buffer, private->line_output,
                            decinfo->output_width * sizeof(uint32_t)
                        );

                        break;
                    }

                    default :
                        dbprintf( "%s(): unsupported JPEG color space: %d\n", __FUNCTION__, decinfo->jpeg_color_space );
                        return 0;
                }
            }

            if ( decinfo->output_scanline >= decinfo->output_height ) {
                jpeg_finish_output(decinfo);
                source->final_pass = jpeg_input_complete(decinfo);
                source->decode_done = ( source->final_pass ) &&
                                      ( decinfo->input_scan_number == decinfo->output_scan_number );

                if ( !source->decode_done ) {
                    private->state = DECOMPRESS;
                }
            }

            if ( ( private->state == OUTPUT_PROCESS ) &&
                 ( source->decode_done ) ) {
                jpeg_finish_decompress(decinfo);
                source->eof = 1;
                return 0;
            }
        }

        if ( jsrc->bytes_in_buffer > 0 ) {
            memmove( source->buffer, jsrc->next_input_byte, jsrc->bytes_in_buffer );
        }

        source->data_size = jsrc->bytes_in_buffer;
    }

    return 0;
}

static int jpeg_loader_get_available_size( void* _private ) {
    jpeg_private_t* private;

    private = (jpeg_private_t*)_private;
    return block_buffer_get_size( &private->output_buffer );
}

static int jpeg_loader_read_data( void* _private, uint8_t* data, size_t size ) {
    jpeg_private_t* private;

    private = (jpeg_private_t*)_private;
    return block_buffer_read_and_delete( &private->output_buffer, data, size );
}

image_loader_t jpeg_loader = {
    .name = "JPEG",
    .identify = jpeg_loader_identify,
    .create = jpeg_loader_create,
    .destroy = jpeg_loader_destroy,
    .add_data = jpeg_loader_add_data,
    .get_available_size = jpeg_loader_get_available_size,
    .read_data = jpeg_loader_read_data
};
