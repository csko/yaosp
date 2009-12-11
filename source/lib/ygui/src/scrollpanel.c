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

#include <stdlib.h>

#include <ygui/scrollpanel.h>

#define SCROLL_BAR_SIZE 15
#define SCROLL_BTN_SIZE 15

typedef struct scrollbar {
    int visible;
    int policy;
    rect_t prev;
    rect_t next;

    rect_t slider;
    int slider_space;

    int dragging;
    point_t drag_position;
} scrollbar_t;

typedef struct scrollpanel {
    scrollbar_t vertical;
    scrollbar_t horizontal;
} scrollpanel_t;

static void scrollbar_calc_vertical_slider( widget_t* widget, scrollbar_t* scrollbar, widget_t* child, int other_visible );
static void scrollbar_calc_horizontal_slider( widget_t* widget, scrollbar_t* scrollbar, widget_t* child, int other_visible );

static void paint_v_scrollbar( widget_t* widget, scrollbar_t* scrollbar, gc_t* gc ) {
    rect_t tmp;
    rect_t bounds;

    static color_t fg_color = { 0, 0, 0, 0xFF };
    static color_t darker_bg_color = { 176, 176, 176, 0xFF };

    widget_get_bounds( widget, &bounds );

    gc_translate_xy(
        gc,
        bounds.right - SCROLL_BAR_SIZE + 1,
        0
    );

    gc_set_pen_color( gc, &fg_color );

    /* Left border */

    rect_init(
        &tmp,
        0,
        scrollbar->prev.top,
        0,
        scrollbar->next.bottom
    );

    gc_fill_rect( gc, &tmp );

    /* Right border */

    rect_init(
        &tmp,
        SCROLL_BAR_SIZE - 1,
        scrollbar->prev.top,
        SCROLL_BAR_SIZE - 1,
        scrollbar->next.bottom
    );

    gc_fill_rect( gc, &tmp );

    /* Up button */

    rect_init(
        &tmp,
        1,
        scrollbar->prev.top,
        SCROLL_BAR_SIZE - 2,
        scrollbar->prev.top
    );

    gc_fill_rect( gc, &tmp );

    rect_init(
        &tmp,
        1,
        scrollbar->prev.bottom,
        SCROLL_BAR_SIZE - 2,
        scrollbar->prev.bottom
    );

    gc_fill_rect( gc, &tmp );

    /* ^ */

    rect_init(
        &tmp,
        rect_width( &scrollbar->prev ) / 2,
        ( SCROLL_BAR_SIZE - 3 ) / 2,
        rect_width( &scrollbar->prev ) / 2,
        ( SCROLL_BAR_SIZE - 3 ) / 2
    );
    gc_fill_rect( gc, &tmp );
    tmp.top++; tmp.bottom++;
    tmp.left--; tmp.right--;
    gc_fill_rect( gc, &tmp );
    tmp.left += 2; tmp.right += 2;
    gc_fill_rect( gc, &tmp );
    tmp.top++; tmp.bottom++;
    tmp.left++; tmp.right++;
    gc_fill_rect( gc, &tmp );
    tmp.left -= 4; tmp.right -= 4;
    gc_fill_rect( gc, &tmp );

    /* Down button */

    rect_init(
        &tmp,
        1,
        scrollbar->next.top,
        SCROLL_BAR_SIZE - 2,
        scrollbar->next.top
    );

    gc_fill_rect( gc, &tmp );

    rect_init(
        &tmp,
        1,
        scrollbar->next.bottom,
        SCROLL_BAR_SIZE - 2,
        scrollbar->next.bottom
    );

    gc_fill_rect( gc, &tmp );

    /* V */

    rect_init(
        &tmp,
        rect_width( &scrollbar->next ) / 2,
        scrollbar->next.top + ( SCROLL_BAR_SIZE - 3 ) / 2 + 3 - 1,
        rect_width( &scrollbar->next ) / 2,
        scrollbar->next.top + ( SCROLL_BAR_SIZE - 3 ) / 2 + 3 - 1
    );
    gc_fill_rect( gc, &tmp );
    tmp.top--; tmp.bottom--;
    tmp.left--; tmp.right--;
    gc_fill_rect( gc, &tmp );
    tmp.left += 2; tmp.right += 2;
    gc_fill_rect( gc, &tmp );
    tmp.top--; tmp.bottom--;
    tmp.left++; tmp.right++;
    gc_fill_rect( gc, &tmp );
    tmp.left -= 4; tmp.right -=4 ;
    gc_fill_rect( gc, &tmp );

    /* Slider */

    rect_init(
        &tmp,
        1,
        scrollbar->slider.top,
        SCROLL_BAR_SIZE - 2,
        scrollbar->slider.top
    );

    gc_fill_rect( gc, &tmp );

    rect_init(
        &tmp,
        1,
        scrollbar->slider.bottom,
        SCROLL_BAR_SIZE - 2,
        scrollbar->slider.bottom
    );

    gc_fill_rect( gc, &tmp );

    /* Space before the slider */

    rect_init(
        &tmp,
        1,
        scrollbar->prev.bottom + 1,
        SCROLL_BAR_SIZE - 2,
        scrollbar->slider.top - 1
    );

    gc_set_pen_color( gc, &darker_bg_color );
    gc_fill_rect( gc, &tmp );

    /* Space after the slider */

    rect_init(
        &tmp,
        1,
        scrollbar->slider.bottom + 1,
        SCROLL_BAR_SIZE - 2,
        scrollbar->next.top - 1
    );

    gc_fill_rect( gc, &tmp );

    gc_translate_xy(
        gc,
        -( bounds.right - SCROLL_BAR_SIZE + 1 ),
        0
    );
}

