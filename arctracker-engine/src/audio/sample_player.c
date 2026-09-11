#include "sample_player.h"
#include <string.h>
#include "memory/heap.h"
#include <stdio.h>
#include "player/module.h"
#include "player/period.h"

static bool generate_audio(audio_generator_state_t *, float *, int);
static float interpolate_linear(const float *, float);
static float interpolate_none(const float *, float);
static void set_volume(audio_generator_state_t *state, uint8_t);
static void volume_slide_on(audio_generator_state_t *state, int, bool);
static void volume_slide_off(audio_generator_state_t *state);
static void pitch_slide_on(audio_generator_state_t *state, int, bool);
static void pitch_slide_off(audio_generator_state_t *state);
static void set_tone_portamento_target(audio_generator_state_t *, int);
static void tone_portamento_on(audio_generator_state_t *, int);
static void tone_portamento_off(audio_generator_state_t *);
static void vibrato_on(audio_generator_state_t *, int, int, pt_waveform_t, bool);
static void vibrato_off(audio_generator_state_t *);
static void tremolo_on(audio_generator_state_t *, int, int, pt_waveform_t, bool);
static void tremolo_off(audio_generator_state_t *);
static void arpeggio_on(audio_generator_state_t *, int, int, int, int);
static void arpeggio_off(audio_generator_state_t *);
static void set_glissando(audio_generator_state_t *, bool);
static void silence_after_delay(audio_generator_state_t *, int);
static void retrigger(audio_generator_state_t *, int);
static void tick(audio_generator_state_t *, int, int);
static void apply_volume_slide(sampler_state_t *);
static void apply_pitch_slide(sampler_state_t *);
static void apply_tone_portamento(sampler_state_t *);
static void apply_vibrato(sampler_state_t *);
static void apply_tremolo(sampler_state_t *);
static void apply_arpeggio(sampler_state_t *, int);
static void apply_silence_after_delay(sampler_state_t *, int);
static void apply_retrigger(sampler_state_t *, int);

audio_generator_t init_sampler(const int note, const player_sample_t *sample, const player_sample_slice_t slice, const uint8_t volume, const float *gain_curve, sampler_state_t *sampler_state)
{
    const int16_t period = period_for_note(note, sample->fine_tuning);
    memset(sampler_state, 0, sizeof(sampler_state_t));
    sampler_state->sample = sample;
    sampler_state->sample_end = sample->sample_end;
    sampler_state->arpeggio.enabled = false;
    sampler_state->period = period;
    sampler_state->tone_portamento_on = false;
    sampler_state->tone_portamento_target_period = period;
    sampler_state->tone_portamento_slide_rate = 0;
    sampler_state->interpolation_type = sample->interpolation_type;
    sampler_state->gain_curve = gain_curve;
    sampler_state->volume = volume;
    sampler_state->volume_slide_rate = 0;
    sampler_state->vibrato_period_modulation = 0;
    sampler_state->vibrato = (lfo_effect_t) {0};
    sampler_state->tremolo = (lfo_effect_t) {0};
    if (slice.length > 0 && slice.offset < (uint32_t) sample->sample_end)
    {
        sampler_state->phase_accumulator = (float) slice.offset;
        if (slice.offset + slice.length < (uint32_t) sample->sample_end)
            sampler_state->sample_end = (int) (slice.offset + slice.length);
    }
    const audio_generator_state_t generator_state = {
        .sampler = sampler_state,
    };
    return (audio_generator_t) {
        .state = generator_state,
        .generate_audio = generate_audio,
        .set_volume = set_volume,
        .volume_slide_on = volume_slide_on,
        .volume_slide_off = volume_slide_off,
        .pitch_slide_on = pitch_slide_on,
        .pitch_slide_off = pitch_slide_off,
        .set_tone_portamento_target = set_tone_portamento_target,
        .tone_portamento_on = tone_portamento_on,
        .tone_portamento_off = tone_portamento_off,
        .vibrato_on = vibrato_on,
        .vibrato_off = vibrato_off,
        .tremolo_on = tremolo_on,
        .tremolo_off = tremolo_off,
        .arpeggio_on = arpeggio_on,
        .arpeggio_off = arpeggio_off,
        .set_glissando = set_glissando,
        .silence_after_delay = silence_after_delay,
        .retrigger = retrigger,
        .tick = tick,
    };
}

