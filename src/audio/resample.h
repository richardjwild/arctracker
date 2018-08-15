#ifndef ARCTRACKER_RESAMPLE_H
#define ARCTRACKER_RESAMPLE_H

#include <playroutine/play_mod.h>

void calculate_phase_increments(long p_sample_rate);

void allocate_resample_buffer(int no_of_frames);

float *resample(voice_t *voice, long frames_to_write);

#endif // ARCTRACKER_RESAMPLE_H