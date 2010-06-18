/* Common macro definitions
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

#ifndef _YUTIL_MACROS_H_
#define _YUTIL_MACROS_H_

#define y_return_if_fail(exp) \
    if ( !(exp) ) {           \
        return;               \
    }

#define y_return_val_if_fail(exp, val) \
    if ( !(exp) ) {                    \
        return (val);                  \
    }

#endif /* _YUTIL_MACROS_H_ */
