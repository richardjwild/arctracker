#include "effects.h"
#include "clock.h"
#include "sequence.h"
#include "period.h"

void process_commands(channel_event_t *event, voice_t *voice, bool on_event)
{
    int bar;

    for (int effect_no = 0; effect_no < 4; effect_no++)
    {
        effect_t effect = event->effects[effect_no];
        switch (effect.command)
        {
            case SET_VOLUME_TRACKER:
                if (on_event)
                    voice->gain = effect.data;
                break;

            case SET_VOLUME_DESKTOP_TRACKER:
                if (on_event)
                    voice->gain = ((effect.data + 1) << 1) - 1;
                break;

            case SET_TEMPO:
                if (on_event && effect.data) /* ensure an "S00" command does not hang player! */
                    set_ticks_per_event(effect.data);
                break;

            case SET_TRACK_STEREO:
                if (on_event)
                {
                    voice->panning = effect.data - 1;
                }
                break;

            case VOLUME_SLIDE_UP:
                if ((255 - voice->gain) > effect.data)
                    voice->gain += effect.data;
                else
                    voice->gain = 255;
                break;

            case VOLUME_SLIDE_DOWN:
                if (voice->gain >= effect.data)
                    voice->gain -= effect.data;
                else
                    voice->gain = 0;
                break;

            case VOLUME_SLIDE:
                bar = (signed char) effect.data << 1;
                if (bar > 0)
                {
                    if ((255 - voice->gain) > bar)
                        voice->gain += bar;
                    else
                        voice->gain = 255;
                }
                else if (bar < 0)
                {
                    if (voice->gain >= bar)
                        voice->gain += bar; /* is -ve value ! */
                    else
                        voice->gain = 0;
                }
                break;

            case PORTAMENTO_UP:
                voice->period -= effect.data;
                if (voice->period < 0x50)
                    voice->period = 0x50;
                break;

            case PORTAMENTO_DOWN:
                voice->period += effect.data;
                if (voice->period > 0x3f0)
                    voice->period = 0x3f0;
                break;

            case TONE_PORTAMENTO:
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
                break;

            case ARPEGGIO:
                if (effect.data)
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
                break;

            case BREAK_PATTERN:
                if (on_event)
                    break_to_next_position();
                break;

            case JUMP_TO_POSITION:
                if (on_event)
                    jump_to_position(effect.data);
                break;

            case SET_TEMPO_FINE:
                if (on_event && effect.data)
                    set_ticks_per_second(effect.data);
                break;

            case PORTAMENTO_FINE:
                if (on_event && effect.data)
                {
                    bar = (unsigned char) effect.data;
                    voice->period += bar;
                    if (bar > 0)
                    {
                        if (voice->period > 0x3f0)
                        {
                            voice->period = 0x3f0;
                        }
                    }
                    else
                    {
                        if (voice->period < 0x50)
                        {
                            voice->period = 0x50;
                        }
                    }
                }
                break;

            case VOLUME_SLIDE_FINE:
                if (on_event && effect.data)
                {
                    bar = (signed char) effect.data << 1;
                    if (bar > 0)
                    {
                        if ((255 - voice->gain) > bar)
                        {
                            voice->gain += bar;
                        }
                        else
                        {
                            voice->gain = 255;
                        }
                    }
                    else if (bar < 0)
                    {
                        if (voice->gain >= bar)
                        {
                            voice->gain += bar; /* is -ve value ! */
                        }
                        else
                        {
                            voice->gain = 0;
                        }
                    }
                }
                break;
        }
    }
}
