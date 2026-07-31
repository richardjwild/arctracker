#include "effects.h"
#include "sequencer.h"
#include "period.h"
#include "memory/bits.h"
#include "pcm/mu_law.h"

static const uint8_t PAN_CENTRE = 0x80;

static void volume_slide_up(voice_t *, uint8_t);
static void volume_slide_down(voice_t *, uint8_t);
static void portamento_up(voice_t *, uint8_t);
static void portamento_down(voice_t *, uint8_t);
static void start_tone_portamento(voice_t *, int, uint8_t);
static void tone_portamento(voice_t *, int);
static void turn_arpeggiator_on(voice_t *);
static void arpeggiate(voice_t *, uint8_t);
static void set_volume(voice_t *, uint8_t);
static void set_tempo(player_t *, uint8_t);
static void set_voice_panning(voice_t *, uint8_t);
static void set_tempo_fine(tick_scheduler_t *, uint8_t);
static void portamento_fine(voice_t *, uint8_t);

void reset_arpeggiator(voice_t *voice)
{
    if (voice->arpeggiator_on)
    {
        voice->period = period_for_note(voice->current_note);
        voice->arpeggiator_on = false;
    }
}

bool portamento(const event_t *event)
{
    const effect_t *effects = event->effects;
    for (int i = 0; i < 4; i++, effects++)
    {
        if (effects->command == PORTAMENTO)
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
        else if (effect.command == SET_PANNING)
            set_voice_panning(voice, effect.data);
        else if (effect.command == PATTERN_BREAK)
            break_to_next_position(&player->sequence);
        else if (effect.command == SEQUENCE_JUMP)
            set_jump_target(effect.data, &player->sequence);
        else if (effect.command == SET_TICKS_PER_SECOND)
            set_tempo_fine(&player->tick_scheduler, effect.data);
        else if (effect.command == FINE_PORTAMENTO)
            portamento_fine(voice, effect.data);
        else if (effect.command == ARPEGGIO)
            turn_arpeggiator_on(voice);
    }
}

void handle_effects_off_event(const event_t *event, voice_t *voice)
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

static void turn_arpeggiator_on(voice_t *voice)
{
    voice->arpeggiator_on = true;
    voice->arpeggio_counter = 1;
}

static void arpeggiate(voice_t *voice, const uint8_t data)
{
    const int chord[] = {
            voice->current_note,
            voice->current_note + HIGH_NYBBLE(data),
            voice->current_note + LOW_NYBBLE(data)
    };
    int arpeggio_note = chord[voice->arpeggio_counter % 3];
    if (NOTE_OUT_OF_RANGE(arpeggio_note))
    {
        arpeggio_note = voice->current_note;
    }
    voice->period = period_for_note(arpeggio_note);
    voice->arpeggio_counter += 1;
}

static void set_volume(voice_t *voice, const uint8_t data)
{
    voice->volume = data;
}

static void portamento_fine(voice_t *voice, const uint8_t data)
{
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

static void set_tempo(player_t *player, const uint8_t data)
{
    if (data > 0 && data <= 15)
        player->tick_scheduler.event_scheduler.ticks_per_event = data;
    if (data > 15)
        player_set_bpm(player, data);
}

static void set_tempo_fine(tick_scheduler_t *tick_scheduler, const uint8_t data)
{
    if (data > 0)
        tick_scheduler->audio_accumulator.ticks_per_second = data;
}