static void paint_h_scrollbar( widget_t* widget, scrollbar_t* scrollbar, gc_t* gc ) {
    rect_t tmp;
    rect_t bounds;

    static color_t fg_color = { 0, 0, 0, 0xFF };
    static color_t darker_bg_color = { 176, 176, 176, 0xFF };

    widget_get_bounds( widget, &bounds );

    gc_translate_xy(
        gc,
        0,
        bounds.bottom - SCROLL_BAR_SIZE + 1
    );

    gc_set_pen_color( gc, &fg_color );

    /* Top border */

    rect_init(
        &tmp,
        scrollbar->prev.left,
        0,
        scrollbar->next.right,
        0
    );

    gc_fill_rect( gc, &tmp );

    /* Bottom border */

    rect_init(
        &tmp,
        scrollbar->prev.left,
        SCROLL_BAR_SIZE - 1,
        scrollbar->next.right,
        SCROLL_BAR_SIZE - 1
    );

    gc_fill_rect( gc, &tmp );

    /* Up button */

    rect_init(
        &tmp,
        scrollbar->prev.left,
        1,
        scrollbar->prev.left,
        SCROLL_BAR_SIZE - 2
    );

    gc_fill_rect( gc, &tmp );

    rect_init(
        &tmp,
        scrollbar->prev.right,
        1,
        scrollbar->prev.right,
        SCROLL_BAR_SIZE - 2
    );

    gc_fill_rect( gc, &tmp );

    /* < */

    rect_init(
        &tmp,
        ( SCROLL_BAR_SIZE - 3 ) / 2,
        rect_height( &scrollbar->prev ) / 2,
        ( SCROLL_BAR_SIZE - 3 ) / 2,
        rect_height( &scrollbar->prev ) / 2
    );
    gc_fill_rect( gc, &tmp );
    tmp.left++; tmp.right++;
    tmp.top--; tmp.bottom--;
    gc_fill_rect( gc, &tmp );
    tmp.top += 2; tmp.bottom += 2;
    gc_fill_rect( gc, &tmp );
    tmp.left++; tmp.right++;
    tmp.top++; tmp.bottom++;
    gc_fill_rect( gc, &tmp );
    tmp.top -= 4; tmp.bottom -= 4;
    gc_fill_rect( gc, &tmp );

    /* Down button */

    rect_init(
        &tmp,
        scrollbar->next.left,
        1,
        scrollbar->next.left,
        SCROLL_BAR_SIZE - 2
    );

    gc_fill_rect( gc, &tmp );

    rect_init(
        &tmp,
        scrollbar->next.right,
        1,
        scrollbar->next.right,
        SCROLL_BAR_SIZE - 2
    );

    gc_fill_rect( gc, &tmp );

    /* > */

    rect_init(
        &tmp,
        scrollbar->next.left + ( SCROLL_BAR_SIZE - 3 ) / 2 + 2,
        rect_height( &scrollbar->next ) / 2,
        scrollbar->next.left + ( SCROLL_BAR_SIZE - 3 ) / 2 + 2,
        rect_height( &scrollbar->next ) / 2
    );
    gc_fill_rect( gc, &tmp );
    tmp.left--; tmp.right--;
    tmp.top--; tmp.bottom--;
    gc_fill_rect( gc, &tmp );
    tmp.top += 2; tmp.bottom += 2;
    gc_fill_rect( gc, &tmp );
    tmp.left--; tmp.right--;
    tmp.top++; tmp.bottom++;
    gc_fill_rect( gc, &tmp );
    tmp.top -= 4; tmp.bottom -= 4;
    gc_fill_rect( gc, &tmp );

    /* Slider */

    rect_init(
        &tmp,
        scrollbar->slider.left,
        1,
        scrollbar->slider.left,
        SCROLL_BAR_SIZE - 2
    );

    gc_fill_rect( gc, &tmp );

    rect_init(
        &tmp,
        scrollbar->slider.right,
        1,
        scrollbar->slider.right,
        SCROLL_BAR_SIZE - 2
    );

    gc_fill_rect( gc, &tmp );

    /* Space before the slider */

    rect_init(
        &tmp,
        scrollbar->prev.right + 1,
        1,
        SCROLL_BAR_SIZE - 2,
        scrollbar->slider.left - 1
    );

    gc_set_pen_color( gc, &darker_bg_color );
    gc_fill_rect( gc, &tmp );

    /* Space after the slider */

    rect_init(
        &tmp,
        scrollbar->slider.right + 1,
        1,
        scrollbar->next.left - 1,
        SCROLL_BAR_SIZE - 2
    );

    gc_fill_rect( gc, &tmp );

    gc_translate_xy(
        gc,
        0,
        -( bounds.bottom - SCROLL_BAR_SIZE + 1 )
    );
}

