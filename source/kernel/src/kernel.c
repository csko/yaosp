/* Miscellaneous kernel functions
 *
 * Copyright (c) 2008, 2009 Zoltan Kovacs
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

#include <console.h>
#include <process.h>
#include <thread.h>
#include <kernel.h>
#include <module.h>
#include <devices.h>
#include <loader.h>
#include <version.h>
#include <macros.h>
#include <errno.h>
#include <config.h>
#include <ipc.h>
#include <debug.h>
#include <symbols.h>
#include <sched/scheduler.h>
#include <lock/context.h>
#include <lib/stdarg.h>
#include <lib/string.h>

#include <arch/interrupt.h>
#include <arch/cpu.h>

static uint32_t kernel_param_count = 0;
static char* kernel_params[ MAX_KERNEL_PARAMS ];
static char kernel_param_buffer[ KERNEL_PARAM_BUF_SIZE ];

extern void arch_reboot( void );
extern void arch_shutdown( void );

__init int parse_kernel_parameters( const char* params ) {
    char* p;
    size_t length;

    p = strchr( params, ' ' );

    if ( p == NULL ) {
        return 0;
    }

    length = strlen( p );

    if ( length == 0 ) {
        return 0;
    }

    length = MIN( length, KERNEL_PARAM_BUF_SIZE - 1 );

    memcpy( kernel_param_buffer, p, length );
    kernel_param_buffer[ length ] = 0;

    p = kernel_param_buffer;

    do {
        kernel_params[ kernel_param_count++ ] = p;
        p = strchr( p, ' ' );

        if ( p != NULL ) {
            *p++ = 0;
        }
    } while ( ( p != NULL ) && ( kernel_param_count < MAX_KERNEL_PARAMS ) );

    return 0;
}

int get_kernel_param_as_string( const char* key, const char** value ) {
    char* s;
    uint32_t i;

    for ( i = 0; i < kernel_param_count; i++ ) {
        s = strchr( kernel_params[ i ], '=' );

        if ( s == NULL ) {
            continue;
        }

        if ( strncmp( kernel_params[ i ], key, s - kernel_params[ i ] ) == 0 ) {
            *value = ++s;
            return 0;
        }
    }

    return -ENOENT;
}

int get_kernel_param_as_bool( const char* key, bool* value ) {
    int error;
    const char* tmp;

    error = get_kernel_param_as_string( key, &tmp );

    if ( error < 0 ) {
        return error;
    }

    if ( strcmp( tmp, "true" ) == 0 ) {
        *value = true;
    } else if ( strcmp( tmp, "false" ) == 0 ) {
        *value = false;
    } else {
        return -EINVAL;
    }

    return 0;
}

int sys_get_kernel_info( kernel_info_t* kernel_info ) {
    kernel_info->major_version = KERNEL_MAJOR_VERSION;
    kernel_info->minor_version = KERNEL_MINOR_VERSION;
    kernel_info->release_version = KERNEL_RELEASE_VERSION;

    strncpy( kernel_info->build_date, build_date, 32 );
    strncpy( kernel_info->build_time, build_time, 32 );

    kernel_info->build_date[ 31 ] = 0;
    kernel_info->build_time[ 31 ] = 0;

    return 0;
}

int sys_get_kernel_statistics( statistics_info_t* statistics_info ) {
    statistics_info->semaphore_count = 0;//get_semaphore_count();

    return 0;
}

int sys_dbprintf( const char* format, char** parameters ) {
#ifndef MK_RELEASE_BUILD
    char buf[ 256 ];

    snprintf(
        buf,
        sizeof( buf ),
        format,
        parameters[ 0 ],
        parameters[ 1 ],
        parameters[ 2 ],
        parameters[ 3 ],
        parameters[ 4 ],
        parameters[ 5 ]
    );

    DEBUG_LOG( "%s", buf );
#endif /* !MK_RELEASE_BUILD */

    return 0;
}

void handle_panic( const char* file, int line, const char* format, ... ) {
    va_list args;
    thread_t* thread;

    kprintf( ERROR, "Panic at %s:%d: ", file, line );

    va_start( args, format );
    kvprintf( ERROR, format, args );
    va_end( args );

    thread = current_thread();

    if ( thread != NULL ) {
        kprintf( ERROR, "Process: %s thread: %s\n", thread->process->name, thread->name );
    }

    debug_print_stack_trace();
    memory_context_dump( &kernel_memory_context );

    halt_loop();
}

void reboot( void ) {
    arch_reboot();
}

int sys_reboot( void ) {
    arch_reboot();
    return 0;
}

void shutdown( void ) {
    arch_shutdown();
}

int sys_shutdown( void ) {
    arch_shutdown();
    return 0;
}

__init void kernel_main( void ) {
    int error;

    init_kernel_symbols();
    init_locking();
    init_regions();
    init_devices();
    init_module_loader();
    init_application_loader();
    init_interpreter_loader();
    init_processes();
    init_threads();
    init_scheduler();
    init_ipc();
    init_smp();

    error = arch_late_init();

    if ( error < 0 ) {
        return;
    }

    init_smp_late();
    create_init_thread();

    /* Enable interrupts. The first timer interrupt will
       start the scheduler */

    enable_interrupts();
    halt_loop();
}
