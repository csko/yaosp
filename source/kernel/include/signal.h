/* Signal handling functions
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

#ifndef _SIGNAL_H_
#define _SIGNAL_H_

#define SIGHUP    1       /* Hangup (POSIX).  */
#define SIGINT    2       /* Interrupt (ANSI).  */
#define SIGQUIT   3       /* Quit (POSIX).  */
#define SIGILL    4       /* Illegal instruction (ANSI).  */
#define SIGTRAP   5       /* Trace trap (POSIX).  */
#define SIGABRT   6       /* Abort (ANSI).  */
#define SIGIOT    6       /* IOT trap (4.2 BSD).  */
#define SIGBUS    7       /* BUS error (4.2 BSD).  */
#define SIGFPE    8       /* Floating-point exception (ANSI).  */
#define SIGKILL   9       /* Kill, unblockable (POSIX).  */
#define SIGUSR1   10      /* User-defined signal 1 (POSIX).  */
#define SIGSEGV   11      /* Segmentation violation (ANSI).  */
#define SIGUSR2   12      /* User-defined signal 2 (POSIX).  */
#define SIGPIPE   13      /* Broken pipe (POSIX).  */
#define SIGALRM   14      /* Alarm clock (POSIX).  */
#define SIGTERM   15      /* Termination (ANSI).  */
#define SIGSTKFLT 16      /* Stack fault.  */
#define SIGCLD    SIGCHLD /* Same as SIGCHLD (System V).  */
#define SIGCHLD   17      /* Child status has changed (POSIX).  */
#define SIGCONT   18      /* Continue (POSIX).  */
#define SIGSTOP   19      /* Stop, unblockable (POSIX).  */
#define SIGTSTP   20      /* Keyboard stop (POSIX).  */
#define SIGTTIN   21      /* Background read from tty (POSIX).  */
#define SIGTTOU   22      /* Background write to tty (POSIX).  */
#define SIGURG    23      /* Urgent condition on socket (4.2 BSD).  */
#define SIGXCPU   24      /* CPU limit exceeded (4.2 BSD).  */
#define SIGXFSZ   25      /* File size limit exceeded (4.2 BSD).  */
#define SIGVTALRM 26      /* Virtual alarm clock (4.2 BSD).  */
#define SIGPROF   27      /* Profiling alarm clock (4.2 BSD).  */
#define SIGWINCH  28      /* Window size change (4.3 BSD, Sun).  */
#define SIGPOLL   SIGIO   /* Pollable event occurred (System V).  */
#define SIGIO     29      /* I/O now possible (4.2 BSD).  */
#define SIGPWR    30      /* Power failure restart (System V).  */
#define SIGSYS    31      /* Bad system call.  */

#define _NSIG     65      /* Biggest signal number + 1 (including real-time signals). */

typedef void ( *sighandler_t )( int );

#define SIG_ERR ((sighandler_t)-1) /* Error return.  */
#define SIG_DFL ((sighandler_t)0) /* Default action.  */
#define SIG_IGN ((sighandler_t)1) /* Ignore signal.  */

typedef struct signal_handler {
    sighandler_t handler;
} signal_handler_t;

struct thread;

int is_signal_pending( struct thread* thread );

int handle_signals( struct thread* thread );
int send_signal( struct thread* thread, int signal );

#endif /* _SIGNAL_H_ */
