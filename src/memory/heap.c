#include "heap.h"
#include <stdlib.h>
#include <io/error.h>

#define NUM_SLOTS 256

static const char *MALLOC_FAILED = "Could not allocate memory";
static const char *ALL_SLOTS_USED = "The programmer needs to increase NUM_SLOTS.";

static void *slots[NUM_SLOTS];
static int slots_used = 0;

static void store(void *);

void *allocate_array(int no_elements, size_t element_size)
{
    size_t array_bytes = no_elements * element_size;
    void *array = malloc(array_bytes);
    if (array == NULL)
        error(MALLOC_FAILED);
    else
    {
        store(array);
        return array;
    }
}

static void store(void *ptr)
{
    if (slots_used == NUM_SLOTS)
        error(ALL_SLOTS_USED);
    else
    {
        slots[slots_used] = ptr;
        slots_used++;
    }
}

void deallocate_all()
{
    for (int slot = 0; slot < slots_used; slot++)
    {
        void *ptr = slots[slot];
        free(ptr);
    }
}