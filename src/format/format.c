#include "format.h"
#include "format_tracker.h"
#include "format_desktop_tracker.h"
#include "format_soundtracker.h"

#define NUM_FORMATS 3

format_t *formats()
{
    static format_t formats[NUM_FORMATS];
    formats[0] = tracker_format();
    formats[1] = desktop_tracker_format();
    formats[2] = soundtracker_format();
    return formats;
}

int num_formats()
{
    return NUM_FORMATS;
}
