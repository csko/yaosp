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

#include <yaosp/semaphore.h>
#include <yutil/array.h>

#include <windowmanager.h>
#include <graphicsdriver.h>
#include <windowdecorator.h>
#include <mouse.h>
#include <window.h>
#include <assert.h>

static array_t window_stack;
static semaphore_id wm_lock;

static window_t* mouse_window = NULL;

static rect_t moving_rect;
static window_t* moving_window = NULL;

window_decorator_t* window_decorator = &default_decorator;

static window_t* get_window_at( point_t* position ) {
    int i;
    int count;
    window_t* window;

    count = array_get_size( &window_stack );

    for ( i = 0; i < count; i++ ) {
        window = ( window_t* )array_get_item( &window_stack, i );

        if ( rect_has_point( &window->screen_rect, position ) ) {
            return window;
        }
    }

    return NULL;
}

static int invert_moving_rect( void ) {
    rect_t tmp;

    /* Top line */

    rect_init(
        &tmp,
        moving_rect.left,
        moving_rect.top,
        moving_rect.right,
        moving_rect.top
    );

    rect_and( &tmp, &screen_rect );

    if ( rect_is_valid( &tmp ) ) {
        graphics_driver->fill_rect( screen_bitmap, &tmp, NULL, DM_INVERT );
    }

    /* Bottom line */

    rect_init(
        &tmp,
        moving_rect.left,
        moving_rect.bottom,
        moving_rect.right,
        moving_rect.bottom
    );

    rect_and( &tmp, &screen_rect );

    if ( rect_is_valid( &tmp ) ) {
        graphics_driver->fill_rect( screen_bitmap, &tmp, NULL, DM_INVERT );
    }

    /* Left line */

    rect_init(
        &tmp,
        moving_rect.left,
        moving_rect.top,
        moving_rect.left,
        moving_rect.bottom
    );

    rect_and( &tmp, &screen_rect );

    if ( rect_is_valid( &tmp ) ) {
        graphics_driver->fill_rect( screen_bitmap, &tmp, NULL, DM_INVERT );
    }

    /* Right line */

    rect_init(
        &tmp,
        moving_rect.right,
        moving_rect.top,
        moving_rect.right,
        moving_rect.bottom
    );

    rect_and( &tmp, &screen_rect );

    if ( rect_is_valid( &tmp ) ) {
        graphics_driver->fill_rect( screen_bitmap, &tmp, NULL, DM_INVERT );
    }

    return 0;
}

int wm_register_window( window_t* window ) {
    int i;
    int j;
    int size;
    int error;
    rect_t winrect;
    point_t lefttop;
    window_t* tmp;
    window_t* tmp2;

    int mouse_inside;
    point_t mouse_position;

    LOCK( wm_lock );

    /* Insert the new window to the window stack */

    error = array_insert_item( &window_stack, 0, window );

    if ( error < 0 ) {
        goto error1;
    }

    /* Regenerate visible regions of other windows */

    size = array_get_size( &window_stack );

    for ( i = 1; i < size; i++ ) {
        tmp = ( window_t* )array_get_item( &window_stack, i );

        region_clear( &tmp->visible_regions );
        region_add( &tmp->visible_regions, &tmp->screen_rect );

        for ( j = i - 1; j >= 0; j-- ) {
            tmp2 = ( window_t* )array_get_item( &window_stack, j );

            region_exclude( &tmp->visible_regions, &tmp2->screen_rect );
        }
    }

    /* Update the decoration of the window */

    window_decorator->update_border( window );

    /* Check mouse position */

    mouse_get_position( &mouse_position );

    mouse_inside = rect_has_point( &window->screen_rect, &mouse_position );

    if ( mouse_inside ) {
        /* TODO: We should check the mouse rect for hiding not only the position */
        hide_mouse_pointer();
    }

    /* Draw the window */

    rect_lefttop( &window->screen_rect, &lefttop );

    memcpy( &winrect, &window->screen_rect, sizeof( rect_t ) );
    rect_sub_point( &winrect, &lefttop );

    graphics_driver->blit_bitmap(
        screen_bitmap,
        &lefttop,
        window->bitmap,
        &winrect,
        DM_COPY
    );

    /* Update the mouse window if required */

    if ( mouse_inside ) {
        if ( mouse_window != NULL ) {
            window_mouse_exited( mouse_window );
        }

        mouse_window = window;

        window_mouse_entered( mouse_window, &mouse_position );

        show_mouse_pointer();
    }

    UNLOCK( wm_lock );

    return 0;

error1:
    UNLOCK( wm_lock );

    return error;
}

