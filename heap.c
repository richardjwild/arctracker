#include <stdlib.h>
#include "error.h"

void* allocate_array(int no_elements, size_t element_size)
{
    long array_bytes = no_elements * element_size;
    void* array = malloc(array_bytes);
    if (array == NULL)
        error(MALLOC_FAILED);
    else
        return array;
}