#include "effects.h"
#include "clock.h"
#include "sequence.h"
#include "period.h"
#include "gain.h"
#include "arctracker.h"

void handle_effects_every_tick(channel_event_t *event, voice_t *voice);

void handle_effects_on_new_event(channel_event_t *event, voice_t *voice);

static void volume_slide_up(voice_t *voice, __uint8_t data);

static void volume_slide_down(voice_t *voice, __uint8_t data);

static void volume_slide_combined(voice_t *voice, __uint8_t data);

static void portamento_up(voice_t *voice, __uint8_t data);

static void portamento_down(voice_t *voice, __uint8_t data);

static void target_portamento(voice_t *voice, __uint8_t data);

static void arpeggiate(voice_t *voice, __uint8_t data);

static void set_volume_tracker(voice_t *voice, __uint8_t data);

static void set_volume_desktop_tracker(voice_t *voice, __uint8_t data);

static void set_tempo(__uint8_t data);

static void set_voice_panning(voice_t *voice, __uint8_t data);

static void set_tempo_fine(__uint8_t data);

static void portamento_fine(voice_t *voice, __uint8_t data);

void process_commands(channel_event_t *event, voice_t *voice, bool on_event)
{
    handle_effects_every_tick(event, voice);
    if (on_event)
        handle_effects_on_new_event(event, voice);
}

void handle_effects_every_tick(channel_event_t *event, voice_t *voice)
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
            target_portamento(voice, effect.data);
        else if (effect.command == ARPEGGIO)
            arpeggiate(voice, effect.data);
    }
}

void handle_effects_on_new_event(channel_event_t *event, voice_t *voice)
{
    for (int i = 0; i < MAX_EFFECTS; i++)
    {
        effect_t effect = event->effects[i];
        if (effect.command == SET_VOLUME_TRACKER)
            set_volume_tracker(voice, effect.data);
        else if (effect.command == SET_VOLUME_DESKTOP_TRACKER)
            set_volume_desktop_tracker(voice, effect.data);
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
    }
}

static inline
void volume_slide_up(voice_t *voice, __uint8_t data)
{
    if ((LOGARITHMIC_GAIN_MAX - voice->gain) > data)
        voice->gain += data;
    else
        voice->gain = LOGARITHMIC_GAIN_MAX;
}

static inline
void volume_slide_down(voice_t *voice, __uint8_t data)
{
    if (voice->gain >= data)
        voice->gain -= data;
    else
        voice->gain = LOGARITHMIC_GAIN_MIN;
}

static inline
void volume_slide_combined(voice_t *voice, __uint8_t data)
{
    __int8_t gain_adjust = data << 1;
    if (gain_adjust > 0)
    {
        if ((LOGARITHMIC_GAIN_MAX - voice->gain) > gain_adjust)
            voice->gain += gain_adjust;
        else
            voice->gain = LOGARITHMIC_GAIN_MAX;
    }
    else if (gain_adjust < LOGARITHMIC_GAIN_MIN)
    {
        if (voice->gain >= gain_adjust)
            voice->gain += gain_adjust; /* is -ve value ! */
        else
            voice->gain = LOGARITHMIC_GAIN_MIN;
    }
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
void target_portamento(voice_t *voice, __uint8_t data)
{
    if (data)
    {
        voice->last_data_byte = data;
    }
    else
    {
        data = voice->last_data_byte;
    }
    if (voice->period < voice->target_period)
    {
        voice->period += data;
        if (voice->period > voice->target_period)
        {
            voice->period = voice->target_period;
        }
    }
    else
    {
        voice->period -= data;
        if (voice->period < voice->target_period)
        {
            voice->period = voice->target_period;
        }
    }
}

static inline
void arpeggiate(voice_t *voice, __uint8_t data)
{
    int temporary_note;
    if (voice->arpeggio_counter == 0)
    {
        temporary_note = voice->note_currently_playing;
    }
    else if (voice->arpeggio_counter == 1)
    {
        temporary_note = voice->note_currently_playing + ((data & 0xf0) >> 4);
        if (0 > temporary_note || temporary_note > 61)
            temporary_note = voice->note_currently_playing;
    }
    else if (voice->arpeggio_counter == 2)
    {
        temporary_note = voice->note_currently_playing + (data & 0xf);
        if (0 > temporary_note || temporary_note > 61)
            temporary_note = voice->note_currently_playing;
    }
    if (++(voice->arpeggio_counter) == 3)
    {
        voice->arpeggio_counter = 0;
    }
    voice->period = period_for_note(temporary_note);
}

static inline
void set_volume_tracker(voice_t *voice, __uint8_t data)
{
    voice->gain = data;
}

static inline
void set_volume_desktop_tracker(voice_t *voice, __uint8_t data)
{
    voice->gain = ((data + 1) << 1) - 1;
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
