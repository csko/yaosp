/* yaOSp GUI control application
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

#ifndef _YGUICTRL_H_
#define _YGUICTRL_H_

typedef int command_handler_t( int argc, char** argv );

typedef struct ctrl_command {
    const char* name;
    command_handler_t* handler;
} ctrl_command_t;

typedef struct ctrl_subsystem {
    const char* name;
    ctrl_command_t* commands;
} ctrl_subsystem_t;

extern char* argv0;
extern ctrl_subsystem_t screen;
extern ctrl_subsystem_t wallpaper;

#endif /* _YGUICTRL_H_ */
