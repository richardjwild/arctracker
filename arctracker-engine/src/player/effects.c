#include "effects.h"
#include <stdlib.h>
#include "sequencer.h"
#include "period.h"
#include "memory/bits.h"

static const uint8_t PAN_CENTRE = 0x80;

static void save_and_reset_effect_state(sampler_state_t *, bool *, bool *);
static void volume_slide_up(sampler_state_t *, uint8_t);
static void volume_slide_down(sampler_state_t *, uint8_t);
static void combined_volume_side(sampler_state_t *, uint8_t);
static void portamento_up(sampler_state_t *, uint8_t);
static void portamento_down(sampler_state_t *, uint8_t);
static void start_tone_portamento(player_track_t *, uint8_t);
static void tone_portamento(sampler_state_t *, const player_track_t *);
static void start_arpeggio(sampler_state_t *, const player_track_t *, uint8_t);
static void define_loop(player_track_t *, sequence_t *, uint8_t);
static void set_glissando_mode(sampler_state_t *, uint8_t);
static void apply_arpeggio(sampler_state_t *);
static void retrigger_sample(const event_scheduler_t *, sampler_state_t *, uint8_t);
static void start_vibrato(sampler_state_t *, player_track_t *, uint8_t, bool);
static void apply_vibrato(sampler_state_t *, const player_track_t *);
static void start_tremolo(sampler_state_t *, player_track_t *, uint8_t, bool);
static void apply_tremolo(sampler_state_t *, const player_track_t *);
static void set_lfo_waveform(lfo_effect_t *, uint8_t);
static void set_volume(sampler_state_t *, uint8_t);
static void set_tempo(player_t *, uint8_t);
static void set_voice_panning(voice_t *, uint8_t);
static void pattern_break(sequence_t *, uint8_t);
static void set_tempo_fine(tick_scheduler_t *, uint8_t);
static void delay_next_event(tick_scheduler_t *, uint8_t);
static void silence_voice(const tick_scheduler_t *, sampler_state_t *, uint8_t);

bool portamento(const event_t *event)
{
    const effect_t *effects = event->effects;
    for (int i = 0; i < 4; i++, effects++)
    {
        if (effects->command == PORTAMENTO || effects->command == PORTAMENTO_PLUS_VOLUME_SIDE)
            return true;
    }
    return false;
}

void handle_effects_before_note(const event_t *event, const player_track_t *track, player_t *player)
{
    for (int effect_no = 0; effect_no < MAX_EFFECTS; effect_no++)
    {
        const effect_t effect = event->effects[effect_no];
        if (effect.command == SET_FINETUNE)
            player_set_sample_finetune(player, track->instrument_no, (int8_t) effect.data);
    }
}

uint8_t get_note_delay(const event_t *event)
{
    for (int effect_no = 0; effect_no < MAX_EFFECTS; effect_no++)
    {
        const effect_t effect = event->effects[effect_no];
        if (effect.command == DELAY_SAMPLE)
            return effect.data;
    }
    return 0;
}

uint8_t get_sample_slice(const event_t *event)
{
    for (int effect_no = 0; effect_no < MAX_EFFECTS; effect_no++)
    {
        const effect_t effect = event->effects[effect_no];
        if (effect.command == USE_SAMPLE_SLICE)
            return effect.data;
    }
    return 0;
}

