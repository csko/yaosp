/* yaosp GUI library
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
#include <assert.h>

#include <ygui/gc.h>
#include <ygui/window.h>
#include <ygui/render/render.h>

#include "internal.h"

typedef enum tr_type {
    TYPE_CHECKPOINT,
    TYPE_TRANSLATE
} tr_type_t;

typedef struct tr_entry {
    tr_type_t type;
    point_t offset;
} tr_entry_t;

int gc_push_restricted_area( gc_t* gc, rect_t* area ) {
    int error;
    rect_t* tmp;
    r_set_clip_rect_t* packet;

    tmp = ( rect_t* )malloc( sizeof( rect_t ) );

    if ( tmp == NULL ) {
        error = -ENOMEM;
        goto error1;
    }

    rect_copy( tmp, area );

    error = stack_push( &gc->res_area_stack, tmp );

    if ( error < 0 ) {
        goto error2;
    }

    rect_copy( &gc->clip_rect, tmp );

    gc->is_clip_valid = 1;

    /* Tell the clip rect to the render engine */

    error = allocate_render_packet( gc->window, sizeof( r_set_clip_rect_t ), ( void** )&packet );

    if ( error < 0 ) {
        return error;
    }

    packet->header.command = R_SET_CLIP_RECT;
    rect_copy( &packet->clip_rect, tmp );

    return 0;

 error2:
    free( tmp );

 error1:
    return error;
}

int gc_pop_restricted_area( gc_t* gc ) {
    int error;
    rect_t* tmp;

    error = stack_pop( &gc->res_area_stack, ( void** )&tmp );

    if ( error < 0 ) {
        return error;
    }

    free( tmp );

    return 0;
}

rect_t* gc_current_restricted_area( gc_t* gc ) {
    int error;
    rect_t* area;

    assert( stack_size( &gc->res_area_stack ) > 0 );

    error = stack_peek( &gc->res_area_stack, ( void** )&area );
    assert( error == 0 );

    return area;
}

int gc_push_translate_checkpoint( gc_t* gc ) {
    int error;
    tr_entry_t* entry;

    entry = ( tr_entry_t* )malloc( sizeof( tr_entry_t ) );

    if ( entry == NULL ) {
        return -ENOMEM;
    }

    entry->type = TYPE_CHECKPOINT;

    error = stack_push( &gc->tr_stack, entry );

    if ( error < 0 ) {
        return error;
    }

    return 0;
}

int gc_rollback_translate( gc_t* gc ) {
    int done;
    int error;
    tr_entry_t* entry;

    done = 0;

    while ( !done ) {
        error = stack_pop( &gc->tr_stack, ( void** )&entry );

        if ( error < 0 ) {
            return error;
        }

        switch ( entry->type ) {
            case TYPE_TRANSLATE :
                point_sub( &gc->lefttop, &entry->offset );
                break;

            case TYPE_CHECKPOINT :
                done = 1;
                break;
        }

        free( entry );
    }

    return 0;
}

int gc_get_pen_color( gc_t* gc, color_t* color ) {
    color_copy( color, &gc->pen_color );

    return 0;
}

int gc_set_pen_color( gc_t* gc, color_t* color ) {
    int error;
    r_set_pen_color_t* packet;

    error = allocate_render_packet( gc->window, sizeof( r_set_pen_color_t ), ( void** )&packet );

    if ( error < 0 ) {
        return error;
    }

    packet->header.command = R_SET_PEN_COLOR;
    color_copy( &packet->color, color );

    color_copy( &gc->pen_color, color );

    return 0;
}

int gc_set_font( gc_t* gc, font_t* font ) {
    int error;
    r_set_font_t* packet;

    /* If this font is already active, we don't have to set it again. */

    if ( gc->active_font == font->handle ) {
        return 0;
    }

    error = allocate_render_packet( gc->window, sizeof( r_set_font_t ), ( void** )&packet );

    if ( error < 0 ) {
        return error;
    }

    packet->header.command = R_SET_FONT;
    packet->font_handle = font->handle;

    gc->active_font = font->handle;

    return 0;
}

int gc_set_clip_rect( gc_t* gc, rect_t* rect ) {
    int error;
    rect_t tmp;
    rect_t* res_area;
    r_set_clip_rect_t* packet;

    assert( stack_size( &gc->res_area_stack ) > 0 );

    error = stack_peek( &gc->res_area_stack, ( void** )&res_area );

    if ( error < 0 ) {
        return error;
    }

    rect_add_point_n( &tmp, rect, &gc->lefttop );
    rect_and( &tmp, res_area );

    if ( !rect_is_valid( &tmp ) ) {
        gc->is_clip_valid = 0;

        return 0;
    }

    gc->is_clip_valid = 1;

    rect_copy( &gc->clip_rect, &tmp );

    error = allocate_render_packet( gc->window, sizeof( r_set_clip_rect_t ), ( void** )&packet );

    if ( error < 0 ) {
        return error;
    }

    packet->header.command = R_SET_CLIP_RECT;
    rect_copy( &packet->clip_rect, &tmp );

    return 0;
}

int gc_reset_clip_rect( gc_t* gc ) {
    int error;
    rect_t* res_area;
    r_set_clip_rect_t* packet;

    assert( stack_size( &gc->res_area_stack ) > 0 );

    error = stack_peek( &gc->res_area_stack, ( void** )&res_area );

    if ( error < 0 ) {
        return error;
    }

    gc->is_clip_valid = 1;
    rect_copy( &gc->clip_rect, res_area );

    error = allocate_render_packet( gc->window, sizeof( r_set_clip_rect_t ), ( void** )&packet );

    if ( error < 0 ) {
        return error;
    }

    packet->header.command = R_SET_CLIP_RECT;
    rect_copy( &packet->clip_rect, res_area );

    return 0;
}

