#ifndef ARCTRACKER_HEAP_H
#define ARCTRACKER_HEAP_H

#include <stdlib.h>

#define RESOURCE_GROUP_COUNT 4

typedef enum {
    MAIN = 0, MODULE = 1, PLAYER = 2, AUDIO = 3
} resource_group_t;

void *allocate_array(resource_group_t, int no_elements, size_t element_size);

void *reallocate_array(resource_group_t resource_group, void *array_before, int no_elements, size_t element_size);

void deallocate(resource_group_t, void *);

int resource_count_for(resource_group_t);

#endif // ARCTRACKER_HEAP_H
