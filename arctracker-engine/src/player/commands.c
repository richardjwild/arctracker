#include "commands.h"
#include <stdlib.h>
#include "sequencer.h"
#include "period.h"

/******************************************************************************
 * Commands are processed in right-to-left priority order, which means that   *
 * when the same command appears twice or more in the same event, the one     *
 * with the highest index wins; that is, the one furthest to the right from   *
 * the user's point of view.                                                  *
 *                                                                            *
 * Some commands are grouped together because they all modify the same state  *
 * variable(s) and there is no practical reason to be using them at the same  *
 * time: they would simply fight each other. For these commands, the one out  *
 * of the group with the highest index wins. The groups are:                  *
 *                                                                            *
 * Pitch slide command group:                                                 *
 *   - PITCH_SLIDE_UP (0x1)                                                   *
 *   - PITCH_SLIDE_DOWN (0x2)                                                 *
 *   - PORTAMENTO (0x3)                                                       *
 *   - FINE_PORTAMENTO_UP (0xE1)                                              *
 *   - FINE_PORTAMENTO_DOWN (0xE2)                                            *
 *                                                                            *
 * Volume slide command group:                                                *
 *   - VOLUME_SLIDE (0xA)                                                     *
 *   - FINE_CRESCENDO (0xEA)                                                  *
 *   - FINE_DECRESCENDO (0xEB)                                                *
 *                                                                            *
 * The other commands are all orthogonal and may be applied simultaneously.   *
 *****************************************************************************/

static const uint8_t PAN_CENTRE = 0x80;

static bool is_pitch_slide_cmd(command_t);
static bool is_volume_slide_cmd(command_t);
static const effect_t *get_priority_cmd(const event_t *, bool (*is_of_group)(command_t));
static void process_pitch_slide_cmd(const effect_t *, audio_generator_t *, track_command_state_t *);
static void process_volume_slide_cmd(const effect_t *, audio_generator_t *);
static const effect_t *get_effect(const event_t *, command_t);
static void process_vibrato_cmd(const event_t *, audio_generator_t *, track_command_state_t *);
static void process_tremolo_cmd(const event_t *, audio_generator_t *, track_command_state_t *);
static void process_set_volume_cmd(const event_t *, audio_generator_t *, track_command_state_t *);
static void process_set_glissando_cmd(const event_t *, audio_generator_t *);
static void process_set_vibrato_waveform_cmd(const event_t *, track_command_state_t *);
static void process_set_tremolo_waveform_cmd(const event_t *, track_command_state_t *);
static void process_arpeggio_cmd(const event_t *, const player_instrument_t *, const player_track_t *, audio_generator_t *);
static void process_retrigger_sample_cmd(const event_t *, audio_generator_t *);
static void process_silence_after_delay_cmd(const event_t *, audio_generator_t *);
static void define_loop(player_track_t *, sequence_t *, uint8_t);
static void set_tempo(player_t *, uint8_t);
static void set_panning(audio_channel_t *, uint8_t);
static void pattern_break(sequence_t *, uint8_t);
static void set_tempo_fine(tick_scheduler_t *, uint8_t);
static void delay_next_event(tick_scheduler_t *, uint8_t);

void process_instrument_commands(const event_t *event, const player_instrument_t *instrument, player_track_t *track, audio_generator_t *generator)
{
    track_command_state_t *command_state = &track->command_state;
    process_pitch_slide_cmd(get_priority_cmd(event, is_pitch_slide_cmd), generator, command_state);
    process_volume_slide_cmd(get_priority_cmd(event, is_volume_slide_cmd), generator);
    process_vibrato_cmd(event, generator, command_state);
    process_tremolo_cmd(event, generator, command_state);
    process_set_volume_cmd(event, generator, command_state);
    process_set_glissando_cmd(event, generator);
    process_set_vibrato_waveform_cmd(event, command_state);
    process_set_tremolo_waveform_cmd(event, command_state);
    process_arpeggio_cmd(event, instrument, track, generator);
    process_retrigger_sample_cmd(event, generator);
    process_silence_after_delay_cmd(event, generator);
}

static bool is_pitch_slide_cmd(const command_t command)
{
    return command == PITCH_SLIDE_UP ||
           command == PITCH_SLIDE_DOWN ||
           command == PORTAMENTO ||
           command == FINE_PORTAMENTO_UP ||
           command == FINE_PORTAMENTO_DOWN;
}

static bool is_volume_slide_cmd(const command_t command)
{
    return command == VOLUME_SLIDE ||
           command == FINE_CRESCENDO ||
           command == FINE_DECRESCENDO;
}

static const effect_t *get_priority_cmd(const event_t *event, bool (*is_of_group)(command_t))
{
    for (int effect_no = MAX_EFFECTS - 1; effect_no >= 0; effect_no--)
    {
        const effect_t *effect = &event->effects[effect_no];
        if (is_of_group(effect->command))
            return effect;
    }
    return NULL;
}