static int scrollpanel_paint( widget_t* widget, gc_t* gc ) {
    rect_t bounds;
    scrollpanel_t* scrollpanel;

    static color_t bg_color = { 216, 216, 216, 0xFF };

    widget_get_bounds( widget, &bounds );

    /* Fill the widget with the background color */

    gc_set_pen_color( gc, &bg_color );
    gc_fill_rect( gc, &bounds );

    scrollpanel = ( scrollpanel_t* )widget_get_data( widget );

    /* Paint the vertical scrollbar */

    if ( scrollpanel->vertical.visible ) {
        paint_v_scrollbar( widget, &scrollpanel->vertical, gc );
    }

    /* Paint the horizontal scrollbar */

    if ( scrollpanel->horizontal.visible ) {
        paint_h_scrollbar( widget, &scrollpanel->horizontal, gc );
    }

    return 0;
}

static void do_v_scroll( widget_t* widget, scrollbar_t* scrollbar, int amount ) {
    widget_t* child;
    scrollpanel_t* scrollpanel;

    if ( amount == 0 ) {
        return;
    }

    scrollpanel = ( scrollpanel_t* )widget_get_data( widget );

    /* Get the child widget */

    if ( widget_get_child_count( widget ) == 0 ) {
        return;
    }

    child = widget_get_child_at( widget, 0 );

    /* Update the scroll offset of the child widget */

    child->scroll_offset.y += amount;

    if ( child->scroll_offset.y > 0 ) {
        child->scroll_offset.y = 0;
    } else if ( -child->scroll_offset.y > child->full_size.y - child->visible_size.y ) {
        child->scroll_offset.y = MIN( 0, child->visible_size.y - child->full_size.y );
    }

    /* Recalculate the position of the vertical slider */

    scrollbar_calc_vertical_slider( widget, scrollbar, child, scrollpanel->horizontal.visible );

    /* Invalidate the scrollpanel */

    widget_invalidate( widget );
}

