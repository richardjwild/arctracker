#include "effects.h"

#include <stdio.h>

#include "sequencer.h"
#include "period.h"
#include "memory/bits.h"
#include "pcm/mu_law.h"

static const uint8_t PAN_CENTRE = 0x80;

static void volume_slide_up(voice_t *, uint8_t);
static void volume_slide_down(voice_t *, uint8_t);
static void combined_volume_side(voice_t *, uint8_t);
static void portamento_up(voice_t *, uint8_t);
static void portamento_down(voice_t *, uint8_t);
static void start_tone_portamento(voice_t *, int, uint8_t);
static void tone_portamento(voice_t *, int);
static void turn_arpeggiator_on(voice_t *);
static void turn_vibrato_on(voice_t *, int, uint8_t);
static void arpeggiate(voice_t *, uint8_t);
static void set_volume(voice_t *, uint8_t);
static void set_tempo(player_t *, uint8_t);
static void set_sample_offset(const player_t *, voice_t *, uint8_t);
static void set_voice_panning(voice_t *, uint8_t);
static void pattern_break(sequence_t *, uint8_t);
static void set_tempo_fine(tick_scheduler_t *, uint8_t);
static void delay_next_event(tick_scheduler_t *, uint8_t);
static void silence_voice(const tick_scheduler_t *tick_scheduler, voice_t *voice, uint8_t data);
static void portamento_fine(voice_t *, uint8_t);
static void apply_vibrato(voice_t *, int);

void reset_effects(voice_t *voice)
{
    if (voice->arpeggiator_on || voice->vibrato_on)
    {
        voice->period_modulation = 0;
        voice->arpeggiator_on = false;
        voice->vibrato_on = false;
    }
}

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

void handle_effects_on_event(const event_t *event, voice_t *voice, player_t *player)
{
    for (int effect_no = 0; effect_no < MAX_EFFECTS; effect_no++)
    {
        const effect_t effect = event->effects[effect_no];
        if (effect.command == PORTAMENTO)
            start_tone_portamento(voice, effect_no, effect.data);
        else if (effect.command == SET_VOLUME)
            set_volume(voice, effect.data);
        else if (effect.command == FINE_CRESCENDO)
            volume_slide_up(voice, effect.data);
        else if (effect.command == FINE_DECRESCENDO)
            volume_slide_down(voice, effect.data);
        else if (effect.command == SET_TEMPO)
            set_tempo(player, effect.data);
        else if (effect.command == USE_SAMPLE_SLICE)
            set_sample_offset(player, voice, effect.data);
        else if (effect.command == SET_PANNING)
            set_voice_panning(voice, effect.data);
        else if (effect.command == PATTERN_BREAK)
            pattern_break(&player->sequence, effect.data);
        else if (effect.command == SEQUENCE_JUMP)
            set_jump_target(effect.data, 0, &player->sequence);
        else if (effect.command == SET_TICKS_PER_SECOND)
            set_tempo_fine(&player->tick_scheduler, effect.data);
        else if (effect.command == DELAY_NEXT_EVENT)
            delay_next_event(&player->tick_scheduler, effect.data);
        else if (effect.command == FINE_PORTAMENTO)
            portamento_fine(voice, effect.data);
        else if (effect.command == VIBRATO)
            turn_vibrato_on(voice, effect_no, effect.data);
        else if (effect.command == ARPEGGIO)
            turn_arpeggiator_on(voice);
    }
}

void handle_effects_off_event(const event_t *event, voice_t *voice, player_t *player)
{
    for (int effect_no = 0; effect_no < MAX_EFFECTS; effect_no++)
    {
        const effect_t effect = event->effects[effect_no];
        if (effect.command == CRESCENDO)
            volume_slide_up(voice, effect.data);
        else if (effect.command == DECRESCENDO)
            volume_slide_down(voice, effect.data);
        else if (effect.command == PITCH_SLIDE_UP)
            portamento_up(voice, effect.data);
        else if (effect.command == PITCH_SLIDE_DOWN)
            portamento_down(voice, effect.data);
        else if (effect.command == PORTAMENTO)
            tone_portamento(voice, effect_no);
        else if (effect.command == PORTAMENTO_PLUS_VOLUME_SIDE)
        {
            tone_portamento(voice, effect_no);
            combined_volume_side(voice, effect.data);
        }
        else if (effect.command == SILENCE_SAMPLE_AFTER_DELAY)
            silence_voice(&player->tick_scheduler, voice, effect.data);
        else if (effect.command == VIBRATO)
            apply_vibrato(voice, effect_no);
        else if (effect.command == VIBRATO_PLUS_VOLUME_SLIDE)
        {
            apply_vibrato(voice, effect_no);
            combined_volume_side(voice, effect.data);
        }
        else if (effect.command == ARPEGGIO)
            arpeggiate(voice, effect.data);
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

static void start_tone_portamento(voice_t *voice, const int effect_no, const uint8_t data)
{
    if (data)
        voice->effect_memory[effect_no] = data;
}

static void tone_portamento(voice_t *voice, const int effect_no)
{
    const int portamento_speed = voice->effect_memory[effect_no];
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
}

static void apply_vibrato(voice_t *voice, const int effect_no)
{
    const uint8_t vibrato_params = voice->effect_memory[effect_no];
    const uint8_t vibrato_speed = vibrato_params >> 4;
    const uint8_t vibrato_depth = vibrato_params & 0xf;
    const float offset = lfo_pt_waveform(voice->vibrato_type, voice->vibrato_phase) * (float) (vibrato_depth * 2);
    voice->period_modulation = (int) offset;
    voice->vibrato_phase = (voice->vibrato_phase + vibrato_speed) % 64;
}

static void turn_arpeggiator_on(voice_t *voice)
{
    voice->arpeggiator_on = true;
    voice->arpeggio_counter = 1;
}

static void turn_vibrato_on(voice_t *voice, const int effect_no, const uint8_t data)
{
    voice->vibrato_on = true;
    if (data >> 4 != 0 && (data & 0xf) != 0)
        voice->effect_memory[effect_no] = data;
}

static void arpeggiate(voice_t *voice, const uint8_t data)
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

static void set_volume(voice_t *voice, const uint8_t data)
{
    voice->volume = data;
}

static void portamento_fine(voice_t *voice, const uint8_t data)
{
    // TODO: This looks wrong. Find out what the semantics of this command are.
    voice->period += data;
    if (voice->period > PERIOD_MAX)
        voice->period = PERIOD_MAX;
    else if (voice->period < PERIOD_MIN)
        voice->period = PERIOD_MIN;
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

static void set_sample_offset(const player_t *player, voice_t *voice, const uint8_t data)
{
    if (!voice->channel_playing) return;
    const instrument_t *instrument = &player->module->instruments[voice->instrument_no - 1];
    if (!instrument->assigned) return;
    const uint32_t sample_length = (uint32_t) player->module->samples[instrument->sample_index].sample_length;
    const sample_slice_t slice = instrument->sample_slices[data];
    if (slice.length == 0 || slice.offset >= sample_length) return;
    voice->phase_accumulator = (float) slice.offset;
    if (slice.offset + slice.length < (uint32_t) voice->sample->sample_end)
        voice->sample->sample_end = (int) (slice.offset + slice.length);
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
