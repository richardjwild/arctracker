#ifndef ARCTRACKER_ENGINE_MUTEXES_H
#define ARCTRACKER_ENGINE_MUTEXES_H

#include <stdbool.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <pthread.h>
#endif

typedef struct {
#ifdef _WIN32
    CRITICAL_SECTION native;
#else
    pthread_mutex_t native;
#endif
} arc_mutex_t;

bool arc_mutex_init(arc_mutex_t *mutex);
void arc_mutex_lock(arc_mutex_t *mutex);
void arc_mutex_unlock(arc_mutex_t *mutex);
void arc_mutex_destroy(arc_mutex_t *mutex);

#endif //ARCTRACKER_ENGINE_MUTEXES_H