static void do_v_scroll_set( widget_t* widget, scrollbar_t* scrollbar, int value ) {
    widget_t* child;
    scrollpanel_t* scrollpanel;

    scrollpanel = ( scrollpanel_t* )widget_get_data( widget );

    /* Get the child widget */

    if ( widget_get_child_count( widget ) == 0 ) {
        return;
    }

    child = widget_get_child_at( widget, 0 );

    /* Update the scroll offset of the child widget */

    if ( child->scroll_offset.y == value ) {
        return;
    }

    child->scroll_offset.y = value;

    if ( child->scroll_offset.y > 0 ) {
        child->scroll_offset.y = 0;
    } else if ( -child->scroll_offset.y > child->full_size.y - child->visible_size.y ) {
        child->scroll_offset.y = MIN( 0, child->visible_size.y - child->full_size.y );
    }

    /* Recalculate the position of the vertical slider */

    scrollbar_calc_vertical_slider( widget, scrollbar, child, scrollpanel->horizontal.visible );

    /* Invalidate the scrollpanel */

    widget_invalidate( widget );
}

static void do_v_scroll_to( widget_t* widget, scrollbar_t* scrollbar ) {
    double p;
    int new_offset;
    widget_t* child;

    /* Get the child widget */

    if ( widget_get_child_count( widget ) == 0 ) {
        return;
    }

    child = widget_get_child_at( widget, 0 );

    if ( child->full_size.y <= child->visible_size.y ) {
        return;
    }

    p = ( double )( scrollbar->slider.top - SCROLL_BTN_SIZE ) / scrollbar->slider_space;

    new_offset = -( ( int )( ( child->full_size.y - child->visible_size.y ) * p ) );

    if ( child->scroll_offset.y == new_offset ) {
        return;
    }

    child->scroll_offset.y = new_offset;

    widget_invalidate( widget );
}

static void do_h_scroll( widget_t* widget, scrollbar_t* scrollbar, int amount ) {
    widget_t* child;
    scrollpanel_t* scrollpanel;

    if ( amount == 0 ) {
        return;
    }

    scrollpanel = ( scrollpanel_t* )widget_get_data( widget );

    /* Get the child widget */

    if ( widget_get_child_count( widget ) == 0 ) {
        return;
    }

    child = widget_get_child_at( widget, 0 );

    /* Update the scroll offset of the child widget */

    child->scroll_offset.x += amount;

    if ( child->scroll_offset.x > 0 ) {
        child->scroll_offset.x = 0;
    } else if ( -child->scroll_offset.x > child->full_size.x - child->visible_size.x ) {
        child->scroll_offset.x = MIN( 0, child->visible_size.x - child->full_size.x );
    }

    /* Recalculate the position of the vertical slider */

    scrollbar_calc_horizontal_slider( widget, scrollbar, child, scrollpanel->vertical.visible );

    /* Invalidate the scrollpanel */

    widget_invalidate( widget );
}

static void do_h_scroll_to( widget_t* widget, scrollbar_t* scrollbar ) {
    double p;
    int new_offset;
    widget_t* child;

    /* Get the child widget */

    if ( widget_get_child_count( widget ) == 0 ) {
        return;
    }

    child = widget_get_child_at( widget, 0 );

    if ( child->full_size.x <= child->visible_size.x ) {
        return;
    }

    p = ( double )( scrollbar->slider.left - SCROLL_BTN_SIZE ) / scrollbar->slider_space;

    new_offset = -( ( int )( ( child->full_size.x - child->visible_size.x ) * p ) );

    if ( child->scroll_offset.x == new_offset ) {
        return;
    }

    child->scroll_offset.x = new_offset;

    widget_invalidate( widget );
}

static void scrollpanel_vertical_drag( widget_t* widget, scrollbar_t* scrollbar, point_t* point ) {
    rect_t bounds;
    point_t offset;

    widget_get_bounds( widget, &bounds );

    point_sub_n( &offset, point, &scrollbar->drag_position );
    point_copy( &scrollbar->drag_position, point );

    offset.x = 0;

    rect_add_point( &scrollbar->slider, &offset );

    if ( scrollbar->slider.top < SCROLL_BTN_SIZE ) {
        rect_add_point_xy(
            &scrollbar->slider,
            0,
            SCROLL_BTN_SIZE - scrollbar->slider.top
        );
    } else if ( scrollbar->slider.bottom >= scrollbar->next.top ) {
        rect_sub_point_xy(
            &scrollbar->slider,
            0,
            scrollbar->slider.bottom - scrollbar->next.top + 1
        );
    }

    do_v_scroll_to( widget, scrollbar );
}