void handle_effects_on_event(const event_t *event, voice_t *voice, player_track_t *track, player_t *player)
{
    bool vibrato_already_on, tremolo_already_on;
    sampler_state_t *sampler_state = &voice->sampler_state;
    save_and_reset_effect_state(sampler_state, &vibrato_already_on, &tremolo_already_on);
    for (int effect_no = 0; effect_no < MAX_EFFECTS; effect_no++)
    {
        const effect_t effect = event->effects[effect_no];
        if (effect.command == PORTAMENTO)
            start_tone_portamento(track, effect.data);
        if (effect.command == SET_VOLUME)
            set_volume(sampler_state, effect.data);
        if (effect.command == FINE_CRESCENDO)
            volume_slide_up(sampler_state, effect.data);
        if (effect.command == FINE_DECRESCENDO)
            volume_slide_down(sampler_state, effect.data);
        if (effect.command == SET_TEMPO)
            set_tempo(player, effect.data);
        if (effect.command == SET_PANNING)
            set_voice_panning(voice, effect.data);
        if (effect.command == PATTERN_BREAK)
            pattern_break(&player->sequence, effect.data);
        if (effect.command == SEQUENCE_JUMP)
            set_jump_target(effect.data, 0, &player->sequence);
        if (effect.command == SET_TICKS_PER_SECOND)
            set_tempo_fine(&player->tick_scheduler, effect.data);
        if (effect.command == DELAY_NEXT_EVENT)
            delay_next_event(&player->tick_scheduler, effect.data);
        if (effect.command == FINE_PORTAMENTO_UP)
            portamento_up(sampler_state, effect.data);
        if (effect.command == FINE_PORTAMENTO_DOWN)
            portamento_down(sampler_state, effect.data);
        if (effect.command == VIBRATO)
            start_vibrato(sampler_state, track, effect.data, vibrato_already_on);
        if (effect.command == TREMOLO)
            start_tremolo(sampler_state, track, effect.data, tremolo_already_on);
        if (effect.command == SET_VIBRATO_WAVEFORM)
            set_lfo_waveform(&sampler_state->vibrato, effect.data);
        if (effect.command == SET_TREMOLO_WAVEFORM)
            set_lfo_waveform(&sampler_state->tremolo, effect.data);
        if (effect.command == ARPEGGIO)
            start_arpeggio(sampler_state, track, effect.data);
        if (effect.command == SET_LOOP)
            define_loop(track, &player->sequence, effect.data);
        if (effect.command == SET_GLISSANDO_MODE)
            set_glissando_mode(sampler_state, effect.data);
    }
    if (!sampler_state->arpeggio.enabled)
    {
        sampler_state->arpeggio.counter = 1;
        if (!sampler_state->vibrato.enabled && !sampler_state->tremolo.enabled)
            sampler_state->period_modulation = 0;
    }
}

static void save_and_reset_effect_state(sampler_state_t *sampler_state, bool *vibrato_already_on, bool *tremolo_already_on)
{
    *vibrato_already_on = sampler_state->vibrato.enabled;
    *tremolo_already_on = sampler_state->tremolo.enabled;
    sampler_state->vibrato.enabled = false;
    sampler_state->tremolo.enabled = false;
    sampler_state->arpeggio.enabled = false;
}

void handle_effects_off_event(const event_t *event, voice_t *voice, const player_track_t *track, const player_t *player)
{
    sampler_state_t *sampler_state = &voice->sampler_state;
    for (int effect_no = 0; effect_no < MAX_EFFECTS; effect_no++)
    {
        const effect_t effect = event->effects[effect_no];
        if (effect.command == VOLUME_SLIDE)
        {
            if ((effect.data & 0x80) > 0) volume_slide_up(sampler_state, effect.data & 0x7f);
            else volume_slide_down(sampler_state, effect.data);
        }
        if (effect.command == PITCH_SLIDE_UP)
            portamento_up(sampler_state, effect.data);
        if (effect.command == PITCH_SLIDE_DOWN)
            portamento_down(sampler_state, effect.data);
        if (effect.command == PORTAMENTO)
            tone_portamento(sampler_state, track);
        if (effect.command == PORTAMENTO_PLUS_VOLUME_SIDE)
        {
            tone_portamento(sampler_state, track);
            combined_volume_side(sampler_state, effect.data);
        }
        if (effect.command == SILENCE_SAMPLE_AFTER_DELAY)
            silence_voice(&player->tick_scheduler, sampler_state, effect.data);
        if (effect.command == VIBRATO)
            apply_vibrato(sampler_state, track);
        if (effect.command == VIBRATO_PLUS_VOLUME_SLIDE)
        {
            apply_vibrato(sampler_state, track);
            combined_volume_side(sampler_state, effect.data);
        }
        if (effect.command == TREMOLO)
            apply_tremolo(sampler_state, track);
        if (effect.command == ARPEGGIO)
            apply_arpeggio(sampler_state);
        if (effect.command == RETRIGGER_SAMPLE)
            retrigger_sample(&player->tick_scheduler.event_scheduler, sampler_state, effect.data);
    }
}

static void volume_slide_up(sampler_state_t *sampler_state, const uint8_t data)
{
    const uint8_t headroom = INTERNAL_GAIN_MAX - sampler_state->volume;
    const uint8_t change = headroom > data ? data : headroom;
    sampler_state->volume += change;
}

