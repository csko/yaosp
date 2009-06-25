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

#include <yutil/array.h>

#include <windowmanager.h>
#include <graphicsdriver.h>
#include <windowdecorator.h>
#include <mouse.h>
#include <window.h>
#include <assert.h>

semaphore_id wm_lock;

static array_t window_stack;

static window_t* mouse_window = NULL;
static window_t* active_window = NULL;
static window_t* mouse_down_window = NULL;

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

static int generate_visible_regions_by_window( window_t* window ) {
    int i;
    int index;
    rect_t real_screen_rect;

    index = array_index_of( &window_stack, window );

    assert( index != -1 );

    rect_and_n( &real_screen_rect, &window->screen_rect, &screen_rect );

    region_clear( &window->visible_regions );
    region_add( &window->visible_regions, &real_screen_rect );

    for ( i = index - 1; i >= 0; i-- ) {
        window_t* tmp_window;

        tmp_window = ( window_t* )array_get_item( &window_stack, i );

        region_exclude( &window->visible_regions, &tmp_window->screen_rect );
    }

    return 0;
}

static int generate_visible_regions_by_index( int index ) {
    int i;
    window_t* window;
    rect_t real_screen_rect;

    window = array_get_item( &window_stack, index );

    assert( window != NULL );

    rect_and_n( &real_screen_rect, &window->screen_rect, &screen_rect );

    region_clear( &window->visible_regions );
    region_add( &window->visible_regions, &real_screen_rect );

    for ( i = index - 1; i >= 0; i-- ) {
        window_t* tmp_window;

        tmp_window = ( window_t* )array_get_item( &window_stack, i );

        region_exclude( &window->visible_regions, &tmp_window->screen_rect );
    }

    return 0;
}

int wm_register_window( window_t* window ) {
    int i;
    int size;
    int error;
    rect_t winrect;
    point_t lefttop;

    int mouse_inside;
    rect_t mouse_rect;

    LOCK( wm_lock );

    /* Insert the new window to the window stack */

    error = array_insert_item( &window_stack, 0, window );

    if ( error < 0 ) {
        goto error1;
    }

    /* Generate visible regions of the new window */

    generate_visible_regions_by_window( window );

    /* Regenerate visible regions of other windows */

    size = array_get_size( &window_stack );

    for ( i = 1; i < size; i++ ) {
        generate_visible_regions_by_index( i );
    }

    /* Update the decoration of the window */

    if ( ( window->flags & WINDOW_NO_BORDER ) == 0 ) {
        window_decorator->update_border( window );
    }

    /* Check mouse position */

    mouse_get_rect( &mouse_rect );
    rect_and( &mouse_rect, &window->screen_rect );

    mouse_inside = rect_is_valid( &mouse_rect );

    if ( mouse_inside ) {
        hide_mouse_pointer();
    }

    /* Draw the window */

    rect_lefttop( &window->screen_rect, &lefttop );
    rect_sub_point_n( &winrect, &window->screen_rect, &lefttop );

    graphics_driver->blit_bitmap(
        screen_bitmap,
        &lefttop,
        window->bitmap,
        &winrect,
        DM_COPY
    );

    /* Update the mouse window if required */

    if ( mouse_inside ) {
        point_t mouse_position;

        if ( mouse_window != NULL ) {
            window_mouse_exited( mouse_window );
        }

        mouse_window = window;

        mouse_get_position( &mouse_position );
        window_mouse_entered( mouse_window, &mouse_position );

        show_mouse_pointer();
    }

    /* Update the active window */

    active_window = window;

    UNLOCK( wm_lock );

    return 0;

error1:
    UNLOCK( wm_lock );

    return error;
}

int wm_bring_to_front( window_t* window ) {
    int i;
    int current;

    current = array_index_of( &window_stack, window );

    if ( current < 0 ) {
        return -1;
    }

    if ( current == 0 ) {
        return 0;
    }

    array_remove_item_from( &window_stack, current );
    array_insert_item( &window_stack, 0, window );

    /* Generate the visible regions of the windows */

    for ( i = 0; i <= current; i++ ) {
        generate_visible_regions_by_index( i );
    }

    wm_update_window_region( window, &window->screen_rect );

    return 0;
}

int wm_update_window_region( window_t* window, rect_t* region ) {
    point_t lefttop;
    int mouse_hidden;
    rect_t mouse_rect;
    rect_t visible_rect;
    clip_rect_t* clip_rect;

    mouse_hidden = 0;

    mouse_get_rect( &mouse_rect );

    for ( clip_rect = window->visible_regions.rects; clip_rect != NULL; clip_rect = clip_rect->next ) {
        rect_and_n( &visible_rect, region, &clip_rect->rect );

        if ( !rect_is_valid( &visible_rect ) ) {
            continue;
        }

        if ( !mouse_hidden ) {
            rect_t hidden_mouse_rect;

            rect_and_n( &hidden_mouse_rect, &mouse_rect, &visible_rect );

            if ( rect_is_valid( &hidden_mouse_rect ) ) {
                hide_mouse_pointer();

                mouse_hidden = 1;
            }
        }

        rect_lefttop( &visible_rect, &lefttop );
        rect_sub_point_xy( &visible_rect, window->screen_rect.left, window->screen_rect.top );

        graphics_driver->blit_bitmap(
            screen_bitmap,
            &lefttop,
            window->bitmap,
            &visible_rect,
            DM_COPY
        );
    }

    if ( mouse_hidden ) {
        show_mouse_pointer();
    }

    return 0;
}

