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
#include <errno.h>
#include <string.h>
#include <pthread.h>

#include <region.h>

#define MAX_FREE_CLIP_RECT     64
#define INITIAL_FREE_CLIP_RECT 32

static int free_clip_rect_count = 0;
static pthread_mutex_t clip_rect_lock;
static clip_rect_t* clip_rect_list = NULL;

static clip_rect_t* get_clip_rect( void ) {
    clip_rect_t* rect;

    rect = NULL;

    pthread_mutex_lock( &clip_rect_lock );

    if ( free_clip_rect_count > 0 ) {
        assert( clip_rect_list != NULL );

        rect = clip_rect_list;
        clip_rect_list = rect->next;

        free_clip_rect_count--;
    }

    pthread_mutex_unlock( &clip_rect_lock );

    if ( rect == NULL ) {
        rect = ( clip_rect_t* )malloc( sizeof( clip_rect_t ) );

        if ( rect == NULL ) {
            return NULL;
        }
    }

    rect->next = NULL;

    return rect;
}

static void put_clip_rect( clip_rect_t* rect ) {
    pthread_mutex_lock( &clip_rect_lock );

    if ( free_clip_rect_count < MAX_FREE_CLIP_RECT ) {
        rect->next = clip_rect_list;
        clip_rect_list = rect;

        free_clip_rect_count++;

        rect = NULL;
    }

    pthread_mutex_unlock( &clip_rect_lock );

    if ( rect != NULL ) {
        free( rect );
    }
}

int init_region( region_t* region ) {
    region->rects = NULL;

    return 0;
}

int destroy_region( region_t* region ) {
    return region_clear( region );
}

int region_clear( region_t* region ) {
    clip_rect_t* rect;

    while ( region->rects != NULL ) {
        rect = region->rects;
        region->rects = rect->next;

        put_clip_rect( rect );
    }

    return 0;
}

int region_add( region_t* region, rect_t* rect ) {
    clip_rect_t* clip_rect;

    clip_rect = get_clip_rect();

    if ( clip_rect == NULL ) {
        return -ENOMEM;
    }

    rect_copy( &clip_rect->rect, rect );

    clip_rect->next = region->rects;
    region->rects = clip_rect;

    return 0;
}

int region_exclude( region_t* region, rect_t* rect ) {
    int i;
    rect_t hide;
    rect_t new_rects[ 4 ];
    clip_rect_t* new_list;
    clip_rect_t* current;

    new_list = NULL;

    while ( region->rects != NULL  ){
        current = region->rects;
        region->rects = current->next;

        rect_and_n( &hide, &current->rect, rect );

        if ( !rect_is_valid( &hide ) ) {
            current->next = new_list;
            new_list = current;

            continue;
        }

        rect_init( &new_rects[ 0 ],
            current->rect.left,
            current->rect.top,
            current->rect.right,
            hide.top - 1
        );

        rect_init( &new_rects[ 1 ],
            current->rect.left,
            hide.bottom + 1,
            current->rect.right,
            current->rect.bottom
        );

        rect_init( &new_rects[ 2 ],
            current->rect.left,
            current->rect.top,
            hide.left - 1,
            hide.bottom
        );

        rect_init( &new_rects[ 3 ],
            hide.right + 1,
            hide.top,
            current->rect.right,
            current->rect.bottom
        );

        put_clip_rect( current );

        for ( i = 0; i < 4; i++ ) {
            if ( !rect_is_valid( &new_rects[ i ] ) ) {
                continue;
            }

            current = get_clip_rect();

            if ( current == NULL ) {
                continue;
            }

            rect_copy( &current->rect, &new_rects[ i ] );

            current->next = new_list;
            new_list = current;
        }
    }

    region->rects = new_list;

    return 0;
}

int region_duplicate( region_t* old_region, region_t* new_region ) {
    clip_rect_t* clip_rect;
    clip_rect_t* new_clip_rect;
    clip_rect_t* last_clip_rect;

    last_clip_rect = NULL;
    new_region->rects = NULL;

    for ( clip_rect = old_region->rects; clip_rect != NULL; clip_rect = clip_rect->next ) {
        new_clip_rect = get_clip_rect();

        if ( new_clip_rect == NULL ) {
            goto error1;
        }

        rect_copy( &new_clip_rect->rect, &clip_rect->rect );

        if ( last_clip_rect == NULL ) {
            new_region->rects = new_clip_rect;
        } else {
            last_clip_rect->next = new_clip_rect;
        }

        last_clip_rect = new_clip_rect;
    }

    if ( last_clip_rect != NULL ) {
        last_clip_rect->next = NULL;
    }

    return 0;

 error1:
    region_clear( new_region );

    return -ENOMEM;
}

int init_region_manager( void ) {
    int i;
    int error;
    clip_rect_t* rect;

    for ( i = 0; i < INITIAL_FREE_CLIP_RECT; i++ ) {
        rect = ( clip_rect_t* )malloc( sizeof( clip_rect_t ) );

        if ( rect == NULL ) {
            return -ENOMEM;
        }

        rect->next = clip_rect_list;
        clip_rect_list = rect;
    }

    error = pthread_mutex_init( &clip_rect_lock, NULL );

    if ( error < 0 ) {
        return error;
    }

    return 0;
}
