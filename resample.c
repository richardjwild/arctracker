#include <stdlib.h>
#include <stdbool.h>
#include "heap.h"
#include "resample.h"

void calculate_phase_increments(const long sample_rate)
{
    phase_increments = (double *) allocate_array(PITCH_QUANTA, sizeof(double));
    for (int period = 1; period <= PITCH_QUANTA; period++)
        phase_increments[period - 1] = PHASE_INCREMENT_CONVERSION / (period * sample_rate);
}

void allocate_resample_buffer(const int no_of_frames)
{
    resample_buffer = (unsigned char *) allocate_array(no_of_frames, sizeof(unsigned char));
    resample_buffer_bytes = no_of_frames * sizeof(unsigned char);
}

static inline
bool end_of_sample(const voice_t *voice)
{
    return voice->phase_accumulator > voice->sample_end;
}

static inline
void loop_sample(voice_t *voice)
{
    voice->phase_accumulator -= voice->repeat_length;
}

void write_audio_to_resample_buffer(voice_t *voice, const long frames_to_write)
{
    const unsigned char *sample = voice->sample_pointer;
    const double phase_increment = phase_increments[voice->period];
    for (long frame = 0; voice->channel_playing && frame < frames_to_write; frame++)
    {
        resample_buffer[frame] = sample[(long) voice->phase_accumulator];
        voice->phase_accumulator += phase_increment;
        if (end_of_sample(voice))
        {
            if (voice->sample_repeats)
                loop_sample(voice);
            else
                voice->channel_playing = false;
        }
    }
}

static inline
void clear_resample_buffer()
{
    memset(resample_buffer, 0, resample_buffer_bytes);
}

unsigned char *resample(voice_t *voice, const long frames_to_write)
{
    clear_resample_buffer();
    if (voice->channel_playing)
        write_audio_to_resample_buffer(voice, frames_to_write);
    return resample_buffer;
}
