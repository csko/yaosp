/* IRC client
 *
 * Copyright (c) 2009 Zoltan Kovacs, Kornel Csernai
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

#ifndef _UI_CHANNEL_VIEW_H_
#define _UI_CHANNEL_VIEW_H_

#include "view.h"

/* TODO: rename to window maybe */
typedef struct channel_data {
    char* name;
    char* title;
} channel_data_t;

view_t* create_channel_view( const char* name );
void destroy_channel_view( view_t* view );

#endif /* _UI_CHANNEL_VIEW_H_ */