static void volume_slide_down(sampler_state_t *sampler_state, const uint8_t data)
{
    if (sampler_state->volume >= data)
        sampler_state->volume -= data;
    else
        sampler_state->volume = 0;
}

static void combined_volume_side(sampler_state_t *sampler_state, const uint8_t data)
{
    const uint8_t up_amount = data >> 4;
    const uint8_t down_amount = data & 0xf;
    if (up_amount > 0 && down_amount > 0) return;
    if (up_amount > 0) volume_slide_up(sampler_state, up_amount);
    if (down_amount > 0) volume_slide_down(sampler_state, down_amount);
}

static void portamento_up(sampler_state_t *sampler_state, const uint8_t data)
{
    sampler_state->period -= data;
    if (sampler_state->period < PERIOD_MIN)
        sampler_state->period = PERIOD_MIN;
}

static void portamento_down(sampler_state_t *sampler_state, const uint8_t data)
{
    sampler_state->period += data;
    if (sampler_state->period > PERIOD_MAX)
        sampler_state->period = PERIOD_MAX;
}

static void start_tone_portamento(player_track_t *track, const uint8_t data)
{
    if (data)
        track->effect_memory.tone_portamento_speed = data;
}

static void tone_portamento(sampler_state_t *sampler_state, const player_track_t *track)
{
    const int portamento_speed = track->effect_memory.tone_portamento_speed;
    if (sampler_state->period < sampler_state->tone_portamento_target_period)
    {
        sampler_state->period += portamento_speed;
        if (sampler_state->period > sampler_state->tone_portamento_target_period)
            sampler_state->period = sampler_state->tone_portamento_target_period;
    }
    else
    {
        sampler_state->period -= portamento_speed;
        if (sampler_state->period < sampler_state->tone_portamento_target_period)
            sampler_state->period = sampler_state->tone_portamento_target_period;
    }
    if (sampler_state->glissando_on)
    {
        const int snapped_period = nearest_note_period(sampler_state->period, sampler_state->sample->fine_tuning);
        sampler_state->period_modulation = snapped_period - sampler_state->period;
    }
    else
    {
        sampler_state->period_modulation = 0;
    }
}

static void set_lfo_waveform(lfo_effect_t *lfo_effect, const uint8_t data)
{
    lfo_effect->retrigger = (data & 0x4) == 0;
    switch (data & 0x3)
    {
        case 0: lfo_effect->waveform = PT_WAVEFORM_SINE;
            break;
        case 1: lfo_effect->waveform = PT_WAVEFORM_RAMP;
            break;
        default: lfo_effect->waveform = PT_WAVEFORM_SQUARE;
            break;
    }
    // Some sources claim that type 3 (or 7) select a waveform at random, but this is disputed.
    // Other sources claim that it selects a noise waveform, and others still say it selects square.
}

static void start_vibrato(sampler_state_t *sampler_state, player_track_t *track, const uint8_t data, const bool already_on)
{
    sampler_state->vibrato.enabled = true;
    if (!already_on && sampler_state->vibrato.retrigger)
        sampler_state->vibrato.phase = 0;
    if ((data & 0xf0) != 0)
        track->effect_memory.vibrato_speed = data >> 4;
    if ((data & 0xf) != 0)
        track->effect_memory.vibrato_depth = data & 0xf;
}

static void apply_vibrato(sampler_state_t *sampler_state, const player_track_t *track)
{
    const uint8_t vibrato_speed = track->effect_memory.vibrato_speed;
    const uint8_t vibrato_depth = track->effect_memory.vibrato_depth;
    const float lfo_value = lfo_pt_waveform(sampler_state->vibrato.waveform, sampler_state->vibrato.phase);
    const float period_modulation = lfo_value * (float) vibrato_depth * 2.0f;
    sampler_state->period_modulation = (int) period_modulation;
    sampler_state->vibrato.phase = (sampler_state->vibrato.phase + vibrato_speed) % PT_LFO_WAVELENGTH;
}

static void start_tremolo(sampler_state_t *sampler_state, player_track_t *track, const uint8_t data, const bool already_on)
{
    sampler_state->tremolo.enabled = true;
    if (!already_on && sampler_state->tremolo.retrigger)
        sampler_state->tremolo.phase = 0;
    if ((data & 0xf0) != 0)
        track->effect_memory.tremolo_speed = data >> 4;
    if ((data & 0xf) != 0)
        track->effect_memory.tremolo_depth = data & 0xf;
}

