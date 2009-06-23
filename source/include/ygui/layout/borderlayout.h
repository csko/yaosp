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

#include <ygui/layout/layout.h>

#ifndef _YAOSP_BORDERLAYOUT_H_
#define _YAOSP_BORDERLAYOUT_H_

#define BRD_PAGE_START ((void*)1)
#define BRD_PAGE_END   ((void*)2)
#define BRD_LINE_START ((void*)3)
#define BRD_LINE_END   ((void*)4)
#define BRD_CENTER     ((void*)5)

layout_t* create_border_layout( void );

#endif /* _YAOSP_BORDERLAYOUT_H_ */
