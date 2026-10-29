#include "threads.h"
#include "io/error.h"
#include "memory/heap.h"
#include <string.h>

#ifdef _WIN32
#include <process.h>
#endif

typedef struct {
    arc_thread_fn start_routine;
    void *arg;
} thread_context_t;

static thread_context_t *make_context(arc_thread_fn start_routine, void *arg) {
    thread_context_t *context = allocate_array(MAIN, 1, sizeof(*context));
    if (context != NULL) {
        context->start_routine = start_routine;
        context->arg = arg;
    }
    return context;
}

#ifdef _WIN32

static unsigned __stdcall thread_entry(void *arg) {
    thread_context_t *context = arg;

    arc_thread_fn start_routine = context->start_routine;
    void *user_arg = context->arg;
    deallocate(MAIN, context);

    start_routine(user_arg);
    return 0;
}

bool arc_thread_create(arc_thread_t *thread, arc_thread_fn start_routine, void *arg) {
    thread_context_t *context = make_context(start_routine, arg);
    if (context == NULL) {
        error("Unable to allocate thread context");
        return false;
    }

    uintptr_t handle = _beginthreadex(NULL, 0, thread_entry, context, 0, NULL);

    if (handle == 0) {
        deallocate(MAIN, context);
        error("Unable to create thread");
        return false;
    }

    thread->native = (HANDLE)handle;
    return true;
}

void arc_thread_join(arc_thread_t *thread) {
    WaitForSingleObject(thread->native, INFINITE);
    CloseHandle(thread->native);
    thread->native = NULL;
}

#else

static void *thread_entry(void *arg) {
    thread_context_t *context = arg;

    arc_thread_fn start_routine = context->start_routine;
    void *user_arg = context->arg;
    deallocate(MAIN, context);

    start_routine(user_arg);
    return NULL;
}

bool arc_thread_create(arc_thread_t *thread, arc_thread_fn start_routine, void *arg) {
    thread_context_t *context = make_context(start_routine, arg);
    if (context == NULL) {
        error("Unable to allocate thread context");
        return false;
    }

    const int err = pthread_create(&thread->native, NULL, thread_entry, context);

    if (err != 0) {
        deallocate(MAIN, context);
        error(strerror(err));
        return false;
    }

    return true;
}

void arc_thread_join(arc_thread_t *thread) {
    pthread_join(thread->native, NULL);
}

#endif