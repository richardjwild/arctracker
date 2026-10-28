#ifndef ARCTRACKER_ENGINE_THREADS_H
#define ARCTRACKER_ENGINE_THREADS_H

#include <stdbool.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <pthread.h>
#endif

typedef struct {
#ifdef _WIN32
    HANDLE native;
#else
    pthread_t native;
#endif
} arc_thread_t;

typedef void (*arc_thread_fn)(void *);

bool arc_thread_create(arc_thread_t *thread, arc_thread_fn start_routine, void *arg);
void arc_thread_join(arc_thread_t *thread);

#endif //ARCTRACKER_ENGINE_THREADS_H
