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

#include <types.h>
#include <signal.h>
#include <thread.h>
#include <scheduler.h>
#include <console.h>
#include <errno.h>
#include <smp.h>

int is_signal_pending( thread_t* thread ) {
    return ( ( thread->pending_signals & ~thread->blocked_signals ) != 0 );
}

int handle_signals( thread_t* thread ) {
    int i;
    int signal;
    signal_handler_t* handler;

    for ( i = 0; i < _NSIG - 1; i++ ) {
        signal = i + 1;

        /* Check if this signal is pending */

        if ( ( thread->pending_signals & ( 1ULL << i ) ) == 0 ) {
            continue;
        }

        /* Remove from pending signals */

        thread->pending_signals &= ~( 1ULL << i );

        handler = &thread->signal_handlers[ i ];

        kprintf( "Handling signal %d in thread %s with %p handler\n", signal, thread->name, handler->handler );

        if ( handler->handler == SIG_IGN ) {
            continue;
        } else if ( handler->handler == SIG_DFL ) {
            switch ( signal ) {
                case SIGCHLD :
                case SIGWINCH :
                    break;

                case SIGSTOP :
                    /* TODO: Stop the thread */
                    break;

                case SIGCONT :
                    /* TODO: Continue the process if stopped */
                    break;

                default :
                    thread_exit( signal );
                    break;
            }
        } else {
            arch_handle_userspace_signal( thread, signal, handler );

            /* Reset the signal handler to SIG_DFL in case of SA_ONESHOT is set in flags */

            if ( ( handler->flags & SA_ONESHOT ) != 0 ) {
                handler->handler = SIG_DFL;
            }
        }
    }

    return 0;
}

int send_signal( thread_t* thread, int signal ) {
    if ( signal == 0 ) {
        return 0;
    }

    signal--;

    /* Add the signal to the pending bitmap */

    thread->pending_signals |= ( 1ULL << signal );

    if ( is_signal_pending( thread ) ) {
        spinlock_disable( &scheduler_lock );

        do_wake_up_thread( thread );

        spinunlock_enable( &scheduler_lock );
    }

    return 0;
}

int sys_sigaction( int signal, struct sigaction* act, struct sigaction* oldact ) {
    thread_t* thread;
    signal_handler_t* handler;

    if ( ( signal < 1 ) || ( signal >= _NSIG ) ) {
        return -EINVAL;
    }

    thread = current_thread();

    handler = &thread->signal_handlers[ signal - 1 ];

    handler->handler = ( sighandler_t )act->sa_handler;

    return 0;
}

int sys_kill_thread( thread_id tid, int signal ) {
    thread_t* thread;

    spinlock_disable( &scheduler_lock );

    thread = get_thread_by_id( tid );

    spinunlock_enable( &scheduler_lock );

    if ( thread == NULL ) {
        return -EINVAL;
    }

    return send_signal( thread, signal );
}
