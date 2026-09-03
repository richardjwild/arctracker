#include "effects.h"
#include <stdlib.h>
#include "sequencer.h"
#include "period.h"
#include "memory/bits.h"
#include "pcm/mu_law.h"

static const uint8_t PAN_CENTRE = 0x80;

static void save_and_reset_effect_state(voice_t *, bool *, bool *);
static void volume_slide_up(voice_t *, uint8_t);
static void volume_slide_down(voice_t *, uint8_t);
static void combined_volume_side(voice_t *, uint8_t);
static void portamento_up(voice_t *, uint8_t);
static void portamento_down(voice_t *, uint8_t);
static void start_tone_portamento(voice_t *, uint8_t);
static void tone_portamento(voice_t *);
static void start_arpeggio(voice_t *);
static void define_loop(voice_t *, sequence_t *, uint8_t);
static void set_glissando_mode(voice_t *voice, uint8_t);
static void apply_arpeggio(voice_t *, uint8_t);
static void retrigger_sample(const event_scheduler_t *, voice_t *, uint8_t);
static void start_vibrato(voice_t *, uint8_t, bool);
static void apply_vibrato(voice_t *);
static void start_tremolo(voice_t *, uint8_t, bool);
static void apply_tremolo(voice_t *);
static void set_lfo_waveform(lfo_effect_t *, uint8_t);
static void set_volume(voice_t *, uint8_t);
static void set_tempo(player_t *, uint8_t);
static void set_voice_panning(voice_t *, uint8_t);
static void pattern_break(sequence_t *, uint8_t);
static void set_tempo_fine(tick_scheduler_t *, uint8_t);
static void delay_next_event(tick_scheduler_t *, uint8_t);
static void silence_voice(const tick_scheduler_t *tick_scheduler, voice_t *voice, uint8_t data);

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

