#include "mutexes.h"

#ifdef _WIN32

bool arc_mutex_init(arc_mutex_t *mutex) {
    InitializeCriticalSection(&mutex->native);
    return true;
}

void arc_mutex_lock(arc_mutex_t *mutex) {
    EnterCriticalSection(&mutex->native);
}

void arc_mutex_unlock(arc_mutex_t *mutex) {
    LeaveCriticalSection(&mutex->native);
}

void arc_mutex_destroy(arc_mutex_t *mutex) {
    DeleteCriticalSection(&mutex->native);
}

#else

bool arc_mutex_init(arc_mutex_t *mutex) {
    return pthread_mutex_init(&mutex->native, NULL) == 0;
}

void arc_mutex_lock(arc_mutex_t *mutex) {
    pthread_mutex_lock(&mutex->native);
}

void arc_mutex_unlock(arc_mutex_t *mutex) {
    pthread_mutex_unlock(&mutex->native);
}

void arc_mutex_destroy(arc_mutex_t *mutex) {
    pthread_mutex_destroy(&mutex->native);
}

#endif