static void scrollpanel_horizontal_drag( widget_t* widget, scrollbar_t* scrollbar, point_t* point ) {
    rect_t bounds;
    point_t offset;

    widget_get_bounds( widget, &bounds );

    point_sub_n( &offset, point, &scrollbar->drag_position );
    point_copy( &scrollbar->drag_position, point );

    offset.y = 0;

    rect_add_point( &scrollbar->slider, &offset );

    if ( scrollbar->slider.left < SCROLL_BTN_SIZE ) {
        rect_add_point_xy(
            &scrollbar->slider,
            SCROLL_BTN_SIZE - scrollbar->slider.left,
            0
        );
    } else if ( scrollbar->slider.right >= scrollbar->next.left ) {
        rect_sub_point_xy(
            &scrollbar->slider,
            scrollbar->slider.right - scrollbar->next.left + 1,
            0
        );
    }

    do_h_scroll_to( widget, scrollbar );
}

static int scrollpanel_mouse_moved( widget_t* widget, point_t* point ) {
    scrollpanel_t* scrollpanel;

    scrollpanel = ( scrollpanel_t* )widget_get_data( widget );

    if ( scrollpanel->vertical.dragging ) {
        scrollpanel_vertical_drag( widget, &scrollpanel->vertical, point );

        return 0;
    }

    if ( scrollpanel->horizontal.dragging ) {
        scrollpanel_horizontal_drag( widget, &scrollpanel->horizontal, point );

        return 0;
    }

    return 0;
}

typedef enum scrollbar_hit_type {
    HIT_NONE,
    HIT_PREV_BTN,
    HIT_NEXT_BTN,
    HIT_SCROLL_BAR,
    HIT_BEFORE_SCROLL_BAR,
    HIT_AFTER_SCROLL_BAR
} scrollbar_hit_type_t;

static scrollbar_hit_type_t scrollbar_check_hit( scrollbar_t* scrollbar, point_t* point ) {
    if ( rect_has_point( &scrollbar->prev, point ) ) {
        return HIT_PREV_BTN;
    } else if ( rect_has_point( &scrollbar->next, point ) ) {
        return HIT_NEXT_BTN;
    } else if ( rect_has_point( &scrollbar->slider, point ) ) {
        return HIT_SCROLL_BAR;
    } else {
        return HIT_NONE;
    }
}

static int scrollpanel_mouse_pressed( widget_t* widget, point_t* point, int button ) {
    scrollpanel_t* scrollpanel;
    scrollbar_hit_type_t hit_type;

    scrollpanel = ( scrollpanel_t* )widget_get_data( widget );

    /* Check vertical scrollbar buttons */

    hit_type = scrollbar_check_hit( &scrollpanel->vertical, point );

    if ( hit_type != HIT_NONE ) {
        switch ( ( int )hit_type ) {
            case HIT_PREV_BTN :
                do_v_scroll( widget, &scrollpanel->vertical, 15 );
                break;

            case HIT_NEXT_BTN :
                do_v_scroll( widget, &scrollpanel->vertical, -15 );
                break;

            case HIT_BEFORE_SCROLL_BAR :
                break;

            case HIT_AFTER_SCROLL_BAR :
                break;

            case HIT_SCROLL_BAR :
                scrollpanel->vertical.dragging = 1;
                point_copy( &scrollpanel->vertical.drag_position, point );

                break;
        }

        return 0;
    }

    /* Check horizontal scrollbar buttons */

    hit_type = scrollbar_check_hit( &scrollpanel->horizontal, point );

    if ( hit_type != HIT_NONE ) {
        switch ( ( int )hit_type ) {
            case HIT_PREV_BTN :
                do_h_scroll( widget, &scrollpanel->horizontal, 15 );
                break;

            case HIT_NEXT_BTN :
                do_h_scroll( widget, &scrollpanel->horizontal, -15 );
                break;

            case HIT_BEFORE_SCROLL_BAR :
                break;

            case HIT_AFTER_SCROLL_BAR :
                break;

            case HIT_SCROLL_BAR :
                scrollpanel->horizontal.dragging = 1;
                point_copy( &scrollpanel->horizontal.drag_position, point );

                break;
        }

        return 0;
    }

    return 0;
}

static int scrollpanel_mouse_released( widget_t* widget, int button ) {
    scrollpanel_t* scrollpanel;

    scrollpanel = ( scrollpanel_t* )widget_get_data( widget );

    /* Check vertical scrollbar */

    if ( scrollpanel->vertical.dragging ) {
        scrollpanel->vertical.dragging = 0;
    }

    /* Check horizontal scrollbar */

    if ( scrollpanel->horizontal.dragging ) {
        scrollpanel->horizontal.dragging = 0;
    }

    return 0;
}

