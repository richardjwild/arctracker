#include "panner.h"

#include "../../../../../../../Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/System/Library/Frameworks/AudioToolbox.framework/Headers/AudioUnitProperties.h"

static const float PAN_HARD_LEFT = 1.0f;
static const float PAN_HARD_RIGHT = 255.0f;
static const uint8_t PAN_CENTRE = 0x80;

static void calculate_gain(uint8_t panning, float *left_gain, float *right_gain);
static void spatialise(audio_spatialiser_state_t *state, const float *mono, stereo_frame_t *stereo, int num_frames);
static void set_amount(audio_spatialiser_state_t *state, uint8_t amount);
static uint8_t get_amount(audio_spatialiser_state_t *state);

audio_spatialiser_t init_panner(panner_state_t *panner_state, const uint8_t initial_amount)
{
    audio_spatialiser_state_t spatialiser_state = {
        .panner = panner_state,
    };
    set_amount(&spatialiser_state, initial_amount);
    return (audio_spatialiser_t) {
        .state = spatialiser_state,
        .spatialise = spatialise,
        .set_amount = set_amount,
        .get_amount = get_amount,
    };
}

static void calculate_gain(const uint8_t panning, float *left_gain, float *right_gain)
{
    *left_gain = (PAN_HARD_RIGHT - (float) panning) / 254.0f;
    *right_gain = ((float) panning - PAN_HARD_LEFT) / 254.0f;
}

static void spatialise(audio_spatialiser_state_t *state, const float *mono, stereo_frame_t *stereo, const int num_frames)
{
    const float left_gain = state->panner->left_gain;
    const float right_gain = state->panner->right_gain;
    for (int frame = 0; frame < num_frames; frame++)
    {
        stereo[frame].l = mono[frame] * left_gain;
        stereo[frame].r = mono[frame] * right_gain;
    }
}

static void set_amount(audio_spatialiser_state_t *state, const uint8_t amount)
{
    panner_state_t *panner = state->panner;
    panner->panning = amount == 0 ? PAN_CENTRE : amount;
    calculate_gain(panner->panning, &panner->left_gain, &panner->right_gain);
}

static uint8_t get_amount(audio_spatialiser_state_t *state)
{
    const panner_state_t *panner = state->panner;
    return panner->panning;
}