static void process_pitch_slide_cmd(const effect_t *effect, audio_generator_t *generator, track_command_state_t *command_state)
{
    if (effect == NULL)
    {
        generator->pitch_slide_off(&generator->state);
        generator->tone_portamento_off(&generator->state);
        return;
    }
    if (effect->command == PITCH_SLIDE_UP || effect->command == FINE_PORTAMENTO_UP)
    {
        const bool fine = effect->command == FINE_PORTAMENTO_UP;
        generator->pitch_slide_on(&generator->state, -effect->data, fine);
    }
    else if (effect->command == PITCH_SLIDE_DOWN || effect->command == FINE_PORTAMENTO_DOWN)
    {
        const bool fine = effect->command == FINE_PORTAMENTO_DOWN;
        generator->pitch_slide_on(&generator->state, effect->data, fine);
    }
    else
    {
        generator->pitch_slide_off(&generator->state);
    }
    if (effect->command == PORTAMENTO)
    {
        int slide_rate = effect->data;
        if (slide_rate == 0) slide_rate = command_state->effect_memory.tone_portamento_speed;
        else command_state->effect_memory.tone_portamento_speed = slide_rate;
        generator->tone_portamento_on(&generator->state, slide_rate);
    }
    else
    {
        generator->tone_portamento_off(&generator->state);
    }
}

static void process_volume_slide_cmd(const effect_t *effect, audio_generator_t *generator)
{
    if (effect == NULL)
    {
        generator->volume_slide_off(&generator->state);
        return;
    }
    switch (effect->command)
    {
        case VOLUME_SLIDE:
        {
            int slide_rate = effect->data &0x7f;
            if ((effect->data & 0x80) == 0) slide_rate *= -1;
            generator->volume_slide_on(&generator->state, slide_rate, false);
            break;
        }
        case FINE_CRESCENDO:
            generator->volume_slide_on(&generator->state, effect->data, true);
            break;
        case FINE_DECRESCENDO:
            generator->volume_slide_on(&generator->state, effect->data * -1, true);
            break;
        default:
            generator->volume_slide_off(&generator->state);
            break;
    }
}

static const effect_t *get_effect(const event_t *event, const command_t command)
{
    for (int effect_no = MAX_EFFECTS - 1; effect_no >= 0; effect_no--)
    {
        if (event->effects[effect_no].command == command)
            return &event->effects[effect_no];
    }
    return NULL;
}

static void process_vibrato_cmd(const event_t *event, audio_generator_t *generator, track_command_state_t *command_state)
{
    const effect_t *effect = get_effect(event, VIBRATO);
    if (effect == NULL)
    {
        generator->vibrato_off(&generator->state);
        return;
    }
    int rate = effect->data >> 4;
    if (rate == 0)
    {
        rate = command_state->effect_memory.vibrato_speed;
    }
    else
    {
        command_state->effect_memory.vibrato_speed = rate;
    }
    int depth = effect->data & 0xf;
    if (depth == 0)
    {
        depth = command_state->effect_memory.vibrato_depth;
    }
    else
    {
        command_state->effect_memory.vibrato_depth = depth;
    }
    const bool retrigger = command_state->vibrato_retrigger;
    const pt_waveform_t waveform = command_state->vibrato_waveform;
    generator->vibrato_on(&generator->state, rate, depth, waveform, retrigger);
}

static void process_tremolo_cmd(const event_t *event, audio_generator_t *generator, track_command_state_t *command_state)
{
    const effect_t *effect = get_effect(event, TREMOLO);
    if (effect == NULL)
    {
        generator->tremolo_off(&generator->state);
        return;
    }
    int rate = effect->data >> 4;
    if (rate == 0)
    {
        rate = command_state->effect_memory.tremolo_speed;
    }
    else
    {
        command_state->effect_memory.tremolo_speed = rate;
    }
    int depth = effect->data & 0xf;
    if (depth == 0)
    {
        depth = command_state->effect_memory.tremolo_depth;
    }
    else
    {
        command_state->effect_memory.tremolo_depth = depth;
    }
    const bool retrigger = command_state->tremolo_retrigger;
    const pt_waveform_t waveform = command_state->tremolo_waveform;
    generator->tremolo_on(&generator->state, rate, depth, waveform, retrigger);
}

static void process_set_volume_cmd(const event_t *event, audio_generator_t *generator, track_command_state_t *command_state)
{
    const effect_t *effect = get_effect(event, SET_VOLUME);
    if (effect == NULL) return;
    generator->set_volume(&generator->state, effect->data);
    command_state->volume = effect->data;
}