static int scrollpanel_get_preferred_size( widget_t* widget, point_t* size ) {
    scrollpanel_t* scrollpanel;

    scrollpanel = ( scrollpanel_t* )widget_get_data( widget );

    if ( widget_get_child_count( widget ) == 0 ) {
        point_init(
            size,
            0,
            0
        );

        if ( scrollpanel->vertical.policy == SCROLLBAR_ALWAYS ) {
            size->x += SCROLL_BAR_SIZE;
        }

        if ( scrollpanel->horizontal.policy == SCROLLBAR_ALWAYS ) {
            size->y += SCROLL_BAR_SIZE;
        }

        return 0;
    }

    widget_get_preferred_size( widget_get_child_at( widget, 0 ), size );

    if ( scrollpanel->vertical.visible ) {
        size->x += SCROLL_BAR_SIZE;
    }

    if ( scrollpanel->horizontal.visible ) {
        size->y += SCROLL_BAR_SIZE;
    }

    return 0;
}

static void scrollbar_calc_vertical_slider( widget_t* widget, scrollbar_t* scrollbar, widget_t* child, int other_visible ) {
    rect_t bounds;
    int slider_size;
    int slider_position;
    int max_slider_size;

    widget_get_bounds( widget, &bounds );

    max_slider_size = rect_height( &bounds ) - 2 * SCROLL_BTN_SIZE - ( other_visible ? SCROLL_BAR_SIZE : 0 );

    if ( child->full_size.y > child->visible_size.y ) {
        double p;

        p = ( double )child->visible_size.y / child->full_size.y;

        slider_size = ( int )( max_slider_size * p );

        if ( slider_size < 5 ) {
            slider_size = 5;
        }
    } else {
        slider_size = max_slider_size;
    }

    scrollbar->slider_space = max_slider_size - slider_size;

    if ( child->full_size.y > child->visible_size.y ) {
        double p;

        p = ( double )-child->scroll_offset.y / ( child->full_size.y - child->visible_size.y );
        slider_position = ( int )( scrollbar->slider_space * p );
    } else {
        slider_position = 0;
    }

    rect_init(
        &scrollbar->slider,
        bounds.right - SCROLL_BAR_SIZE + 1,
        scrollbar->prev.bottom + slider_position + 1,
        bounds.right,
        scrollbar->prev.bottom + slider_position + slider_size
    );
}

static void scrollpanel_calc_vertical_bar( widget_t* widget, scrollbar_t* scrollbar, widget_t* child, int other_visible ) {
    rect_t bounds;

    widget_get_bounds( widget, &bounds );

    /* Prev button */

    rect_init(
        &scrollbar->prev,
        bounds.right - SCROLL_BAR_SIZE + 1,
        bounds.top,
        bounds.right,
        bounds.top + SCROLL_BTN_SIZE - 1
    );

    /* Next button */

    rect_init(
        &scrollbar->next,
        bounds.right - SCROLL_BAR_SIZE + 1,
        bounds.bottom - SCROLL_BTN_SIZE + 1,
        bounds.right,
        bounds.bottom
    );

    if ( other_visible ) {
        rect_resize(
            &scrollbar->next,
            0,
            -SCROLL_BAR_SIZE,
            0,
            -SCROLL_BAR_SIZE
        );
    }

    /* Slider */

    scrollbar_calc_vertical_slider( widget, scrollbar, child, other_visible );
}

static void scrollbar_calc_horizontal_slider( widget_t* widget, scrollbar_t* scrollbar, widget_t* child, int other_visible ) {
    rect_t bounds;
    int slider_size;
    int slider_position;
    int max_slider_size;

    widget_get_bounds( widget, &bounds );

    max_slider_size = rect_width( &bounds ) - 2 * SCROLL_BTN_SIZE - ( other_visible ? SCROLL_BAR_SIZE : 0 );

    if ( child->full_size.x > child->visible_size.x ) {
        double p;

        p = ( double )child->visible_size.x / child->full_size.x;

        slider_size = ( int )( max_slider_size * p );

        if ( slider_size < 5 ) {
            slider_size = 5;
        }
    } else {
        slider_size = max_slider_size;
    }

    scrollbar->slider_space = max_slider_size - slider_size;

    if ( child->full_size.x > child->visible_size.x ) {
        double p;

        p = ( double )-child->scroll_offset.x / ( child->full_size.x - child->visible_size.x );

        slider_position = ( int )( scrollbar->slider_space * p );
    } else {
        slider_position = 0;
    }

    rect_init(
        &scrollbar->slider,
        scrollbar->prev.right + slider_position + 1,
        bounds.bottom - SCROLL_BAR_SIZE + 1,
        scrollbar->prev.right + slider_position + slider_size,
        bounds.bottom
    );
}