void handle_effects_before_note(const event_t *event, const voice_t *voice, player_t *player)
{
    for (int effect_no = 0; effect_no < MAX_EFFECTS; effect_no++)
    {
        const effect_t effect = event->effects[effect_no];
        if (effect.command == SET_FINETUNE)
            player_set_sample_finetune(player, voice->instrument_no, (int8_t) effect.data);
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

void handle_effects_on_event(const event_t *event, voice_t *voice, player_t *player)
{
    bool vibrato_already_on, tremolo_already_on;
    save_and_reset_effect_state(voice, &vibrato_already_on, &tremolo_already_on);
    for (int effect_no = 0; effect_no < MAX_EFFECTS; effect_no++)
    {
        const effect_t effect = event->effects[effect_no];
        if (effect.command == PORTAMENTO)
            start_tone_portamento(voice, effect.data);
        if (effect.command == SET_VOLUME)
            set_volume(voice, effect.data);
        if (effect.command == FINE_CRESCENDO)
            volume_slide_up(voice, effect.data);
        if (effect.command == FINE_DECRESCENDO)
            volume_slide_down(voice, effect.data);
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
        if (effect.command == FINE_PORTAMENTO)
        {
            if ((effect.data & 0x80) == 0) portamento_up(voice, effect.data);
            else portamento_down(voice, effect.data & 0x7f);
        }
        if (effect.command == VIBRATO)
            start_vibrato(voice, effect.data, vibrato_already_on);
        if (effect.command == TREMOLO)
            start_tremolo(voice, effect.data, tremolo_already_on);
        if (effect.command == SET_LFO_WAVEFORM)
        {
            const uint8_t effect_type = effect.data >> 4;
            if (effect_type == 0) set_lfo_waveform(&voice->vibrato, effect.data & 0xf);
            if (effect_type == 1) set_lfo_waveform(&voice->tremolo, effect.data & 0xf);
        }
        if (effect.command == ARPEGGIO)
            start_arpeggio(voice);
        if (effect.command == SET_LOOP)
            define_loop(voice, &player->sequence, effect.data);
        if (effect.command == SET_GLISSANDO_MODE)
            set_glissando_mode(voice, effect.data);
    }
    if (!voice->arpeggiator_on)
    {
        voice->arpeggio_counter = 1;
        if (!voice->vibrato.enabled && !voice->tremolo.enabled)
            voice->period_modulation = 0;
    }
}

static void save_and_reset_effect_state(voice_t *voice, bool *vibrato_already_on, bool *tremolo_already_on)
{
    *vibrato_already_on = voice->vibrato.enabled;
    *tremolo_already_on = voice->tremolo.enabled;
    voice->vibrato.enabled = false;
    voice->tremolo.enabled = false;
    voice->arpeggiator_on = false;
}

void handle_effects_off_event(const event_t *event, voice_t *voice, player_t *player)
{
    for (int effect_no = 0; effect_no < MAX_EFFECTS; effect_no++)
    {
        const effect_t effect = event->effects[effect_no];
        if (effect.command == CRESCENDO)
            volume_slide_up(voice, effect.data);
        if (effect.command == DECRESCENDO)
            volume_slide_down(voice, effect.data);
        if (effect.command == PITCH_SLIDE_UP)
            portamento_up(voice, effect.data);
        if (effect.command == PITCH_SLIDE_DOWN)
            portamento_down(voice, effect.data);
        if (effect.command == PORTAMENTO)
            tone_portamento(voice);
        if (effect.command == PORTAMENTO_PLUS_VOLUME_SIDE)
        {
            tone_portamento(voice);
            combined_volume_side(voice, effect.data);
        }
        if (effect.command == SILENCE_SAMPLE_AFTER_DELAY)
            silence_voice(&player->tick_scheduler, voice, effect.data);
        if (effect.command == VIBRATO)
            apply_vibrato(voice);
        if (effect.command == VIBRATO_PLUS_VOLUME_SLIDE)
        {
            apply_vibrato(voice);
            combined_volume_side(voice, effect.data);
        }
        if (effect.command == TREMOLO)
            apply_tremolo(voice);
        if (effect.command == ARPEGGIO)
            apply_arpeggio(voice, effect.data);
        if (effect.command == RETRIGGER_SAMPLE)
            retrigger_sample(&player->tick_scheduler.event_scheduler, voice, effect.data);
    }
}

static void volume_slide_up(voice_t *voice, const uint8_t data)
{
    const uint8_t headroom = INTERNAL_GAIN_MAX - voice->volume;
    const uint8_t change = headroom > data ? data : headroom;
    voice->volume += change;
}

static void volume_slide_down(voice_t *voice, const uint8_t data)
{
    if (voice->volume >= data)
        voice->volume -= data;
    else
        voice->volume = 0;
}

static void combined_volume_side(voice_t *voice, const uint8_t data)
{
    const uint8_t up_amount = data >> 4;
    const uint8_t down_amount = data & 0xf;
    if (up_amount > 0 && down_amount > 0) return;
    if (up_amount > 0) volume_slide_up(voice, up_amount);
    if (down_amount > 0) volume_slide_down(voice, down_amount);
}

static void portamento_up(voice_t *voice, const uint8_t data)
{
    voice->period -= data;
    if (voice->period < PERIOD_MIN)
        voice->period = PERIOD_MIN;
}

static void portamento_down(voice_t *voice, const uint8_t data)
{
    voice->period += data;
    if (voice->period > PERIOD_MAX)
        voice->period = PERIOD_MAX;
}

static void start_tone_portamento(voice_t *voice, const uint8_t data)
{
    if (data)
        voice->effect_memory.tone_portamento_speed = data;
}

static void tone_portamento(voice_t *voice)
{
    const int portamento_speed = voice->effect_memory.tone_portamento_speed;
    if (voice->period < voice->tone_portamento_target_period)
    {
        voice->period += portamento_speed;
        if (voice->period > voice->tone_portamento_target_period)
            voice->period = voice->tone_portamento_target_period;
    }
    else
    {
        voice->period -= portamento_speed;
        if (voice->period < voice->tone_portamento_target_period)
            voice->period = voice->tone_portamento_target_period;
    }
    if (voice->glissando_on)
    {
        const int snapped_period = nearest_note_period(voice->period, voice->sample->fine_tuning);
        voice->period_modulation = snapped_period - voice->period;
    }
    else
    {
        voice->period_modulation = 0;
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

static void start_vibrato(voice_t *voice, const uint8_t data, const bool already_on)
{
    voice->vibrato.enabled = true;
    if (!already_on && voice->vibrato.retrigger)
        voice->vibrato.phase = 0;
    if ((data & 0xf0) != 0)
        voice->effect_memory.vibrato_speed = data >> 4;
    if ((data & 0xf) != 0)
        voice->effect_memory.vibrato_depth = data & 0xf;
}

static void apply_vibrato(voice_t *voice)
{
    const uint8_t vibrato_speed = voice->effect_memory.vibrato_speed;
    const uint8_t vibrato_depth = voice->effect_memory.vibrato_depth;
    const float lfo_value = lfo_pt_waveform(voice->vibrato.waveform, voice->vibrato.phase);
    const float period_modulation = lfo_value * (float) vibrato_depth * 2.0f;
    voice->period_modulation = (int) period_modulation;
    voice->vibrato.phase = (voice->vibrato.phase + vibrato_speed) % PT_LFO_WAVELENGTH;
}

static void start_tremolo(voice_t *voice, const uint8_t data, const bool already_on)
{
    voice->tremolo.enabled = true;
    if (!already_on && voice->tremolo.retrigger)
        voice->tremolo.phase = 0;
    if ((data & 0xf0) != 0)
        voice->effect_memory.tremolo_speed = data >> 4;
    if ((data & 0xf) != 0)
        voice->effect_memory.tremolo_depth = data & 0xf;
}

static void apply_tremolo(voice_t *voice)
{
    const uint8_t tremolo_speed = voice->effect_memory.tremolo_speed;
    const uint8_t tremolo_depth = voice->effect_memory.tremolo_depth;
    const float lfo_value = lfo_pt_waveform(voice->tremolo.waveform, voice->tremolo.phase);
    const float volume_modulation = lfo_value * (float) tremolo_depth * 16.0f;
    voice->volume_modulation = (int) volume_modulation;
    voice->tremolo.phase = (voice->tremolo.phase + tremolo_speed) % PT_LFO_WAVELENGTH;
}

static void start_arpeggio(voice_t *voice)
{
    voice->arpeggiator_on = true;
}

static void define_loop(voice_t *voice, sequence_t *sequence, const uint8_t data)
{
    if (data == 0)
    {
        voice->loop_state.start = sequence->pattern_index;
        return;
    }
    if (voice->loop_state.looping)
    {
        voice->loop_state.counter -= 1;
        if (voice->loop_state.counter == 0)
        {
            clear_pattern_loop(sequence);
            voice->loop_state.looping = false;
        }
    }
    else
    {
        set_loop(sequence, voice->loop_state.start, sequence->pattern_index);
        voice->loop_state.looping = true;
        voice->loop_state.counter = data;
    }
}

static void set_glissando_mode(voice_t *voice, const uint8_t data)
{
    voice->glissando_on = data != 0;
}

static void apply_arpeggio(voice_t *voice, const uint8_t data)
{
    const int chord[] = {
            voice->current_note,
            voice->current_note + HIGH_NYBBLE(data),
            voice->current_note + LOW_NYBBLE(data)
    };
    int arpeggio_note = chord[voice->arpeggio_counter % 3];
    if (note_out_of_range(arpeggio_note))
    {
        arpeggio_note = voice->current_note;
    }
    voice->period_modulation = period_for_note(arpeggio_note, voice->sample->fine_tuning) - voice->period;
    voice->arpeggio_counter += 1;
}

static void retrigger_sample(const event_scheduler_t *event_scheduler, voice_t *voice, const uint8_t data)
{
    if (data == 0 || event_scheduler->ticks == 0)
        return;
    if (event_scheduler->ticks % data == 0)
        voice->phase_accumulator = 0.0f;
}

static void set_volume(voice_t *voice, const uint8_t data)
{
    voice->volume = data;
}

static void set_voice_panning(voice_t *voice, const uint8_t data)
{
    voice->panning = (data == 0) ? PAN_CENTRE : data;
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

static void silence_voice(const tick_scheduler_t *tick_scheduler, voice_t *voice, const uint8_t data)
{
    if (tick_scheduler->event_scheduler.ticks == data)
        voice->volume = 0;
}
