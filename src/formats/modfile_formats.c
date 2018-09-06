#include "modfile_formats.h"
#include "tracker_module.h"
#include "desktop_tracker_module.h"

#define NUM_FORMATS 2

format_t *formats()
{
    static format_t formats[NUM_FORMATS];
    formats[0] = tracker_format();
    formats[1] = desktop_tracker_format();
    return formats;
}

int num_formats()
{
    return NUM_FORMATS;
}