static void process_set_glissando_cmd(const event_t *event, audio_generator_t *generator)
{
    const effect_t *effect = get_effect(event, SET_GLISSANDO_MODE);
    if (effect == NULL) return;
    generator->set_glissando(&generator->state, effect->data != 0);
}

static void process_set_vibrato_waveform_cmd(const event_t *event, track_command_state_t *command_state)
{
    const effect_t *effect = get_effect(event, SET_VIBRATO_WAVEFORM);
    if (effect == NULL) return;
    const int waveform_select = effect->data & 0x3;
    pt_waveform_t vibrato_waveform = PT_WAVEFORM_SQUARE;
    if (waveform_select == 0) vibrato_waveform = PT_WAVEFORM_SINE;
    if (waveform_select == 1) vibrato_waveform = PT_WAVEFORM_RAMP;
    command_state->vibrato_waveform = vibrato_waveform;
    command_state->vibrato_retrigger = (effect->data & 4) == 0;
}

static void process_set_tremolo_waveform_cmd(const event_t *event, track_command_state_t *command_state)
{
    const effect_t *effect = get_effect(event, SET_TREMOLO_WAVEFORM);
    if (effect == NULL) return;
    const int waveform_select = effect->data & 0x3;
    pt_waveform_t tremolo_waveform = PT_WAVEFORM_SQUARE;
    if (waveform_select == 0) tremolo_waveform = PT_WAVEFORM_SINE;
    if (waveform_select == 1) tremolo_waveform = PT_WAVEFORM_RAMP;
    command_state->tremolo_waveform = tremolo_waveform;
    command_state->tremolo_retrigger = (effect->data & 4) == 0;
}

static void process_arpeggio_cmd(const event_t *event, const player_instrument_t *instrument, const player_track_t *track, audio_generator_t *generator)
{
    const effect_t *effect = get_effect(event, ARPEGGIO);
    if (effect == NULL)
    {
        generator->arpeggio_off(&generator->state);
        return;
    }
    const int root_note = event->note == 0 ? track->current_note : event->note - 1;
    const int interval_1 = effect->data >> 4;
    const int interval_2 = effect->data & 0xf;
    const track_command_state_t *command_state = &track->command_state;
    generator->arpeggio_on(&generator->state, root_note + instrument->transpose, interval_1, interval_2, command_state->arpeggio_speed);
}

static void process_retrigger_sample_cmd(const event_t *event, audio_generator_t *generator)
{
    const effect_t *effect = get_effect(event, RETRIGGER_SAMPLE);
    if (effect == NULL) return;
    generator->retrigger(&generator->state, effect->data);
}

static void process_silence_after_delay_cmd(const event_t *event, audio_generator_t *generator)
{
    const effect_t *effect = get_effect(event, SILENCE_SAMPLE_AFTER_DELAY);
    if (effect == NULL) return;
    generator->silence_after_delay(&generator->state, effect->data);
}

bool portamento(const event_t *event)
{
    return get_effect(event, PORTAMENTO) != NULL;
}

void handle_effects_before_note(const event_t *event, const player_track_t *track, player_t *player)
{
    const effect_t *effect = get_effect(event, SET_FINETUNE);
    if (effect != NULL)
        player_set_sample_finetune(player, track->instrument_no, (int8_t) effect->data);
}

uint8_t get_note_delay(const event_t *event)
{
    const effect_t *effect = get_effect(event, DELAY_SAMPLE);
    return effect != NULL ? effect->data : 0;
}

uint8_t get_sample_slice(const event_t *event)
{
    const effect_t *effect = get_effect(event, USE_SAMPLE_SLICE);
    return effect != NULL ? effect->data : 0;
}

void process_non_instrument_commands(const event_t *event, audio_channel_t *channel, player_track_t *track, player_t *player)
{
    const effect_t *effect = NULL;
    if ((effect = get_effect(event, SET_TEMPO)) != NULL)
        set_tempo(player, effect->data);
    if ((effect = get_effect(event, SET_PANNING)) != NULL)
        set_panning(channel, effect->data);
    if ((effect = get_effect(event, PATTERN_BREAK)) != NULL)
        pattern_break(&player->sequence, effect->data);
    if ((effect = get_effect(event, SEQUENCE_JUMP)) != NULL)
        set_jump_target(effect->data, 0, &player->sequence);
    if ((effect = get_effect(event, SET_TICKS_PER_SECOND)) != NULL)
        set_tempo_fine(&player->tick_scheduler, effect->data);
    if ((effect = get_effect(event, DELAY_NEXT_EVENT)) != NULL)
        delay_next_event(&player->tick_scheduler, effect->data);
    if ((effect = get_effect(event, SET_LOOP)) != NULL)
        define_loop(track, &player->sequence, effect->data);
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

static void set_panning(audio_channel_t *channel, const uint8_t data)
{
    channel->panning = data == 0 ? PAN_CENTRE : data;
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
