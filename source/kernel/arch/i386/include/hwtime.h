/* Date and time handling
 *
 * Copyright (c) 2009 Kornel Csernai
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
#ifndef _HWTIME_H_
#define _HWTIME_H_
#include <lib/time.h>

#define RTCADDR 0x70
#define RTCDATA 0x71
/* Gets the current hardware time from RTC */
tm_t gethwclock();
/* Sets the current hardware time */
int sethwclock(const tm_t* tm);
#endif // _HWTIME_H_
