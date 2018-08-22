#include "effects.h"
#include "sequence.h"
#include <audio/gain.h>
#include <audio/period.h>
#include <chrono/clock.h>
#include <memory/bits.h>
#include <pcm/mu_law.h>

static const int MAX_EFFECTS = 4;

static void volume_slide_up(voice_t *, __uint8_t);

static void volume_slide_down(voice_t *, __uint8_t);

static void volume_slide_combined(voice_t *, __int8_t);

static void portamento_up(voice_t *, __uint8_t);

static void portamento_down(voice_t *, __uint8_t);

static void start_tone_portamento(voice_t *, __uint8_t);

static void tone_portamento(voice_t *);

static void turn_arpeggiator_on(voice_t *);

static void arpeggiate(voice_t *, __uint8_t);

static void set_volume(voice_t *, __uint8_t);

static void set_tempo(__uint8_t);

static void set_voice_panning(voice_t *, __uint8_t);

static void set_tempo_fine(__uint8_t);

static void portamento_fine(voice_t *, __uint8_t);

inline
void reset_arpeggiator(voice_t *voice)
{
    if (voice->arpeggiator_on)
    {
        voice->period = period_for_note(voice->note_playing);
        voice->arpeggiator_on = false;
    }
}

void handle_effects_on_event(channel_event_t *event, voice_t *voice)
{
    for (int i = 0; i < MAX_EFFECTS; i++)
    {
        effect_t effect = event->effects[i];
        if (effect.command == TONE_PORTAMENTO)
            start_tone_portamento(voice, effect.data);
        else if (effect.command == SET_VOLUME)
            set_volume(voice, effect.data);
        else if (effect.command == SET_TEMPO)
            set_tempo(effect.data);
        else if (effect.command == SET_TRACK_STEREO)
            set_voice_panning(voice, effect.data);
        else if (effect.command == BREAK_PATTERN)
            break_to_next_position();
        else if (effect.command == JUMP_TO_POSITION)
            jump_to_position(effect.data);
        else if (effect.command == SET_TEMPO_FINE)
            set_tempo_fine(effect.data);
        else if (effect.command == PORTAMENTO_FINE)
            portamento_fine(voice, effect.data);
        else if (effect.command == VOLUME_SLIDE_FINE)
            volume_slide_combined(voice, effect.data);
        else if (effect.command == ARPEGGIO)
            turn_arpeggiator_on(voice);
    }
}

void handle_effects_off_event(channel_event_t *event, voice_t *voice)
{
    for (int i = 0; i < MAX_EFFECTS; i++)
    {
        effect_t effect = event->effects[i];
        if (effect.command == VOLUME_SLIDE_UP)
            volume_slide_up(voice, effect.data);
        else if (effect.command == VOLUME_SLIDE_DOWN)
            volume_slide_down(voice, effect.data);
        else if (effect.command == VOLUME_SLIDE)
            volume_slide_combined(voice, effect.data);
        else if (effect.command == PORTAMENTO_UP)
            portamento_up(voice, effect.data);
        else if (effect.command == PORTAMENTO_DOWN)
            portamento_down(voice, effect.data);
        else if (effect.command == TONE_PORTAMENTO)
            tone_portamento(voice);
        else if (effect.command == ARPEGGIO)
            arpeggiate(voice, effect.data);
    }
}

static inline
void volume_slide_up(voice_t *voice, __uint8_t data)
{
    voice->gain += data;
    if (voice->gain > INTERNAL_GAIN_MAX)
        voice->gain = INTERNAL_GAIN_MAX;
}

static inline
void volume_slide_down(voice_t *voice, __uint8_t data)
{
    voice->gain -= data;
    if (voice->gain < INTERNAL_GAIN_MIN)
        voice->gain = INTERNAL_GAIN_MIN;
}

static inline
void volume_slide_combined(voice_t *voice, __int8_t data)
{
    voice->gain += data << 1;
    if (voice->gain > INTERNAL_GAIN_MAX)
        voice->gain = INTERNAL_GAIN_MAX;
    else if (voice->gain < INTERNAL_GAIN_MIN)
        voice->gain = INTERNAL_GAIN_MIN;
}

static inline
void portamento_up(voice_t *voice, __uint8_t data)
{
    voice->period -= data;
    if (voice->period < PERIOD_MIN)
        voice->period = PERIOD_MIN;
}

static inline
void portamento_down(voice_t *voice, __uint8_t data)
{
    voice->period += data;
    if (voice->period > PERIOD_MAX)
        voice->period = PERIOD_MAX;
}

static inline
void start_tone_portamento(voice_t *voice, __uint8_t data)
{
    if (data)
        voice->tone_portamento_speed = data;
}

static inline
void tone_portamento(voice_t *voice)
{
    if (voice->period < voice->tone_portamento_target_period)
    {
        voice->period += voice->tone_portamento_speed;
        if (voice->period > voice->tone_portamento_target_period)
            voice->period = voice->tone_portamento_target_period;
    }
    else
    {
        voice->period -= voice->tone_portamento_speed;
        if (voice->period < voice->tone_portamento_target_period)
            voice->period = voice->tone_portamento_target_period;
    }
}

static inline
void turn_arpeggiator_on(voice_t *voice)
{
    voice->arpeggiator_on = true;
    voice->arpeggio_counter = 1;
}

static inline
void arpeggiate(voice_t *voice, __uint8_t data)
{
    int chord[] = {
            voice->note_playing,
            voice->note_playing + HIGH_NYBBLE(data),
            voice->note_playing + LOW_NYBBLE(data)
    };
    int arpeggio_note = chord[voice->arpeggio_counter % 3];
    if (NOTE_OUT_OF_RANGE(arpeggio_note))
    {
        arpeggio_note = voice->note_playing;
    }
    voice->period = period_for_note(arpeggio_note);
    voice->arpeggio_counter += 1;
}

static inline
void set_volume(voice_t *voice, __uint8_t data)
{
    voice->gain = get_internal_gain(data);
}

static inline
void portamento_fine(voice_t *voice, __uint8_t data)
{
    voice->period += data;
    if (voice->period > PERIOD_MAX)
        voice->period = PERIOD_MAX;
    else if (voice->period < PERIOD_MIN)
        voice->period = PERIOD_MIN;
}

static inline
void set_voice_panning(voice_t *voice, __uint8_t data)
{
    voice->panning = data - 1;
}

static inline
void set_tempo(__uint8_t data)
{
    if (data > 0)
        set_ticks_per_event(data);
}

static inline
void set_tempo_fine(__uint8_t data)
{
    if (data > 0)
        set_ticks_per_second(data);
}
