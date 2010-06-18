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

#ifndef _YGUI_DIALOG_FILECHOOSER_H_
#define _YGUI_DIALOG_FILECHOOSER_H_

#include <ygui/window.h>

typedef enum chooser_type {
    T_OPEN_DIALOG,
    T_SAVE_DIALOG
} chooser_type_t;

typedef enum chooser_event {
    E_CHOOSER_OK,
    E_CHOOSER_CANCEL
} chooser_event_t;

struct file_chooser;

typedef int file_chooser_callback_t( struct file_chooser* chooser, chooser_event_t event, void* data );

typedef struct file_chooser {
    window_t* window;

    char* current_path;

    widget_t* path_label;
    widget_t* directory_view;
    widget_t* filename_field;

    file_chooser_callback_t* callback;
    void* data;
} file_chooser_t;

file_chooser_t* create_file_chooser( chooser_type_t type, const char* path,
                                     file_chooser_callback_t* callback, void* data );

char* file_chooser_get_selected_file( file_chooser_t* chooser );

int file_chooser_show( file_chooser_t* chooser );

#endif /* _YGUI_DIALOG_FILECHOOSER_H_ */
