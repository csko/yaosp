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

#include <splash.h>
#include <graphicsdriver.h>

#define PROGRESS_WIDTH 280
#define PROGRESS_HEIGHT 20
#define PROGRESS_SPACING 2

#define PROGRESS_FULL_WIDTH ( 2 /* left & right border */ + 2 * PROGRESS_SPACING + PROGRESS_WIDTH )
#define PROGRESS_FULL_HEIGHT ( 2 /* top & bottom border */ + 2 * PROGRESS_SPACING + PROGRESS_HEIGHT )

int splash_count_current = 0;
int splash_count_total = 0;

static color_t splash_bg_color = { 0, 0, 0, 255 };
static color_t splash_fg_color = { 255, 255, 255, 255 };
static color_t splash_pg_color = { 51, 102, 152, 255 };

static point_t progress_lefttop;

static int splash_paint_progress( void ) {
    rect_t tmp;
    int cur_width;

    cur_width = ( double )PROGRESS_WIDTH * splash_count_current / splash_count_total;

    tmp.left = progress_lefttop.x + 1 /* border */ + PROGRESS_SPACING;
    tmp.top = progress_lefttop.y + 1 /* border */ + PROGRESS_SPACING;
    tmp.right = tmp.left + cur_width - 1;
    tmp.bottom = tmp.top + PROGRESS_HEIGHT - 1;

    graphics_driver->fill_rect(
        screen_bitmap, &tmp,
        &splash_pg_color, DM_COPY
    );

    return 0;
}

int splash_inc_progress( void ) {
    splash_count_current++;
    splash_paint_progress();

    return 0;
}

int init_splash( void ) {
    rect_t tmp;
    int scr_width;
    int scr_height;

    /* Fill the background with black */

    graphics_driver->fill_rect(
        screen_bitmap,
        &screen_rect,
        &splash_bg_color,
        DM_COPY
    );

    /* Draw the progress bar border */

    rect_bounds( &screen_rect, &scr_width, &scr_height );

    point_init(
        &progress_lefttop,
        ( scr_width - PROGRESS_FULL_WIDTH ) / 2,
        scr_height * 2 / 3
    );

    tmp.left = progress_lefttop.x;
    tmp.top = progress_lefttop.y;
    tmp.right = tmp.left + PROGRESS_FULL_WIDTH - 1;
    tmp.bottom = tmp.top;

    graphics_driver->fill_rect(
        screen_bitmap, &tmp,
        &splash_fg_color, DM_COPY
    );

    tmp.top += PROGRESS_FULL_HEIGHT - 1;
    tmp.bottom = tmp.top;

    graphics_driver->fill_rect(
        screen_bitmap, &tmp,
        &splash_fg_color, DM_COPY
    );

    tmp.top = progress_lefttop.y;
    tmp.right = tmp.left;

    graphics_driver->fill_rect(
        screen_bitmap, &tmp,
        &splash_fg_color, DM_COPY
    );

    tmp.left += PROGRESS_FULL_WIDTH - 1;
    tmp.right += PROGRESS_FULL_WIDTH - 1;

    graphics_driver->fill_rect(
        screen_bitmap, &tmp,
        &splash_fg_color, DM_COPY
    );

    return 0;
}
