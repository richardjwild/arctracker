#include <stdlib.h>
#include "arctracker.h"
#include "heap.h"

#define PITCH_QUANTA 2047
#define PHASE_INCREMENT_CONVERSION (60000L * 3575872L)

long* phase_increments;
unsigned char* resample_buffer;

long phase_increment(int period, long sample_rate)
{
    return PHASE_INCREMENT_CONVERSION/(period * sample_rate);
}

void calculate_phase_increments(long sample_rate)
{
    phase_increments = (long*) allocate_array(PITCH_QUANTA, sizeof(long));

    for (int period=1; period<=PITCH_QUANTA; period++)
        phase_increments[period - 1] = phase_increment(period, sample_rate);
}

void allocate_resample_buffer()
{
    resample_buffer = (unsigned char*) allocate_array(BUF_SIZE, sizeof(unsigned char));
}

void increment_phase_accumulator(channel_info *voice)
{
    voice->phase_acc_fraction += phase_increments[voice->period];
    voice->phase_accumulator += voice->phase_acc_fraction >> 16;
    voice->phase_acc_fraction -= (voice->phase_acc_fraction >> 16) << 16;
}

bool end_of_sample(const channel_info *voice)
{
    return voice->phase_accumulator > voice->sample_length;
}

void loop_sample(channel_info *voice)
{
    voice->phase_accumulator -= voice->repeat_length;
}

unsigned char* resample(channel_info* voice, long frames_to_write)
{
    unsigned char* sample = voice->sample_pointer;
    long frames_written;

    for (frames_written = 0; voice->channel_playing && frames_written < frames_to_write; frames_written++)
    {
        resample_buffer[frames_written] = sample[voice->phase_accumulator];
        increment_phase_accumulator(voice);
        if (end_of_sample(voice))
        {
            if (voice->sample_repeats)
                loop_sample(voice);
            else
                voice->channel_playing = false;
        }
    }

    for (; frames_written < frames_to_write; frames_written++)
    {
        resample_buffer[frames_written] = (unsigned char) 0;
    }

    return resample_buffer;
}
