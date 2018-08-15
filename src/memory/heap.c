#include "heap.h"
#include <stdlib.h>
#include <io/error.h>

static const char *MALLOC_FAILED = "Could not allocate memory";

void *allocate_array(int no_elements, size_t element_size)
{
    size_t array_bytes = no_elements * element_size;
    void *array = malloc(array_bytes);
    if (array == NULL)
        error(MALLOC_FAILED);
    else
        return array;
}
