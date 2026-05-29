#include <string.h>
#include "resample.h"
#include "memory/heap.h"

const static int PITCH_QUANTA = 2047;
const static float PHASE_INCREMENT_CONVERSION = 3273808.59375f;

static void write_audio_to_resample_buffer(voice_t *voice, float *, float *, int);
static float interpolate(const float *sample, float);

float *calculate_phase_increments(const int sample_rate)
{
    float *phase_increments = allocate_array(AUDIO, PITCH_QUANTA, sizeof(float));
    for (int period = 1; period <= PITCH_QUANTA; period++)
        phase_increments[period - 1] = PHASE_INCREMENT_CONVERSION / (period * sample_rate);
    return phase_increments;
}

float *allocate_resample_buffer(const int no_of_frames)
{
    return allocate_array(AUDIO, no_of_frames, sizeof(float));
}

float *resample(voice_t *voice, float *resample_buffer, const size_t resample_buffer_bytes, float *phase_increments, const int frames_to_write)
{
    memset(resample_buffer, 0, resample_buffer_bytes);
    if (voice->channel_playing)
        write_audio_to_resample_buffer(voice, resample_buffer, phase_increments, frames_to_write);
    return resample_buffer;
}

static void write_audio_to_resample_buffer(voice_t *voice, float *resample_buffer, float *phase_increments, const int frames_to_write)
{
    const float *sample = voice->sample_pointer;
    const float phase_increment = phase_increments[voice->period];
    const float sample_end = voice->sample_end;
    const float repeat_length = voice->repeat_length;
    const bool sample_repeats = voice->sample_repeats;
    float phase_accumulator = voice->phase_accumulator;
    for (int frame = 0; frame < frames_to_write; frame++)
    {
        resample_buffer[frame] = interpolate(sample, phase_accumulator);
        phase_accumulator += phase_increment;
        if (phase_accumulator >= sample_end)
        {
            if (!sample_repeats)
            {
                voice->channel_playing = false;
                break;
            }
            phase_accumulator -= repeat_length;
        }
    }
    voice->phase_accumulator = phase_accumulator;
}

static float interpolate(const float *sample, const float phase_accumulator)
{
    const int frame_from = (int) phase_accumulator;
    const float sample_from = sample[frame_from];
    const float distance = sample[frame_from + 1] - sample_from;
    const float fraction = phase_accumulator - frame_from;
    return sample_from + (distance * fraction);
}