static void apply_tremolo(sampler_state_t *sampler_state, const player_track_t *track)
{
    const uint8_t tremolo_speed = track->effect_memory.tremolo_speed;
    const uint8_t tremolo_depth = track->effect_memory.tremolo_depth;
    const float lfo_value = lfo_pt_waveform(sampler_state->tremolo.waveform, sampler_state->tremolo.phase);
    const float volume_modulation = lfo_value * (float) tremolo_depth * 16.0f;
    sampler_state->volume_modulation = (int) volume_modulation;
    sampler_state->tremolo.phase = (sampler_state->tremolo.phase + tremolo_speed) % PT_LFO_WAVELENGTH;
}

static void start_arpeggio(sampler_state_t *sampler_state, const player_track_t *track, const uint8_t data)
{
    uint16_t arpeggio_note_2 = track->current_note + HIGH_NYBBLE(data);
    if (note_out_of_range(arpeggio_note_2))
    {
        arpeggio_note_2 = track->current_note;
    }
    uint16_t arpeggio_note_3 = track->current_note + LOW_NYBBLE(data);
    if (note_out_of_range(arpeggio_note_3))
    {
        arpeggio_note_3 = track->current_note;
    }
    sampler_state->arpeggio.enabled = true;
    sampler_state->arpeggio.chord[0] = 0;
    sampler_state->arpeggio.chord[1] = period_for_note(arpeggio_note_2, sampler_state->sample->fine_tuning) - sampler_state->period;
    sampler_state->arpeggio.chord[2] = period_for_note(arpeggio_note_3, sampler_state->sample->fine_tuning) - sampler_state->period;
}

static void define_loop(player_track_t *track, sequence_t *sequence, const uint8_t data)
{
    if (data == 0)
    {
        track->loop_state.start = sequence->pattern_index;
        return;
    }
    if (track->loop_state.looping)
    {
        track->loop_state.counter -= 1;
        if (track->loop_state.counter == 0)
        {
            clear_pattern_loop(sequence);
            track->loop_state.looping = false;
        }
    }
    else
    {
        set_loop(sequence, track->loop_state.start, sequence->pattern_index);
        track->loop_state.looping = true;
        track->loop_state.counter = data;
    }
}

static void set_glissando_mode(sampler_state_t *sampler_state, const uint8_t data)
{
    sampler_state->glissando_on = data != 0;
}

static void apply_arpeggio(sampler_state_t *sampler_state)
{
    sampler_state->period_modulation = sampler_state->arpeggio.chord[sampler_state->arpeggio.counter % 3];
    sampler_state->arpeggio.counter += 1;
}

static void retrigger_sample(const event_scheduler_t *event_scheduler, sampler_state_t *sampler_state, const uint8_t data)
{
    if (data == 0 || event_scheduler->ticks == 0)
        return;
    if (event_scheduler->ticks % data == 0)
        sampler_state->phase_accumulator = 0.0f;
}

static void set_volume(sampler_state_t *sampler_state, const uint8_t data)
{
    sampler_state->volume = data;
}

static void set_voice_panning(voice_t *voice, const uint8_t data)
{
    voice->panning = data == 0 ? PAN_CENTRE : data;
}

static void pattern_break(sequence_t *sequence, const uint8_t data)
{
    break_to_next_position(sequence, data);
}

static void set_tempo(player_t *player, const uint8_t data)
{
    if (data <= 32)
        player->tick_scheduler.event_scheduler.ticks_per_event = data;
    else if (player->module->lines_per_beat > 0)
        player_set_bpm(player, data);
}

static void set_tempo_fine(tick_scheduler_t *tick_scheduler, const uint8_t data)
{
    if (data > 0)
        tick_scheduler->audio_accumulator.ticks_per_second = data;
}

static void delay_next_event(tick_scheduler_t *tick_scheduler, const uint8_t data)
{
    if (data > 0)
        tick_scheduler->event_scheduler.event_delay = data * tick_scheduler->event_scheduler.ticks_per_event;
}

static void silence_voice(const tick_scheduler_t *tick_scheduler, sampler_state_t *sampler_state, const uint8_t data)
{
    if (tick_scheduler->event_scheduler.ticks == data)
        sampler_state->volume = 0;
}
