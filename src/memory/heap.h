#ifndef ARCTRACKER_HEAP_H
#define ARCTRACKER_HEAP_H

#include <stdlib.h>

void *allocate_array(int no_elements, size_t element_size);

void deallocate_all();

#endif // ARCTRACKER_HEAP_H