int wm_mouse_moved( point_t* delta ) {
    window_t* window;
    point_t mouse_diff;
    point_t mouse_position;
    point_t old_mouse_position;

    LOCK( wm_lock );

    hide_mouse_pointer();

    mouse_get_position( &old_mouse_position );
    mouse_moved( delta );
    mouse_get_position( &mouse_position );

    point_sub_n( &mouse_diff, &mouse_position, &old_mouse_position );

    if ( moving_window != NULL ) {
        invert_moving_rect();

        rect_add_point( &moving_rect, &mouse_diff );

        invert_moving_rect();

        goto done;
    }

    window = get_window_at( &mouse_position );

    if ( mouse_window == window ) {
        if ( mouse_window != NULL ) {
            window_mouse_moved( mouse_window, &mouse_position );
        }
    } else {
        if ( mouse_window != NULL ) {
            window_mouse_exited( mouse_window );
        }

        if ( window != NULL ) {
            window_mouse_entered( window, &mouse_position );
        }

        mouse_window = window;
    }

done:
    show_mouse_pointer();

    UNLOCK( wm_lock );

    return 0;
}

int wm_mouse_pressed( int button ) {
    LOCK( wm_lock );

    if ( mouse_window != NULL ) {
        /* TODO: bring to front */

        window_mouse_pressed( mouse_window, button );
    }

    UNLOCK( wm_lock );

    return 0;
}

int wm_mouse_released( int button ) {
    LOCK( wm_lock );

    if ( mouse_window != NULL ) {
        window_mouse_released( mouse_window, button );
    }

    UNLOCK( wm_lock );

    return 0;
}

int wm_hide_window_region( window_t* window, rect_t* rect ) {
    rect_t tmp;

    color_t bg_color = { 0x11, 0x22, 0x33, 0xFF };

    rect_and_n( &tmp, rect, &screen_rect );

    graphics_driver->fill_rect(
        screen_bitmap,
        &tmp,
        &bg_color,
        DM_COPY
    );

    return 0;
}

int wm_set_moving_window( window_t* window ) {
    if ( window != NULL ) {
        assert( moving_window == NULL );
        assert( window->is_moving == 0 );

        window->is_moving = 1;

        memcpy( &moving_rect, &window->screen_rect, sizeof( rect_t ) );

        hide_mouse_pointer();
        invert_moving_rect();
        show_mouse_pointer();
    } else {
        point_t tmp;
        point_t lefttop;
        rect_t winrect;

        assert( moving_window != NULL );
        assert( moving_window->is_moving );

        moving_window->is_moving = 0;

        hide_mouse_pointer();

        invert_moving_rect();

        if ( rect_is_equal( &moving_window->screen_rect, &moving_rect ) ) {
            goto done;
        }

        wm_hide_window_region( moving_window, &moving_window->screen_rect );

        /* Draw the window */

        memcpy( &moving_window->screen_rect, &moving_rect, sizeof( rect_t ) );

        window_decorator->calculate_regions( moving_window );

        memcpy( &winrect, &moving_window->screen_rect, sizeof( rect_t ) );
        rect_and( &winrect, &screen_rect );

        rect_lefttop( &winrect, &lefttop );

        rect_lefttop( &moving_window->screen_rect, &tmp );
        rect_sub_point( &winrect, &tmp );

        graphics_driver->blit_bitmap(
            screen_bitmap,
            &lefttop,
            moving_window->bitmap,
            &winrect,
            DM_COPY
        );

done:
        show_mouse_pointer();
    }

    moving_window = window;

    return 0;
}

int init_windowmanager( void ) {
    int error;

    error = init_array( &window_stack );

    if ( error < 0 ) {
        goto error1;
    }

    array_set_realloc_size( &window_stack, 32 );

    wm_lock = create_semaphore( "Window manager lock", SEMAPHORE_BINARY, 0, 1 );

    if ( wm_lock < 0 ) {
        error = wm_lock;
        goto error2;
    }

    return 0;

error2:
    /* TODO: Delete the window stack array */

error1:
    return error;
}