static void scrollpanel_calc_horizontal_bar( widget_t* widget, scrollbar_t* scrollbar, widget_t* child, int other_visible ) {
    rect_t bounds;

    widget_get_bounds( widget, &bounds );

    /* Prev button */

    rect_init(
        &scrollbar->prev,
        bounds.left,
        bounds.bottom - SCROLL_BAR_SIZE + 1,
        bounds.left + SCROLL_BTN_SIZE - 1,
        bounds.bottom
    );

    /* Next button */

    rect_init(
        &scrollbar->next,
        bounds.right - SCROLL_BTN_SIZE + 1,
        bounds.bottom - SCROLL_BAR_SIZE + 1,
        bounds.right,
        bounds.bottom
    );

    if ( other_visible ) {
        rect_resize(
            &scrollbar->next,
            -SCROLL_BAR_SIZE,
            0,
            -SCROLL_BAR_SIZE,
            0
        );
    }

    /* Slider */

    scrollbar_calc_horizontal_slider( widget, scrollbar, child, other_visible );
}

static int scrollpanel_size_changed( widget_t* widget ) {
    widget_t* child;
    point_t position;
    point_t visible_size;
    point_t preferred_size;
    scrollpanel_t* scrollpanel;

    scrollpanel = ( scrollpanel_t* )widget_get_data( widget );

    if ( widget_get_child_count( widget ) == 0 ) {
        return 0;
    }

    child = widget_get_child_at( widget, 0 );

    widget_get_preferred_size( child, &preferred_size );

    /* Vertical scrollbar */

    switch ( scrollpanel->vertical.policy ) {
        case SCROLLBAR_NEVER :
            visible_size.x = widget->visible_size.x;
            break;

        case SCROLLBAR_AUTO :
            /* todo */
            break;

        case SCROLLBAR_ALWAYS :
            scrollpanel->vertical.visible = 1;
            visible_size.x = widget->visible_size.x - SCROLL_BAR_SIZE;
            break;
    }

    /* Horizontal scrollbar */

    switch ( scrollpanel->horizontal.policy ) {
        case SCROLLBAR_NEVER :
            visible_size.y = widget->visible_size.y;
            break;

        case SCROLLBAR_AUTO :
            /* todo */
            break;

        case SCROLLBAR_ALWAYS :
            scrollpanel->horizontal.visible = 1;
            visible_size.y = widget->visible_size.y - SCROLL_BAR_SIZE;
            break;
    }

    /* Set the position and the size of the child widget */

    point_init(
        &position,
        0,
        0
    );
    point_max( &preferred_size, &visible_size );

    widget_set_position_and_sizes(
        child, &position, &visible_size, &preferred_size
    );

    /* Calculate scrollbar values */

    if ( scrollpanel->vertical.visible ) {
        scrollpanel_calc_vertical_bar( widget, &scrollpanel->vertical, child, scrollpanel->horizontal.visible );
    }

    if ( scrollpanel->horizontal.visible ) {
        scrollpanel_calc_horizontal_bar( widget, &scrollpanel->horizontal, child, scrollpanel->vertical.visible );
    }

    return 0;
}

static int child_preferred_size_changed( widget_t* widget, void* data ) {
    widget_t* parent;
    scrollpanel_t* scrollpanel;
    point_t pref_size;

    parent = ( widget_t* )data;
    scrollpanel = widget_get_data( parent );

    /* Update the full size of the child widget */

    widget_get_preferred_size( widget, &pref_size );
    point_max_n( &widget->full_size, &pref_size, &widget->visible_size );

    /* Update the scrollbars */

    if ( scrollpanel->vertical.visible ) {
        scrollpanel_calc_vertical_bar( parent, &scrollpanel->vertical, widget, scrollpanel->horizontal.visible );
    }

    if ( scrollpanel->horizontal.visible ) {
        scrollpanel_calc_horizontal_bar( parent, &scrollpanel->horizontal, widget, scrollpanel->vertical.visible );
    }

    /* Request a repaint on the scrollpanel widget */

    widget_invalidate( parent );

    return 0;
}

