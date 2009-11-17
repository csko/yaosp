/* Fork implementation
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

#include <process.h>
#include <smp.h>
#include <errno.h>
#include <thread.h>
#include <macros.h>
#include <console.h>
#include <mm/context.h>
#include <sched/scheduler.h>
#include <lib/string.h>

#include <arch/fork.h>

int sys_fork( void ) {
    int error;
    thread_t* new_thread;
    process_t* new_process;
    thread_t* this_thread;
    process_t* this_process;

    this_thread = current_thread();
    this_process = this_thread->process;

    /* Create a new process */

    new_process = allocate_process( this_process->name );

    if ( new_process == NULL ) {
        error = -ENOMEM;
        goto error1;
    }

    /* Clone the memory context of the old process */

    new_process->memory_context = memory_context_clone( this_process->memory_context, new_process );

    if ( new_process->memory_context == NULL ) {
        error = -ENOMEM;
        goto error2;
    }

    if ( this_process->heap_region != NULL ) {
        new_process->heap_region = memory_context_get_region_for(
            new_process->memory_context,
            this_process->heap_region->address
        );

        memory_region_put( new_process->heap_region );
    }

    /* Clone the I/O context */

    new_process->io_context = io_context_clone( this_process->io_context );

    if ( new_process->io_context == NULL ) {
        error = -ENOMEM;
        goto error2;
    }

    /* Clone the locking context */

    new_process->lock_context = lock_context_clone( this_process->lock_context );

    if ( new_process->lock_context == NULL ) {
        error = -ENOMEM;
        goto error2;
    }

    /* Create the main thread of the new process */

    new_thread = allocate_thread(
        this_thread->name,
        new_process,
        this_thread->priority,
        this_thread->kernel_stack_pages
    );

    if ( new_thread == NULL ) {
        error = -ENOMEM;
        goto error2;
    }

    /* Set the parent ID of the new thread */

    new_thread->parent_id = this_thread->id;

    /* If the process has an userspace stack region then we have to find
       out the region ID of the cloned stack region. */

    if ( this_thread->user_stack_region != NULL ) {
        new_thread->user_stack_region = memory_context_get_region_for(
            new_process->memory_context,
            this_thread->user_stack_region->address
        );

        memory_region_put( new_thread->user_stack_region );
    }

    error = arch_do_fork( this_thread, new_thread );

    if ( error < 0 ) {
        goto error3;
    }

    /* Copy signal related informations to the new thread */

    memcpy(
        &new_thread->signal_handlers[ 0 ],
        &this_thread->signal_handlers[ 0 ],
        ( _NSIG - 1 ) * sizeof( struct sigaction )
    );

    /* Insert the new process and thread */

    scheduler_lock();

    error = insert_process( new_process );

    if ( error >= 0 ) {
        error = insert_thread( new_thread );

        if ( error >= 0 ) {
            lock_context_update( new_process->lock_context, new_thread->id );

            add_thread_to_ready( new_thread );

            error = new_thread->id;
        }
    }

    scheduler_unlock();

    if ( error < 0 ) {
        goto error3;
    }

    return error;

error3:
    /* NOTE: We don't have to destroy the process because
             destroy_thread() will do it. */

    destroy_thread( new_thread );

    return error;

error2:
    destroy_process( new_process );

error1:
    return error;
}
