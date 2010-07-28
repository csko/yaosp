/* dlopen function
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

#include <dlfcn.h>
#include <yaosp/syscall.h>
#include <yaosp/syscall_table.h>

#define MAX_GLOBAL_CTORS 64

typedef void global_ctor_t(void);

static int do_init_library(void* p) {
    int i;
    int ret;
    global_ctor_t* ctors[MAX_GLOBAL_CTORS];

    ret = syscall3(SYS_dlgetglobalinit, (int)p, (int)ctors, MAX_GLOBAL_CTORS);

    if (ret < 0) {
        return ret;
    }

    for (i = 0; i < ret; i++) {
        global_ctor_t* ctor = ctors[i];
        ctor();
    }

    return 0;
}

void* dlopen( const char* filename, int flag ) {
    void* p;

    p = (void*)syscall2(SYS_dlopen, (int)filename, flag);

    if (p == NULL) {
        return NULL;
    }

    do_init_library(p);

    return p;
}
