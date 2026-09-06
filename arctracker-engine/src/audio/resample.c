#include "resample.h"
#include <string.h>
#include "memory/heap.h"
#include <stdio.h>

static float interpolate_linear(const float *, float);
static float interpolate_none(const float *, float);

float *allocate_resample_buffer(const int no_of_frames)
{
    return allocate_array(AUDIO, no_of_frames, sizeof(float));
}

void resample(voice_t *voice, float *resample_buffer, int frames_to_write, const interpolation_type_t interpolation_type)
{
    sampler_state_t *sampler_state = &voice->sampler_state;
    if (!voice->channel_playing || sampler_state->period == 0)
    {
        // Fill the buffer with silence.
        memset(resample_buffer, 0, frames_to_write * sizeof(float));
        return;
    }
    float (*interpolate)(const float *, float) = interpolation_type == LINEAR ? interpolate_linear : interpolate_none;
    const float *sample = sampler_state->sample->sample_pointer;
    const int period = sampler_state->period + sampler_state->period_modulation;
    const float phase_increment = sampler_state->sample->phase_increment_per_period / (float) period;
    const float sample_end = (float) sampler_state->sample->sample_end;
    const float repeat_length = (float) sampler_state->sample->repeat_length;
    const bool sample_repeats = sampler_state->sample->sample_repeats;
    float phase_accumulator = sampler_state->phase_accumulator;
    int offset = 0;
    while (frames_to_write > 0)
    {
        resample_buffer[offset++] = interpolate(sample, phase_accumulator);
        frames_to_write--;
        phase_accumulator += phase_increment;
        if (phase_accumulator >= sample_end)
        {
            if (!sample_repeats || repeat_length <= 0) break;
            phase_accumulator -= repeat_length;
        }
    }
    if (frames_to_write > 0)
    {
        // The sample ended before we wrote all the requested frames.
        voice->channel_playing = false;
        // Fill the remainder of the buffer with silence.
        memset(resample_buffer + offset, 0, frames_to_write * sizeof(float));
    }
    sampler_state->phase_accumulator = phase_accumulator;
}

static float interpolate_linear(const float *sample, const float phase_accumulator)
{
    const int frame_from = (int) phase_accumulator;
    const float sample_from = sample[frame_from];
    const float distance = sample[frame_from + 1] - sample_from;
    const float fraction = phase_accumulator - (float) frame_from;
    return sample_from + distance * fraction;
}

static float interpolate_none(const float *sample, const float phase_accumulator)
{
    return sample[(int) phase_accumulator];
}
