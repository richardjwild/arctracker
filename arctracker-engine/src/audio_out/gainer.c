#include "gainer.h"
#include <assert.h>
#include <math.h>
#include "player/module.h"

static const float *get_curve(gainer_state_t *state);
static void apply_volume_slide(gainer_state_t *state);
static void apply_tremolo(gainer_state_t *state);

void gainer_init_curves(gain_curves_t *gain_curves)
{
    const float volume_max = (float) INTERNAL_VOLUME_MAX;
    for (int vol = 0; vol <= INTERNAL_VOLUME_MAX; vol++)
    {
        gain_curves->linear[vol] = (float) vol / volume_max;
        const float exponent = (float) vol / volume_max;
        gain_curves->exponential[vol] = (powf(volume_max + 1.0f, exponent) - 1.0f) / volume_max;
    }
    gain_curves->initialised = true;
}

gainer_state_t gainer_init(const gain_curves_t *gain_curves, const volume_mapping_type_t volume_mapping)
{
    assert(gain_curves->initialised);
    const gainer_state_t state = {
        .volume = INTERNAL_VOLUME_MAX,
        .modulation = 0,
        .slide_rate = 0,
        .slide_fine = false,
        .volume_mapping = volume_mapping,
        .gain_curves = gain_curves,
        .tremolo = (lfo_effect_t) {
            .enabled = false,
        }
    };
    return state;
}

void gainer_apply(gainer_state_t *state, float *channel_buffer, const unsigned int frames_requested)
{
    int volume = state->volume + state->modulation;
    if (volume < 0) volume = 0;
    if (volume > INTERNAL_VOLUME_MAX) volume = INTERNAL_VOLUME_MAX;
    const float *curve = get_curve(state);
    const float gain = curve[volume];
    assert(gain >= 0.0);
    assert(gain <= 1.0);
    for (unsigned int frame = 0; frame < frames_requested; frame++)
    {
        channel_buffer[frame] *= gain;
    }
}

void gainer_tick(gainer_state_t *state, const int ticks, const int ticks_per_event, const unsigned long frames_until_next_tick)
{
    (void) ticks_per_event;
    (void) frames_until_next_tick;
    if (ticks == 0) return;
    if (state->slide_rate != 0 && !state->slide_fine)
        apply_volume_slide(state);
    if (state->tremolo.enabled)
        apply_tremolo(state);
}

void gainer_set_volume(gainer_state_t *state, const int volume)
{
    state->volume = volume;
}

void gainer_volume_slide_on(gainer_state_t *state, const int slide_rate, const bool fine)
{
    state->slide_rate = slide_rate;
    if (fine)
    {
        int new_volume = state->volume + state->slide_rate;
        if (new_volume < 0) new_volume = 0;
        if (new_volume > INTERNAL_VOLUME_MAX) new_volume = INTERNAL_VOLUME_MAX;
        state->volume = new_volume;
        state->slide_fine = true;
    }
    else
    {
        state->slide_fine = false;
    }
}

void gainer_volume_slide_off(gainer_state_t *state)
{
    state->slide_rate = 0;
    state->slide_fine = false;
}

void gainer_tremolo_on(gainer_state_t *state, const int rate, const int depth, const pt_waveform_t waveform, const bool retrigger)
{
    state->tremolo.rate = rate;
    state->tremolo.depth = depth;
    state->tremolo.waveform = waveform;
    if (retrigger && !state->tremolo.enabled)
    {
        state->tremolo.phase = 0;
    }
    state->tremolo.enabled = true;
}

void gainer_tremolo_off(gainer_state_t *state)
{
    state->tremolo.enabled = false;
    state->modulation = 0;
}

void gainer_set_volume_mapping(gainer_state_t *state, const volume_mapping_type_t volume_mapping_type)
{
    state->volume_mapping = volume_mapping_type;
}

static const float *get_curve(gainer_state_t *state)
{
    return state->volume_mapping == VOLUME_ARCHIMEDES
        ? state->gain_curves->exponential
        : state->gain_curves->linear;
}

static void apply_volume_slide(gainer_state_t *state)
{
    int new_volume = state->volume + state->slide_rate;
    if (new_volume < 0) new_volume = 0;
    if (new_volume > INTERNAL_VOLUME_MAX) new_volume = INTERNAL_VOLUME_MAX;
    state->volume = new_volume;
}

static void apply_tremolo(gainer_state_t *state)
{
    const uint8_t rate = state->tremolo.rate;
    const uint8_t depth = state->tremolo.depth;
    const float lfo_value = lfo_pt_waveform(state->tremolo.waveform, state->tremolo.phase);
    const float modulation = lfo_value * (float) depth * 16.0f;
    state->modulation = (int) modulation;
    state->tremolo.phase = (state->tremolo.phase + rate) % PT_LFO_WAVELENGTH;
}