int wm_hide_window_region( window_t* window, rect_t* region ) {
    int i;
    int size;
    int error;
    int win_pos;
    int mouse_hidden;
    point_t lefttop;
    rect_t mouse_rect;
    rect_t visible_rect;
    clip_rect_t* clip_rect;

    win_pos = array_index_of( &window_stack, ( void* )window );

    assert( win_pos != -1 );

    mouse_hidden = 0;
    mouse_get_rect( &mouse_rect );

    size = array_get_size( &window_stack );

    for ( i = win_pos + 1; i < size; i++ ) {
        window_t* tmp_win;
        region_t tmp_region;

        tmp_win = ( window_t* )array_get_item( &window_stack, i );

        error = region_duplicate( &window->visible_regions, &tmp_region );

        if ( error < 0 ) {
            continue;
        }

        for ( clip_rect = tmp_region.rects; clip_rect != NULL; clip_rect = clip_rect->next ) {
            rect_and_n( &visible_rect, &clip_rect->rect, region );

            if ( !rect_is_valid( &visible_rect ) ) {
                continue;
            }

            rect_and( &visible_rect, &tmp_win->screen_rect );

            if ( !rect_is_valid( &visible_rect ) ) {
                continue;
            }

            /* Hide the mouse if it's not already hidden and we're going to
               draw above the mouse cursor */

            if ( !mouse_hidden ) {
                rect_t hidden_mouse_rect;

                rect_and_n( &hidden_mouse_rect, &mouse_rect, &visible_rect );

                if ( rect_is_valid( &hidden_mouse_rect ) ) {
                    hide_mouse_pointer();

                    mouse_hidden = 1;
                }
            }

            region_exclude( &window->visible_regions, &visible_rect );

            rect_lefttop( &visible_rect, &lefttop );
            rect_sub_point_xy( &visible_rect, tmp_win->screen_rect.left, tmp_win->screen_rect.top );

            graphics_driver->blit_bitmap(
                screen_bitmap,
                &lefttop,
                tmp_win->bitmap,
                &visible_rect,
                DM_COPY
            );
        }

        region_clear( &tmp_region );
    }

    for ( clip_rect = window->visible_regions.rects; clip_rect != NULL; clip_rect = clip_rect->next ) {
        if ( !mouse_hidden ) {
            rect_t hidden_mouse_rect;

            rect_and_n( &hidden_mouse_rect, &mouse_rect, &visible_rect );

            if ( rect_is_valid( &hidden_mouse_rect ) ) {
                hide_mouse_pointer();

                mouse_hidden = 1;
            }
        }

        static color_t tmp_color = { 0x11, 0x22, 0x33, 0x00 };

        graphics_driver->fill_rect(
            screen_bitmap,
            &clip_rect->rect,
            &tmp_color,
            DM_COPY
        );
    }

    if ( mouse_hidden ) {
        show_mouse_pointer();
    }

    return 0;
}

int wm_key_pressed( int key ) {
    LOCK( wm_lock );

    if ( active_window != NULL ) {
        window_key_pressed( active_window, key );
    }

    UNLOCK( wm_lock );

    return 0;
}

int wm_key_released( int key ) {
    LOCK( wm_lock );

    if ( active_window != NULL ) {
        window_key_released( active_window, key );
    }

    UNLOCK( wm_lock );

    return 0;
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
        /* The window under the mouse is the new active window */

        active_window = mouse_window;

        /* Bring the window to the front */

        wm_bring_to_front( mouse_window );

        /* Send the mouse event to the window */

        window_mouse_pressed( mouse_window, button );
    }

    mouse_down_window = mouse_window;

    UNLOCK( wm_lock );

    return 0;
}

int wm_mouse_released( int button ) {
    LOCK( wm_lock );

    if ( mouse_down_window != NULL ) {
        window_mouse_released( mouse_down_window, button );
        mouse_down_window = NULL;
    }

    UNLOCK( wm_lock );

    return 0;
}

int wm_set_moving_window( window_t* window ) {
    if ( window != NULL ) {
        assert( moving_window == NULL );
        assert( window->is_moving == 0 );

        window->is_moving = 1;

        rect_copy( &moving_rect, &window->screen_rect );

        invert_moving_rect();
    } else {
        int i;
        int size;
        int index;

        assert( moving_window != NULL );
        assert( moving_window->is_moving );

        moving_window->is_moving = 0;

        invert_moving_rect();

        if ( rect_is_equal( &moving_window->screen_rect, &moving_rect ) ) {
            goto done;
        }

        wm_hide_window_region( moving_window, &moving_window->screen_rect );

        /* Update the window rects */

        rect_copy( &moving_window->screen_rect, &moving_rect );
        rect_copy( &moving_window->client_rect, &moving_rect );

        if ( ( moving_window->flags & WINDOW_NO_BORDER ) == 0 ) {
            rect_resize(
                &moving_window->client_rect,
                window_decorator->lefttop_offset.x,
                window_decorator->lefttop_offset.y,
                -( window_decorator->border_size.x - window_decorator->lefttop_offset.x ),
                -( window_decorator->border_size.y - window_decorator->lefttop_offset.y )
            );

            /* Update the window decorator informations */

            window_decorator->calculate_regions( moving_window );
        }

        /* Regenerate the visible rects of the moved window and the other below */

        size = array_get_size( &window_stack );
        index = array_index_of( &window_stack, moving_window );

        assert( index != -1 );

        for ( i = index; i < size; i++ ) {
            generate_visible_regions_by_index( i );
        }

        /* Update the moved window on the screen */

        wm_update_window_region( moving_window, &moving_window->screen_rect );
done:
        ;
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
