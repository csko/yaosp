#include <pthread.h>
#include <errno.h>

#include <yaosp/debug.h>
#include <yaosp/syscall.h>
#include <yaosp/syscall_table.h>

int pthread_mutex_init( pthread_mutex_t* mutex, pthread_mutexattr_t* attr ) {
    mutex->mutex_id = syscall2( SYS_mutex_create, ( int )"pthread mutex", 0 );

    if ( mutex->mutex_id < 0 ) {
        errno = mutex->mutex_id;
        return -1;
    }

    mutex->init_magic = PTHREAD_MUTEX_MAGIC;

    return 0;
}

int pthread_mutex_destroy( pthread_mutex_t* mutex ) {
    dbprintf( "%s(): TODO\n", __FUNCTION__ );

    return 0;
}

int pthread_mutex_lock( pthread_mutex_t* mutex ) {
    int error;

    if ( mutex->init_magic != PTHREAD_MUTEX_MAGIC ) {
        error = pthread_mutex_init( mutex, NULL );

        if ( error < 0 ) {
            return -1;
        }
    }

    error = syscall1( SYS_mutex_lock, mutex->mutex_id );

    if ( error < 0 ) {
        errno = error;
        return -1;
    }

    return 0;
}

int pthread_mutex_trylock( pthread_mutex_t* mutex ) {
    int error;

    if ( mutex->init_magic != PTHREAD_MUTEX_MAGIC ) {
        error = pthread_mutex_init( mutex, NULL );

        if ( error < 0 ) {
            return -1;
        }
    }

    error = syscall1( SYS_mutex_trylock, mutex->mutex_id );

    if ( error < 0 ) {
        errno = error;
        return -1;
    }

    return 0;
}

int pthread_mutex_unlock( pthread_mutex_t* mutex ) {
    int error;

    if ( mutex->init_magic != PTHREAD_MUTEX_MAGIC ) {
        error = pthread_mutex_init( mutex, NULL );

        if ( error < 0 ) {
            return -1;
        }
    }

    error = syscall1( SYS_mutex_unlock, mutex->mutex_id );

    if ( error < 0 ) {
        errno = error;
        return -1;
    }

    return 0;
}