static bool generate_audio(audio_generator_state_t *state, float *channel_buffer, int frames_to_write)
{
    sampler_state_t *sampler = state->sampler;
    const int period = sampler->period + sampler->vibrato_period_modulation + sampler->arpeggio_period_modulation;
    if (period <= 0)
    {
        // Fill the buffer with silence.
        memset(channel_buffer, 0, frames_to_write * sizeof(float));
        return false;
    }
    int volume = sampler->volume + sampler->volume_modulation;
    if (volume > 255) volume = 255;
    if (volume < 0) volume = 0;
    float (*interpolate)(const float *, float) = sampler->interpolation_type == LINEAR ? interpolate_linear : interpolate_none;
    const player_sample_t *sample = sampler->sample;
    const float *sample_data = sample->sample_data;
    const float phase_increment = sample->phase_increment_per_period / (float) period;
    const float sample_end = (float) sampler->sample_end;
    const float repeat_length = (float) sample->repeat_length;
    const bool sample_repeats = sample->sample_repeats;
    float phase_accumulator = sampler->phase_accumulator;
    int offset = 0;
    const float gain = sampler->gain_curve[volume];
    while (frames_to_write > 0)
    {
        channel_buffer[offset++] = interpolate(sample_data, phase_accumulator) * gain;
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
        // Fill the remainder of the buffer with silence.
        memset(channel_buffer + offset, 0, frames_to_write * sizeof(float));
        return false;
    }
    sampler->phase_accumulator = phase_accumulator;
    return true;
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

static void set_volume(audio_generator_state_t *state, const uint8_t volume)
{
    sampler_state_t *sampler = state->sampler;
    sampler->volume = volume;
}

static void volume_slide_on(audio_generator_state_t *state, const int slide_rate, const bool fine)
{
    sampler_state_t *sampler = state->sampler;
    sampler->volume_slide_rate = slide_rate;
    sampler->volume_slide_fine = fine;
    if (sampler->volume_slide_fine)
    {
        int new_volume = sampler->volume + sampler->volume_slide_rate;
        if (new_volume < 0) new_volume = 0;
        if (new_volume > 255) new_volume = 255;
        sampler->volume = new_volume;
    }
}

static void volume_slide_off(audio_generator_state_t *state)
{
    sampler_state_t *sampler = state->sampler;
    sampler->volume_slide_rate = 0;
}

static void pitch_slide_on(audio_generator_state_t *state, const int slide_rate, const bool fine)
{
    sampler_state_t *sampler = state->sampler;
    sampler->pitch_slide_rate = slide_rate;
    sampler->pitch_slide_fine = fine;
    if (sampler->pitch_slide_fine)
    {
        int new_period = sampler->period + slide_rate;
        if (new_period < PERIOD_MIN) new_period = PERIOD_MIN;
        if (new_period > PERIOD_MAX) new_period = PERIOD_MAX;
        sampler->period = new_period;
    }
}

static void pitch_slide_off(audio_generator_state_t *state)
{
    sampler_state_t *sampler = state->sampler;
    sampler->pitch_slide_rate = 0;
}

static void set_tone_portamento_target(audio_generator_state_t *state, const int target_note)
{
    sampler_state_t *sampler = state->sampler;
    const double fine_tuning = sampler->sample->fine_tuning;
    sampler->tone_portamento_target_period = period_for_note(target_note, fine_tuning);
}

static void tone_portamento_on(audio_generator_state_t *state, const int slide_rate)
{
    sampler_state_t *sampler = state->sampler;
    sampler->tone_portamento_on = true;
    sampler->tone_portamento_slide_rate = slide_rate;
}

static void tone_portamento_off(audio_generator_state_t *state)
{
    sampler_state_t *sampler = state->sampler;
    sampler->tone_portamento_on = false;
}

static void vibrato_on(audio_generator_state_t *state, const int rate, const int depth, const pt_waveform_t waveform, const bool retrigger)
{
    sampler_state_t *sampler = state->sampler;
    sampler->vibrato.rate = rate;
    sampler->vibrato.depth = depth;
    sampler->vibrato.waveform = waveform;
    if (retrigger && !sampler->vibrato.enabled)
    {
        sampler->vibrato.phase = 0;
    }
    sampler->vibrato.enabled = true;
}

static void vibrato_off(audio_generator_state_t *state)
{
    sampler_state_t *sampler = state->sampler;
    sampler->vibrato.enabled = false;
    sampler->vibrato_period_modulation = 0;
}

static void tremolo_on(audio_generator_state_t *state, const int rate, const int depth, const pt_waveform_t waveform, const bool retrigger)
{
    sampler_state_t *sampler = state->sampler;
    sampler->tremolo.rate = rate;
    sampler->tremolo.depth = depth;
    sampler->tremolo.waveform = waveform;
    if (retrigger && !sampler->tremolo.enabled)
    {
        sampler->tremolo.phase = 0;
    }
    sampler->tremolo.enabled = true;
}

static void tremolo_off(audio_generator_state_t *state)
{
    sampler_state_t *sampler = state->sampler;
    sampler->tremolo.enabled = false;
    sampler->volume_modulation = 0;
}

static void arpeggio_on(audio_generator_state_t *state, const int root_note, const int interval_1, const int interval_2, const int speed)
{
    sampler_state_t *sampler = state->sampler;
    const int arpeggio_note_1 = root_note;
    uint16_t arpeggio_note_2 = arpeggio_note_1 + interval_1;
    uint16_t arpeggio_note_3 = arpeggio_note_1 + interval_2;
    if (note_out_of_range(arpeggio_note_2)) arpeggio_note_2 = arpeggio_note_1;
    if (note_out_of_range(arpeggio_note_3)) arpeggio_note_3 = arpeggio_note_1;
    const double fine_tuning = sampler->sample->fine_tuning;
    sampler->arpeggio.chord[0] = period_for_note(arpeggio_note_1, fine_tuning) - sampler->period;
    sampler->arpeggio.chord[1] = period_for_note(arpeggio_note_2, fine_tuning) - sampler->period;
    sampler->arpeggio.chord[2] = period_for_note(arpeggio_note_3, fine_tuning) - sampler->period;
    sampler->arpeggio.speed = speed;
    sampler->arpeggio.enabled = true;
}

static void arpeggio_off(audio_generator_state_t *state)
{
    sampler_state_t *sampler = state->sampler;
    sampler->arpeggio.enabled = false;
    sampler->arpeggio_period_modulation = 0;
}

static void set_glissando(audio_generator_state_t *state, const bool glissando_on)
{
    sampler_state_t *sampler = state->sampler;
    sampler->glissando_on = glissando_on;
}

static void silence_after_delay(audio_generator_state_t *state, const int silence_delay)
{
    sampler_state_t *sampler = state->sampler;
    sampler->silence_delay = silence_delay;
}

static void retrigger(audio_generator_state_t *state, const int retrigger_delay)
{
    sampler_state_t *sampler = state->sampler;
    sampler->retrigger_delay = retrigger_delay;
}

// static void print_state(const sampler_state_t *sampler, const int tick, const int ticks_per_event)
// {
//     printf("(%02d/%02d) %s %s %s %s %s %s\n",
//         tick,
//         ticks_per_event,
//         sampler->volume_slide_rate != 0 ? "[VOL]" : " VOL ",
//         sampler->pitch_slide_rate != 0 ? "[PIT]" : " PIT ",
//         sampler->tone_portamento_on ? "[POR]" : " POR ",
//         sampler->vibrato.enabled ? "[VIB]" : " VIB ",
//         sampler->tremolo.enabled ? "[TRE]" : " TRE ",
//         sampler->arpeggio.enabled ? "[ARP]" : " ARP ");
// }

static void tick(audio_generator_state_t *state, const int tick, const int ticks_per_event)
{
    (void) ticks_per_event;
    sampler_state_t *sampler = state->sampler;
    if (sampler->volume_slide_rate != 0)
        apply_volume_slide(sampler);
    if (sampler->pitch_slide_rate != 0)
        apply_pitch_slide(sampler);
    if (sampler->tone_portamento_on)
        apply_tone_portamento(sampler);
    if (sampler->vibrato.enabled)
        apply_vibrato(sampler);
    if (sampler->tremolo.enabled)
        apply_tremolo(sampler);
    if (sampler->arpeggio.enabled)
        apply_arpeggio(sampler, tick);
    if (sampler->silence_delay != 0)
        apply_silence_after_delay(sampler, tick);
    if (sampler->retrigger_delay != 0)
        apply_retrigger(sampler, tick);
}

static void apply_volume_slide(sampler_state_t *sampler)
{
    if (sampler->volume_slide_fine) return;
    int new_volume = sampler->volume + sampler->volume_slide_rate;
    if (new_volume < 0) new_volume = 0;
    if (new_volume > 255) new_volume = 255;
    sampler->volume = new_volume;
}

static void apply_pitch_slide(sampler_state_t *sampler)
{
    if (sampler->pitch_slide_fine) return;
    int new_period = sampler->period + sampler->pitch_slide_rate;
    if (new_period < PERIOD_MIN) new_period = PERIOD_MIN;
    if (new_period > PERIOD_MAX) new_period = PERIOD_MAX;
    sampler->period = new_period;
}

static void apply_tone_portamento(sampler_state_t *sampler)
{
    const int slide_rate = sampler->tone_portamento_slide_rate;
    if (sampler->period < sampler->tone_portamento_target_period)
    {
        sampler->period += slide_rate;
        if (sampler->period > sampler->tone_portamento_target_period)
        {
            sampler->period = sampler->tone_portamento_target_period;
            sampler->tone_portamento_on = false;
        }
    }
    else
    {
        sampler->period -= slide_rate;
        if (sampler->period < sampler->tone_portamento_target_period)
        {
            sampler->period = sampler->tone_portamento_target_period;
            sampler->tone_portamento_on = false;
        }
    }
    if (sampler->glissando_on)
    {
        const int snapped_period = nearest_note_period(sampler->period, sampler->sample->fine_tuning);
        sampler->vibrato_period_modulation = snapped_period - sampler->period;
    }
    else
    {
        sampler->vibrato_period_modulation = 0;
    }
}

static void apply_vibrato(sampler_state_t *sampler)
{
    const uint8_t rate = sampler->vibrato.rate;
    const uint8_t depth = sampler->vibrato.depth;
    const float lfo_value = lfo_pt_waveform(sampler->vibrato.waveform, sampler->vibrato.phase);
    const float period_modulation = lfo_value * (float) depth * 2.0f;
    sampler->vibrato_period_modulation = (int) period_modulation;
    sampler->vibrato.phase = (sampler->vibrato.phase + rate) % PT_LFO_WAVELENGTH;
}

static void apply_tremolo(sampler_state_t *sampler)
{
    const uint8_t rate = sampler->tremolo.rate;
    const uint8_t depth = sampler->tremolo.depth;
    const float lfo_value = lfo_pt_waveform(sampler->tremolo.waveform, sampler->tremolo.phase);
    const float volume_modulation = lfo_value * (float) depth * 16.0f;
    sampler->volume_modulation = (int) volume_modulation;
    sampler->tremolo.phase = (sampler->tremolo.phase + rate) % PT_LFO_WAVELENGTH;
}

static void apply_arpeggio(sampler_state_t *sampler, const int tick)
{
    if (tick % sampler->arpeggio.speed != 0) return;
    sampler->arpeggio_period_modulation = sampler->arpeggio.chord[sampler->arpeggio.counter % 3];
    sampler->arpeggio.counter += 1;
}

static void apply_silence_after_delay(sampler_state_t *sampler, const int tick)
{
    if (tick == sampler->silence_delay)
        sampler->volume = 0;
}

static void apply_retrigger(sampler_state_t *sampler, const int tick)
{
    if (tick % sampler->retrigger_delay == 0)
        sampler->phase_accumulator = 0.0f;
}
