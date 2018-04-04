#include <stdlib.h>
#include "arctracker.h"
#include "heap.h"

#define PITCH_QUANTA 2047
#define PHASE_INCREMENT_CONVERSION 3273808.59375

double* phase_increments;
unsigned char* resample_buffer;

void calculate_phase_increments(long sample_rate)
{
    phase_increments = (double*) allocate_array(PITCH_QUANTA, sizeof(double));

    for (int period=1; period<=PITCH_QUANTA; period++)
        phase_increments[period - 1] = PHASE_INCREMENT_CONVERSION/(period * sample_rate);
}

void allocate_resample_buffer()
{
    resample_buffer = (unsigned char*) allocate_array(BUF_SIZE, sizeof(unsigned char));
}

static inline void increment_phase_accumulator(channel_info *voice)
{
    voice->phase_accumulator += phase_increments[voice->period];
}

static inline bool end_of_sample(const channel_info *voice)
{
    return voice->phase_accumulator > voice->sample_length;
}

static inline void loop_sample(channel_info *voice)
{
    voice->phase_accumulator -= voice->repeat_length;
}

unsigned char* resample(channel_info* voice, long frames_to_write)
{
    memset(resample_buffer, 0, BUF_SIZE);
    unsigned char* sample = voice->sample_pointer;
    for (long frame = 0; voice->channel_playing && frame < frames_to_write; frame++)
    {
        resample_buffer[frame] = sample[(long) voice->phase_accumulator];
        increment_phase_accumulator(voice);
        if (end_of_sample(voice))
        {
            if (voice->sample_repeats)
                loop_sample(voice);
            else
                voice->channel_playing = false;
        }
    }
    return resample_buffer;
}
