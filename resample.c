#include <stdlib.h>
#include "arctracker.h"
#include "heap.h"

#define PITCH_QUANTA 2047
#define PHASE_INCREMENT_CONVERSION 60000L * 3575872L

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

unsigned char* resample(channel_info* voice, long frames_to_write, long* frames_written)
{
    unsigned char* sptr;

    for (*frames_written = 0; voice->channel_playing && *frames_written < frames_to_write; (*frames_written)++)
    {
        sptr = voice->sample_pointer + voice->phase_accumulator;
        resample_buffer[*frames_written] = *sptr;

        /* increment phase accumulator */
        voice->phase_acc_fraction += voice->phase_increment;
        voice->phase_accumulator += voice->phase_acc_fraction >> 16;
        voice->phase_acc_fraction -= (voice->phase_acc_fraction >> 16) << 16;

        /* end of sample? */
        if (voice->phase_accumulator > voice->sample_length)
        {
            /* if sample repeats then set accumulator back to repeat offset */
            if (voice->sample_repeats == YES)
                voice->phase_accumulator -= voice->repeat_length;
            else
                voice->channel_playing = false;
        }

    }
    return resample_buffer;
}

long phase_increment_for(int period)
{
    return phase_increments[period];
}
