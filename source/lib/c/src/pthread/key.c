/* pthread thread specific functions
 *
 * Copyright (c) 2010 Zoltan Kovacs
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

#include <pthread.h>

#include <yaosp/debug.h>
#include <yaosp/syscall.h>
#include <yaosp/syscall_table.h>

int pthread_key_create(pthread_key_t* key, void (*destructor)(void*)) {
    int tld;

    /* Allocate a new TLD entry for this key. */
    tld = syscall0(SYS_alloc_tld);

    if (tld < 0) {
        return -1;
    }

    *key = tld * 4;
    // todo: handle destructor

    /* Set the default NULL for the key. */
    pthread_setspecific(*key, NULL);

    return 0;
}

int pthread_key_delete(pthread_key_t key) {
    dbprintf("pthread_key_delete() called.\n");
    return 0;
}

void* pthread_getspecific(pthread_key_t key) {
    register int value;
    __asm__ __volatile__(
        "movl %%gs:(%1), %0"
        : "=r" (value)
        : "r" (key)
    );
    return (void*)value;
}

int pthread_setspecific(pthread_key_t key, const void* value) {
    __asm__ __volatile__(
        "movl %0, %%gs:(%1)"
        :
        : "r" ((int)value), "r" (key)
    );
    return 0;
}
