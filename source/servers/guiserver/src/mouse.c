/* GUI server
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
#include <assert.h>
#include <pthread.h>
#include <sys/types.h>
#include <ygui/point.h>

#include <mouse.h>
#include <graphicsdriver.h>

#define MW 0xFFFFFFFF
#define MB 0xFF000000
#define MI 0x00000000

static uint32_t default_pointer[ 16 * 16 ] = {
    MW, MW, MI, MI, MI, MI, MI, MI, MI, MI, MI, MI, MI, MI, MI, MI,
    MW, MB, MW, MI, MI, MI, MI, MI, MI, MI, MI, MI, MI, MI, MI, MI,
    MW, MB, MB, MW, MI, MI, MI, MI, MI, MI, MI, MI, MI, MI, MI, MI,
    MW, MB, MB, MB, MW, MI, MI, MI, MI, MI, MI, MI, MI, MI, MI, MI,
    MW, MB, MB, MB, MB, MW, MI, MI, MI, MI, MI, MI, MI, MI, MI, MI,
    MW, MB, MB, MB, MB, MB, MW, MI, MI, MI, MI, MI, MI, MI, MI, MI,
    MW, MB, MB, MB, MB, MB, MB, MW, MI, MI, MI, MI, MI, MI, MI, MI,
    MW, MB, MB, MB, MB, MB, MB, MB, MW, MI, MI, MI, MI, MI, MI, MI,
    MW, MB, MB, MB, MB, MB, MB, MB, MB, MW, MI, MI, MI, MI, MI, MI,
    MW, MB, MB, MB, MB, MB, MW, MW, MW, MW, MW, MI, MI, MI, MI, MI,
    MW, MB, MB, MW, MB, MB, MW, MI, MI, MI, MI, MI, MI, MI, MI, MI,
    MW, MB, MW, MW, MW, MB, MB, MW, MI, MI, MI, MI, MI, MI, MI, MI,
    MW, MW, MI, MI, MW, MB, MB, MW, MI, MI, MI, MI, MI, MI, MI, MI,
    MI, MI, MI, MI, MI, MW, MB, MB, MW, MI, MI, MI, MI, MI, MI, MI,
    MI, MI, MI, MI, MI, MW, MB, MB, MW, MI, MI, MI, MI, MI, MI, MI,
    MI, MI, MI, MI, MI, MI, MW, MW, MW, MI, MI, MI, MI, MI, MI, MI
};

static point_t mouse_position = { 0, 0 };

static int mouse_pointer_id_counter;
static hashtable_t mouse_pointer_table;
static pthread_mutex_t mouse_pointer_lock;

static mouse_pointer_t* active_mouse_pointer = NULL;

static int insert_mouse_pointer( mouse_pointer_t* pointer ) {
    int error;

    do {
        pointer->id = mouse_pointer_id_counter++;

        if ( mouse_pointer_id_counter < 0 ) {
            mouse_pointer_id_counter = 0;
        }
    } while ( hashtable_get( &mouse_pointer_table, ( const void* )&pointer->id ) != NULL );

    error = hashtable_add( &mouse_pointer_table, ( hashitem_t* )pointer );

    if ( error < 0 ) {
        return error;
    }

    return 0;
}

mouse_pointer_t* create_mouse_pointer( uint32_t width, uint32_t height, color_space_t color_space, void* raster ) {
    int error;
    mouse_pointer_t* pointer;

    pointer = ( mouse_pointer_t* )malloc( sizeof( mouse_pointer_t ) );

    if ( pointer == NULL ) {
        goto error1;
    }

    pointer->pointer_bitmap = create_bitmap_from_buffer( width, height, color_space, raster );

    if ( pointer->pointer_bitmap == NULL ) {
        goto error2;
    }

    pointer->hidden_bitmap = create_bitmap( width, height, color_space );

    if ( pointer->hidden_bitmap == NULL ) {
        goto error3;
    }

    error = insert_mouse_pointer( pointer );

    if ( error < 0 ) {
        goto error4;
    }

    pointer->ref_count = 1;

    return pointer;

error4:
    bitmap_put( pointer->hidden_bitmap );

error3:
    bitmap_put( pointer->pointer_bitmap );

error2:
    free( pointer );

error1:
    return NULL;
}

static int do_put_mouse_pointer( mouse_pointer_t* pointer ) {
    assert( pointer->ref_count >= 0 );

    if ( --pointer->ref_count == 0 ) {
        bitmap_put( pointer->pointer_bitmap );
        bitmap_put( pointer->hidden_bitmap );
        free( pointer );
    }

    return 0;
}

int put_mouse_pointer( mouse_pointer_t* pointer ) {
    int error;

    pthread_mutex_lock( &mouse_pointer_lock );

    error = do_put_mouse_pointer( pointer );

    pthread_mutex_unlock( &mouse_pointer_lock );

    return error;
}

int activate_mouse_pointer( mouse_pointer_t* pointer ) {
    pthread_mutex_lock( &mouse_pointer_lock );

    if ( active_mouse_pointer != NULL ) {
        do_put_mouse_pointer( active_mouse_pointer );
    }

    active_mouse_pointer = pointer;

    pointer->ref_count++;

    pthread_mutex_unlock( &mouse_pointer_lock );

    return 0;
}

int show_mouse_pointer( void ) {
    rect_t mouse_rect;
    static point_t null_point = { .x = 0, .y = 0 };

    assert( active_mouse_pointer != NULL );

    rect_init(
        &mouse_rect,
        mouse_position.x,
        mouse_position.y,
        mouse_position.x + active_mouse_pointer->pointer_bitmap->width - 1,
        mouse_position.y + active_mouse_pointer->pointer_bitmap->height - 1
    );
    rect_and( &mouse_rect, &screen_rect );

    /* Save the screen below the mouse cursor */

    graphics_driver->blit_bitmap(
        active_mouse_pointer->hidden_bitmap,
        &null_point,
        screen_bitmap,
        &mouse_rect,
        DM_COPY
    );

    /* Draw the mouse pointer to the screen */

    rect_sub_point( &mouse_rect, &mouse_position );

    graphics_driver->blit_bitmap(
        screen_bitmap,
        &mouse_position,
        active_mouse_pointer->pointer_bitmap,
        &mouse_rect,
        DM_BLEND
    );

    return 0;
}

