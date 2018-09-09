#include "heap.h"
#include <stdlib.h>
#include <io/error.h>

static const char *MALLOC_FAILED = "Could not allocate memory";
static const int BATCH_SIZE = 256;

static void **slots = NULL;
static int slots_used = 0;
static int slots_available = 0;

static void store(void *);

static void extend_slots();

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
    if (slots_used == slots_available)
    {
        extend_slots();
    }
    slots[slots_used] = ptr;
    slots_used++;
}

void extend_slots()
{
    int slots_required = slots_available + BATCH_SIZE;
    slots = realloc(slots, slots_required * sizeof(void *));
    if (slots == NULL)
        error(MALLOC_FAILED);
    else
        slots_available += BATCH_SIZE;
}

void deallocate_all()
{
    for (int slot = 0; slot < slots_used; slot++)
    {
        void *ptr = slots[slot];
        free(ptr);
    }
    free(slots);
    slots_used = 0;
    slots_available = 0;
}