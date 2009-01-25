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

#include <string.h>
#include <sys/types.h>

#define NAME_MAX 255

#define _D_EXACT_NAMLEN(d) (strlen((d)->d_name))
#define _D_ALLOC_NAMLEN(d) (NAME_MAX+1)

struct dirent {
    ino_t d_ino;
    char d_name[ NAME_MAX + 1 ];
};

void _exit( int status );

pid_t fork( void );

int execve( const char* filename, char* const argv[], char* const envp[] );
int execvp( const char* filename, char* const argv[] );

void* sbrk( int increment );

int close( int fd );
int dup2( int old_fd, int new_fd );

ssize_t read( int fd, void* buf, size_t count );
ssize_t write( int fd, const void* buf, size_t count );
off_t lseek( int fd, off_t offset, int whence );

int isatty( int fd );
int chdir( const char* path );
int fchdir( int fd );
int getdents( int fd, struct dirent* entry, unsigned int count );
int ftruncate( int fd, off_t length );

int link( const char* oldpath, const char* newpath );
int access( const char* pathname, int mode );
int unlink( const char* pathname );
ssize_t readlink( const char* path, char* buf, size_t bufsiz );
int rmdir( const char* pathname );
int chown( const char* path, uid_t owner, gid_t group );
char* getcwd( char* buf, size_t size );

pid_t getpid( void );
int getpagesize( void );

unsigned int sleep( unsigned int seconds );

#endif // _UNISTD_H_