static int child_viewport_changed( widget_t* widget, void* data ) {
    widget_t* parent;
    scrollpanel_t* scrollpanel;
    rect_t viewport;
    rect_t visible_rect;

    parent = ( widget_t* )data;
    scrollpanel = widget_get_data( parent );

    widget_get_viewport( widget, &viewport );

    visible_rect.left = -widget->scroll_offset.x;
    visible_rect.top = -widget->scroll_offset.y;
    visible_rect.right = visible_rect.left + widget->visible_size.x - 1;
    visible_rect.bottom = visible_rect.top + widget->visible_size.y - 1;

    /* Check if we have to change any of the scrollbars */

    if ( rect_has_rect( &visible_rect, &viewport ) ) {
        return 0;
    }

    if ( viewport.top < visible_rect.top ) {
        widget->scroll_offset.y = -viewport.top;
    } else if ( viewport.bottom > visible_rect.bottom ) {
        widget->scroll_offset.y -= viewport.bottom - visible_rect.bottom;
    }

    if ( scrollpanel->vertical.visible ) {
        scrollpanel_calc_vertical_bar( parent, &scrollpanel->vertical, widget, scrollpanel->horizontal.visible );
    }

    /* Request a repaint on the scrollpanel widget */

    widget_invalidate( parent );

    return 0;
}

static int scrollpanel_child_added( widget_t* widget, widget_t* child ) {
    widget_connect_event_handler(
        child,
        "preferred-size-changed",
        child_preferred_size_changed,
        ( void* )widget
    );

    widget_connect_event_handler(
        child,
        "viewport-changed",
        child_viewport_changed,
        ( void* )widget
    );

    return 0;
}

static widget_operations_t scrollpanel_ops = {
    .paint = scrollpanel_paint,
    .key_pressed = NULL,
    .key_released = NULL,
    .mouse_entered = NULL,
    .mouse_exited = NULL,
    .mouse_moved = scrollpanel_mouse_moved,
    .mouse_pressed = scrollpanel_mouse_pressed,
    .mouse_released = scrollpanel_mouse_released,
    .get_minimum_size = NULL,
    .get_preferred_size = scrollpanel_get_preferred_size,
    .get_maximum_size = NULL,
    .do_validate = NULL,
    .size_changed = scrollpanel_size_changed,
    .added_to_window = NULL,
    .child_added = scrollpanel_child_added,
    .destroy = NULL
};

int scroll_panel_get_v_size( widget_t* widget ) {
    widget_t* child;

    if ( ( widget_get_id( widget ) != W_SCROLLPANEL ) ||
         ( widget_get_child_count( widget ) == 0 ) ) {
        return 0;
    }

    child = widget_get_child_at( widget, 0 );

    return -MIN( 0, child->visible_size.y - child->full_size.y );
}

int scroll_panel_set_v_offset( widget_t* widget, int offset ) {
    scrollpanel_t* scrollpanel;

    if ( widget_get_id( widget ) != W_SCROLLPANEL ) {
        return -1;
    }

    scrollpanel = widget_get_data( widget );

    do_v_scroll_set( widget, &scrollpanel->vertical, -offset );

    return 0;
}

widget_t* create_scroll_panel( scrollbar_policy_t v_scroll_policy, scrollbar_policy_t h_scroll_policy ) {
    widget_t* widget;
    scrollpanel_t* scrollpanel;

    scrollpanel = ( scrollpanel_t* )malloc( sizeof( scrollpanel_t ) );

    if ( scrollpanel == NULL ) {
        goto error1;
    }

    widget = create_widget( W_SCROLLPANEL, &scrollpanel_ops, scrollpanel );

    if ( widget == NULL ) {
        goto error2;
    }

    scrollpanel->vertical.policy = v_scroll_policy;
    scrollpanel->horizontal.policy = h_scroll_policy;

    return widget;

 error2:
    free( scrollpanel );

 error1:
    return NULL;
}
