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
#include <scheduler.h>
#include <sysinfo.h>
#include <macros.h>
#include <mm/context.h>

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

    new_process->memory_context = memory_context_clone( this_process->memory_context );

    if ( new_process->memory_context == NULL ) {
        error = -ENOMEM;
        goto error2;
    }

    if ( this_process->heap_region >= 0 ) {
        region_t* heap_region;
        region_info_t region_info;

        error = get_region_info( this_process->heap_region, &region_info );

        if ( error >= 0 ) {
            heap_region = memory_context_get_region_for(
                new_process->memory_context,
                region_info.start
            );

            ASSERT( heap_region != NULL );

            new_process->heap_region = heap_region->id;
        }
    }

    /* Clone the I/O context */

    new_process->io_context = io_context_clone( this_process->io_context );

    if ( new_process->io_context == NULL ) {
        error = -ENOMEM;
        goto error2;
    }

    /* Clone the semaphore context */

    new_process->semaphore_context = semaphore_context_clone( this_process->semaphore_context );

    if ( new_process->semaphore_context == NULL ) {
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

    /* If the process has an userspace stack region then we have to find
       out the region ID of the cloned stack region. */

    if ( this_thread->user_stack_region >= 0 ) {
        region_t* stack_region;
        region_info_t region_info;

        error = get_region_info( this_thread->user_stack_region, &region_info );

        if ( error >= 0 ) {
            stack_region = memory_context_get_region_for(
                new_process->memory_context,
                region_info.start
            );

            ASSERT( stack_region != NULL );

            new_thread->user_stack_region = stack_region->id;
        }
    }

    error = arch_do_fork( this_thread, new_thread );

    if ( error < 0 ) {
        goto error3;
    }

    /* Insert the new process and thread */

    spinlock_disable( &scheduler_lock );

    error = insert_process( new_process );

    if ( error >= 0 ) {
        error = insert_thread( new_thread );

        if ( error >= 0 ) {
            add_thread_to_ready( new_thread );

            error = new_process->id;
        }
    }

    spinunlock_enable( &scheduler_lock );

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
