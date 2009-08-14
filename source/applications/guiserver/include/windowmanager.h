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

#ifndef _WINDOWMANAGER_H_
#define _WINDOWMANAGER_H_

#include <pthread.h>

#include <window.h>

extern pthread_mutex_t wm_lock;

int wm_register_window( window_t* window );
int wm_bring_to_front( window_t* window );

int wm_update_window_region( window_t* window, rect_t* region );
int wm_hide_window_region( window_t* window, rect_t* region );

int wm_key_pressed( int key );
int wm_key_released( int key );
int wm_mouse_moved( point_t* delta );
int wm_mouse_pressed( int button );
int wm_mouse_released( int button );

int wm_set_moving_window( window_t* window );

int init_windowmanager( void );

#endif /* _WINDOWMANAGER_H_ */