int gc_translate( gc_t* gc, point_t* point ) {
    int error;
    tr_entry_t* entry;

    entry = ( tr_entry_t* )malloc( sizeof( tr_entry_t ) );

    if ( entry == NULL ) {
        error = -ENOMEM;
        goto error1;
    }

    entry->type = TYPE_TRANSLATE;
    point_copy( &entry->offset, point );

    error = stack_push( &gc->tr_stack, entry );

    if ( error < 0 ) {
        goto error2;
    }

    point_add( &gc->lefttop, point );

    return 0;

 error2:
    free( entry );

 error1:
    return error;
}

int gc_translate_xy( gc_t* gc, int x, int y ) {
    point_t tmp = {
        .x = x,
        .y = y
    };

    return gc_translate( gc, &tmp );
}

int gc_set_drawing_mode( gc_t* gc, drawing_mode_t mode ) {
    int error;
    r_set_drawing_mode_t* packet;

    error = allocate_render_packet( gc->window, sizeof( r_set_drawing_mode_t ), ( void** )&packet );

    if ( error < 0 ) {
        return error;
    }

    packet->header.command = R_SET_DRAWING_MODE;
    packet->mode = mode;

    return 0;
}

int gc_draw_rect( gc_t* gc, rect_t* rect ) {
    int error;
    rect_t tmp;
    r_draw_rect_t* packet;

    if ( !gc->is_clip_valid ) {
        return 0;
    }

    rect_add_point_n( &tmp, rect, &gc->lefttop );
    rect_and( &tmp, &gc->clip_rect );

    if ( !rect_is_valid( &tmp ) ) {
        return 0;
    }

    error = allocate_render_packet( gc->window, sizeof( r_draw_rect_t ), ( void** )&packet );

    if ( error < 0 ) {
        return error;
    }

    packet->header.command = R_DRAW_RECT;
    rect_copy( &packet->rect, &tmp );

    return 0;
}

int gc_fill_rect( gc_t* gc, rect_t* rect ) {
    int error;
    rect_t tmp;
    r_fill_rect_t* packet;

    if ( !gc->is_clip_valid ) {
        return 0;
    }

    rect_add_point_n( &tmp, rect, &gc->lefttop );
    rect_and( &tmp, &gc->clip_rect );

    if ( !rect_is_valid( &tmp ) ) {
        return 0;
    }

    error = allocate_render_packet( gc->window, sizeof( r_fill_rect_t ), ( void** )&packet );

    if ( error < 0 ) {
        return error;
    }

    packet->header.command = R_FILL_RECT;
    rect_copy( &packet->rect, &tmp );

    return 0;
}

int gc_draw_text( gc_t* gc, point_t* position, const char* text, int length ) {
    int error;
    r_draw_text_t* packet;

    if ( !gc->is_clip_valid ) {
        return 0;
    }

    if ( text == NULL ) {
        return -EINVAL;
    }

    if ( length == -1 ) {
        length = strlen( text );
    }

    if ( length == 0 ) {
        return 0;
    }

    error = allocate_render_packet( gc->window, sizeof( r_draw_text_t ) + length, ( void** )&packet );

    if ( error < 0 ) {
        return error;
    }

    packet->header.command = R_DRAW_TEXT;
    packet->length = length;

    point_add_n( &packet->position, position, &gc->lefttop );
    memcpy( ( void* )( packet + 1 ), text, length );

    return 0;
}

int gc_draw_bitmap( gc_t* gc, point_t* position, bitmap_t* bitmap ) {
    int error;
    r_draw_bitmap_t* packet;

    if ( !gc->is_clip_valid ) {
        return 0;
    }

    error = allocate_render_packet( gc->window, sizeof( r_draw_bitmap_t ), ( void** )&packet );

    if ( error < 0 ) {
        return error;
    }

    packet->header.command = R_DRAW_BITMAP;
    packet->bitmap_id = bitmap->id;
    point_add_n( &packet->position, position, &gc->lefttop );

    return 0;
}

int gc_clean_up( gc_t* gc ) {
    /* Clean the tr stack */

    while ( stack_size( &gc->tr_stack ) > 0 ) {
        tr_entry_t* entry;

        stack_pop( &gc->tr_stack, ( void** )&entry );

        free( entry );
    }

    /* Clean the res area stack */

    while ( stack_size( &gc->res_area_stack ) > 0 ) {
        rect_t* tmp;

        stack_pop( &gc->res_area_stack, ( void** )&tmp );

        free( tmp );
    }

    point_init(
        &gc->lefttop,
        0,
        0
    );

    gc->is_clip_valid = 1;

    return 0;
}

int gc_clean_cache( gc_t* gc ) {
    gc->active_font = -1;

    return 0;
}

int init_gc( window_t* window, gc_t* gc ) {
    int error;

    error = init_stack( &gc->tr_stack );

    if ( error < 0 ) {
        goto error1;
    }

    error = init_stack( &gc->res_area_stack );

    if ( error < 0 ) {
        goto error2;
    }

    gc->window = window;
    gc->is_clip_valid = 0;

    point_init(
        &gc->lefttop,
        0,
        0
    );

    gc_clean_cache( gc );

    return 0;

 error2:
    destroy_stack( &gc->tr_stack );

 error1:
    return error;
}

int destroy_gc( gc_t* gc ) {
    gc_clean_up( gc );

    destroy_stack( &gc->tr_stack );
    destroy_stack( &gc->res_area_stack );

    return 0;
}
