#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "heap.h"
#include "io/error.h"
#include "messages.h"

static int resource_count[RESOURCE_GROUP_COUNT] = {0, 0, 0, 0};
static bool size_multiply(size_t, size_t, size_t *);

void *allocate_array(const resource_group_t resource_group, const unsigned int no_elements, const size_t element_size)
{
    size_t requested_bytes;
    if (!size_multiply(no_elements, element_size, &requested_bytes))
    {
        error(ARITHMETIC_OVERFLOW);
        return NULL;
    }
    if (requested_bytes == 0)
    {
        error(ZERO_MEMORY_REQUESTED);
        return NULL;
    }
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

void *reallocate_array(const resource_group_t resource_group, void *array_before, const unsigned int no_elements, const size_t element_size)
{
    size_t requested_bytes;
    if (!size_multiply(no_elements, element_size, &requested_bytes))
    {
        error(ARITHMETIC_OVERFLOW);
        return NULL;
    }
    if (requested_bytes == 0)
    {
        error(ZERO_MEMORY_REQUESTED);
        return NULL;
    }
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

void deallocate(const resource_group_t resource_group, void *ptr)
{
    if (ptr != NULL && resource_group < RESOURCE_GROUP_COUNT)
        resource_count[resource_group]--;
    free(ptr);
}

int resource_count_for(const resource_group_t resource_group)
{
    return resource_count[resource_group];
}

static bool size_multiply(const size_t multiplicand, const size_t multiplier, size_t *result)
{
    if (multiplicand != 0 && multiplier > SIZE_MAX / multiplicand)
    {
        return false;
    }
    *result = multiplicand * multiplier;
    return true;
}
