#ifndef _PTHREAD_H_
#define _PTHREAD_H_

#include <inttypes.h>

#define PTHREAD_MUTEX_MAGIC 0xC001C0DE

#define PTHREAD_MUTEX_INITIALIZER { 0, -1 }

typedef struct pthread_mutexattr {
    int flags;
} pthread_mutexattr_t;

typedef struct pthread_mutex {
    uint32_t init_magic;
    int mutex_id;
} pthread_mutex_t;

int pthread_mutex_init( pthread_mutex_t* mutex, pthread_mutexattr_t* attr );
int pthread_mutex_destroy( pthread_mutex_t* mutex );
int pthread_mutex_lock( pthread_mutex_t* mutex );
int pthread_mutex_trylock( pthread_mutex_t* mutex );
int pthread_mutex_unlock( pthread_mutex_t* mutex );

#endif /* _PTHREAD_H_ */
