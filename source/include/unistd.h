/* yaosp C library
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

#ifndef _UNISTD_H_
#define _UNISTD_H_

#include <sys/types.h>

#define NAME_MAX 255

struct dirent {
    ino_t inode_number;
    char name[ NAME_MAX + 1 ];
};

void exit( int status );
void _exit( int status );

pid_t fork( void );

int execve( const char* filename, char* const argv[], char* const envp[] );
int execvp( const char* filename, char* const argv[] );

void* sbrk( int increment );

int close( int fd );
int dup2( int old_fd, int new_fd );

ssize_t read( int fd, void* buf, size_t count );
ssize_t write( int fd, const void* buf, size_t count );

int isatty( int fd );
int fchdir( int fd );
int getdents( int fd, struct dirent* entry, unsigned int count );

#endif // _UNISTD_H_