int hide_mouse_pointer( void ) {
    rect_t hidden_rect;

    /* Restore the screen below the mouse cursor */

    rect_init(
        &hidden_rect,
        mouse_position.x,
        mouse_position.y,
        mouse_position.x + active_mouse_pointer->pointer_bitmap->width - 1,
        mouse_position.y + active_mouse_pointer->pointer_bitmap->height - 1
    );
    rect_and( &hidden_rect, &screen_rect );
    rect_sub_point( &hidden_rect, &mouse_position );

    graphics_driver->blit_bitmap(
        screen_bitmap,
        &mouse_position,
        active_mouse_pointer->hidden_bitmap,
        &hidden_rect,
        DM_COPY
    );

    return 0;
}

int mouse_moved( point_t* delta ) {
    point_add( &mouse_position, delta );

    if ( mouse_position.x < 0 ) {
        mouse_position.x = 0;
    } else if ( mouse_position.x > screen_rect.right ) {
        mouse_position.x = screen_rect.right;
    }

    if ( mouse_position.y < 0 ) {
        mouse_position.y = 0;
    } else if ( mouse_position.y > screen_rect.bottom ) {
        mouse_position.y = screen_rect.bottom;
    }

    return 0;
}

int mouse_get_position( point_t* position ) {
    point_copy( position, &mouse_position );

    return 0;
}

int mouse_get_rect( rect_t* mouse_rect ) {
    rect_init(
        mouse_rect,
        mouse_position.x,
        mouse_position.y,
        mouse_position.x + active_mouse_pointer->pointer_bitmap->width - 1,
        mouse_position.y + active_mouse_pointer->pointer_bitmap->height - 1
    );

    return 0;
}

static void* mouse_pointer_key( hashitem_t* item ) {
    mouse_pointer_t* pointer;

    pointer = ( mouse_pointer_t* )item;

    return ( void* )&pointer->id;
}

int init_mouse_manager( void ) {
    int error;
    mouse_pointer_t* pointer;

    error = init_hashtable(
        &mouse_pointer_table,
        64,
        mouse_pointer_key,
        hash_int,
        compare_int
    );

    if ( error < 0 ) {
        goto error1;
    }

    error = pthread_mutex_init( &mouse_pointer_lock, NULL );

    if ( error < 0 ) {
        goto error2;
    }

    mouse_pointer_id_counter = 0;

    pointer = create_mouse_pointer( 16, 16, CS_RGB32, default_pointer );

    if ( pointer == NULL ) {
        goto error3;
    }

    error = activate_mouse_pointer( pointer );

    if ( error < 0 ) {
        goto error4;
    }

    return 0;

error4:
    put_mouse_pointer( pointer );

error3:
    pthread_mutex_destroy( &mouse_pointer_lock );

error2:
    destroy_hashtable( &mouse_pointer_table );

error1:
    return error;
}
