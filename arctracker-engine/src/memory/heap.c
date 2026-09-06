#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "heap.h"
#include "io/error.h"
#include "messages.h"

static int resource_count[RESOURCE_GROUP_COUNT] = {0, 0, 0, 0};

void *allocate_array(resource_group_t resource_group, int no_elements, size_t element_size)
{
    size_t requested_bytes = (no_elements * element_size);
    uint8_t *array = malloc(requested_bytes);
    if (array == NULL)
    {
        error(MEMORY_ALLOCATION_FAILED);
        return NULL;
    }
    memset(array, 0, requested_bytes);
    if (resource_group < RESOURCE_GROUP_COUNT)
        resource_count[resource_group]++;
    return array;
}

void *reallocate_array(resource_group_t resource_group, void *array_before, int no_elements, size_t element_size)
{
    size_t requested_bytes = (no_elements * element_size);
    uint8_t *array_after = realloc(array_before, requested_bytes);
    if (array_after == NULL)
    {
        error(MEMORY_ALLOCATION_FAILED);
        return NULL;
    }
    if (array_before == NULL && resource_group < RESOURCE_GROUP_COUNT)
        resource_count[resource_group]++;
    return array_after;
}

void deallocate(resource_group_t resource_group, void *ptr)
{
    if (ptr != NULL && resource_group < RESOURCE_GROUP_COUNT)
        resource_count[resource_group]--;
    free(ptr);
}

int resource_count_for(resource_group_t resource_group)
{
    return resource_count[resource_group];
}
