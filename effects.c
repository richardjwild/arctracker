#include "effects.h"
#include "clock.h"
#include "sequence.h"
#include "period.h"
#include "arctracker.h"

void handle_effects_every_tick(channel_event_t *event, voice_t *voice);

void handle_effects_on_new_event(channel_event_t *event, voice_t *voice);

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
        {
            if ((255 - voice->gain) > effect.data)
                voice->gain += effect.data;
            else
                voice->gain = 255;
        }
        else if (effect.command == VOLUME_SLIDE_DOWN)
        {
            if (voice->gain >= effect.data)
                voice->gain -= effect.data;
            else
                voice->gain = 0;
        }
        else if (effect.command == VOLUME_SLIDE)
        {
            __int8_t gain_adjust = effect.data << 1;
            if (gain_adjust > 0)
            {
                if ((255 - voice->gain) > gain_adjust)
                    voice->gain += gain_adjust;
                else
                    voice->gain = 255;
            }
            else if (gain_adjust < 0)
            {
                if (voice->gain >= gain_adjust)
                    voice->gain += gain_adjust; /* is -ve value ! */
                else
                    voice->gain = 0;
            }
        }
        else if (effect.command == PORTAMENTO_UP)
        {
            voice->period -= effect.data;
            if (voice->period < PERIOD_MIN)
                voice->period = PERIOD_MIN;
        }
        else if (effect.command == PORTAMENTO_DOWN)
        {
            voice->period += effect.data;
            if (voice->period > PERIOD_MAX)
                voice->period = PERIOD_MAX;
        }
        else if (effect.command == TONE_PORTAMENTO)
        {
            if (effect.data)
            {
                voice->last_data_byte = effect.data;
            }
            else
            {
                effect.data = voice->last_data_byte;
            }
            if (voice->period < voice->target_period)
            {
                voice->period += effect.data;
                if (voice->period > voice->target_period)
                {
                    voice->period = voice->target_period;
                }
            }
            else
            {
                voice->period -= effect.data;
                if (voice->period < voice->target_period)
                {
                    voice->period = voice->target_period;
                }
            }
        }
        else if (effect.command == ARPEGGIO)
        {
            int temporary_note;
            if (voice->arpeggio_counter == 0)
                temporary_note = voice->note_currently_playing;
            else if (voice->arpeggio_counter == 1)
            {
                temporary_note = voice->note_currently_playing +
                                 ((effect.data & 0xf0) >> 4);

                if (0 > temporary_note || temporary_note > 61)
                    temporary_note = voice->note_currently_playing;
            }
            else if (voice->arpeggio_counter == 2)
            {
                temporary_note = voice->note_currently_playing +
                                 (effect.data & 0xf);

                if (0 > temporary_note || temporary_note > 61)
                    temporary_note = voice->note_currently_playing;
            }

            if (++(voice->arpeggio_counter) == 3)
                voice->arpeggio_counter = 0;

            voice->period = period_for_note(temporary_note);
        }
    }
}

void handle_effects_on_new_event(channel_event_t *event, voice_t *voice)
{
    for (int i = 0; i < MAX_EFFECTS; i++)
    {
        effect_t effect = event->effects[i];
        if (effect.command == SET_VOLUME_TRACKER)
        {
            voice->gain = effect.data;
        }
        else if (effect.command == SET_VOLUME_DESKTOP_TRACKER)
        {
            voice->gain = ((effect.data + 1) << 1) - 1;
        }
        else if (effect.command == SET_TEMPO)
        {
            if (effect.data > 0)
                set_ticks_per_event(effect.data);
        }
        else if (effect.command == SET_TRACK_STEREO)
        {
            voice->panning = effect.data - 1;
        }
        else if (effect.command == BREAK_PATTERN)
        {
            break_to_next_position();
        }
        else if (effect.command == JUMP_TO_POSITION)
        {
            jump_to_position(effect.data);
        }
        else if (effect.command == SET_TEMPO_FINE)
        {
            if (effect.data > 0)
                set_ticks_per_second(effect.data);
        }
        else if (effect.command == PORTAMENTO_FINE)
        {
            voice->period += effect.data;
            if (voice->period > PERIOD_MAX)
                voice->period = PERIOD_MAX;
            else if (voice->period < PERIOD_MIN)
                voice->period = PERIOD_MIN;
        }
        else if (effect.command == VOLUME_SLIDE_FINE)
        {
            __int8_t gain_adjust = effect.data << 1;
            if (gain_adjust > 0)
            {
                if ((255 - voice->gain) > gain_adjust)
                    voice->gain += gain_adjust;
                else
                    voice->gain = 255;
            }
            else if (gain_adjust < 0)
            {
                if (voice->gain >= gain_adjust)
                    voice->gain += gain_adjust; /* is -ve value ! */
                else
                    voice->gain = 0;
            }
        }
    }
}
