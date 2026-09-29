#include "commands.h"
#include "sequencer.h"
#include "period.h"

static const uint8_t PAN_CENTRE = 0x80;

static bool is_pitch_slide_cmd(command_t);
static bool is_volume_slide_cmd(command_t);
static const effect_t *get_priority_cmd(const event_t *, bool (*is_of_group)(command_t));
static void process_pitch_slide_cmd(const effect_t *, audio_generator_t *, track_command_state_t *);
static void process_volume_slide_cmd(const effect_t *, audio_generator_t *);
static const effect_t *get_track_effect(const event_t *, command_t);
static const effect_t *get_global_effect(const event_t *, int, command_t);
static void process_vibrato_cmd(const event_t *, audio_generator_t *, track_command_state_t *);
static void process_tremolo_cmd(const event_t *, audio_generator_t *, track_command_state_t *);
static void process_set_volume_cmd(const event_t *, audio_generator_t *, track_command_state_t *);
static void process_set_glissando_cmd(const event_t *, audio_generator_t *);
static void process_set_vibrato_waveform_cmd(const event_t *, track_command_state_t *);
static void process_set_tremolo_waveform_cmd(const event_t *, track_command_state_t *);
static void process_arpeggio_cmd(const event_t *, const player_instrument_t *, const player_track_t *, audio_generator_t *);
static void process_set_arpeggio_speed_cmd(const event_t *, track_command_state_t *);
static void process_retrigger_sample_cmd(const event_t *, audio_generator_t *);
static void process_silence_after_delay_cmd(const event_t *, audio_generator_t *);
static void process_advance_phase_cmd(const event_t *, audio_generator_t *);
static void process_clear_repeat_cmd(const event_t *, audio_generator_t *);
static void process_set_stereo_cmd(const event_t *event, audio_channel_t *channel);
static void define_loop(pt_loop_state_t *, sequence_t *, uint8_t);
static void set_tempo(player_t *, uint8_t);
static void pattern_break(sequence_t *, uint8_t);
static void set_tempo_fine(tick_scheduler_t *, uint8_t);
static void delay_next_event(tick_scheduler_t *, uint8_t);

/******************************************************************************
 * Track commands are processed in right-to-left priority order, which means  *
 * that when the same command appears twice or more in the same event, the    *
 * one with the highest index wins; that is, the one furthest to the right    *
 * from the user's point of view.                                             *
 *                                                                            *
 * Some commands are grouped together because they all modify the same state  *
 * variable and there is no practical reason to be using them at the same     *
 * time. They would simply fight each other. For such commands, the one out   *
 * of the group with the highest effect index wins. The groups are:           *
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
 * The other track commands are all orthogonal and may be applied together.   *
 *****************************************************************************/

void process_track_event_commands(const event_t *event, const player_instrument_t *instrument, player_track_t *track)
{
    audio_channel_t *channel = track->audio_channel;
    audio_generator_t *generator = &channel->audio_generator;
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
    process_set_arpeggio_speed_cmd(event, command_state);
    process_retrigger_sample_cmd(event, generator);
    process_silence_after_delay_cmd(event, generator);
    process_advance_phase_cmd(event, generator);
    process_clear_repeat_cmd(event, generator);
    process_set_stereo_cmd(event, channel);
}

static bool is_pitch_slide_cmd(const command_t command)
{
    return command == PORTAMENTO_UP ||
           command == PORTAMENTO_DOWN ||
           command == TONE_PORTAMENTO ||
           command == PORTAMENTO_UP_FINE ||
           command == PORTAMENTO_DOWN_FINE;
}

static bool is_volume_slide_cmd(const command_t command)
{
    return command == VOLUME_SLIDE ||
           command == VOLUME_SLIDE_UP_FINE ||
           command == VOLUME_SLIDE_DOWN_FINE;
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
    if (effect->command == PORTAMENTO_UP || effect->command == PORTAMENTO_UP_FINE)
    {
        const bool fine = effect->command == PORTAMENTO_UP_FINE;
        generator->pitch_slide_on(&generator->state, -effect->data, fine);
    }
    else if (effect->command == PORTAMENTO_DOWN || effect->command == PORTAMENTO_DOWN_FINE)
    {
        const bool fine = effect->command == PORTAMENTO_DOWN_FINE;
        generator->pitch_slide_on(&generator->state, effect->data, fine);
    }
    else
    {
        generator->pitch_slide_off(&generator->state);
    }
    if (effect->command == TONE_PORTAMENTO)
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
        case VOLUME_SLIDE_UP_FINE:
            generator->volume_slide_on(&generator->state, effect->data, true);
            break;
        case VOLUME_SLIDE_DOWN_FINE:
            generator->volume_slide_on(&generator->state, effect->data * -1, true);
            break;
        default:
            generator->volume_slide_off(&generator->state);
            break;
    }
}

static const effect_t *get_track_effect(const event_t *event, const command_t command)
{
    for (int effect_no = MAX_EFFECTS - 1; effect_no >= 0; effect_no--)
    {
        if (event->effects[effect_no].command == command)
            return &event->effects[effect_no];
    }
    return NULL;
}

static const effect_t *get_global_effect(const event_t *events, const int num_tracks, const command_t command)
{
    for (int track_no = num_tracks - 1; track_no >= 0; track_no--)
    {
        const effect_t *effect = get_track_effect(&events[track_no], command);
        if (effect != NULL)
            return effect;
    }
    return NULL;
}

static void process_vibrato_cmd(const event_t *event, audio_generator_t *generator, track_command_state_t *command_state)
{
    const effect_t *effect = get_track_effect(event, VIBRATO);
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
    const effect_t *effect = get_track_effect(event, TREMOLO);
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
    const effect_t *effect = get_track_effect(event, SET_VOLUME);
    if (effect == NULL) return;
    generator->set_volume(&generator->state, effect->data);
    command_state->volume = effect->data;
}

static void process_set_glissando_cmd(const event_t *event, audio_generator_t *generator)
{
    const effect_t *effect = get_track_effect(event, SET_GLISSANDO_MODE);
    if (effect == NULL) return;
    generator->set_glissando(&generator->state, effect->data != 0);
}

static void process_set_vibrato_waveform_cmd(const event_t *event, track_command_state_t *command_state)
{
    const effect_t *effect = get_track_effect(event, SET_VIBRATO_WAVEFORM);
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
    const effect_t *effect = get_track_effect(event, SET_TREMOLO_WAVEFORM);
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
    const effect_t *effect = get_track_effect(event, CHIPTUNE_ARPEGGIO);
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

static void process_set_arpeggio_speed_cmd(const event_t *event, track_command_state_t *command_state)
{
    const effect_t *effect = get_track_effect(event, SET_ARPEGGIO_SPEED);
    if (effect == NULL) return;
    command_state->arpeggio_speed = effect->data;
}

static void process_retrigger_sample_cmd(const event_t *event, audio_generator_t *generator)
{
    const effect_t *effect = get_track_effect(event, RETRIGGER_SAMPLE);
    if (effect == NULL) return;
    generator->retrigger(&generator->state, effect->data);
}

static void process_silence_after_delay_cmd(const event_t *event, audio_generator_t *generator)
{
    const effect_t *effect = get_track_effect(event, SILENCE_SAMPLE_AFTER_DELAY);
    if (effect == NULL) return;
    generator->silence_after_delay(&generator->state, effect->data);
}

static void process_advance_phase_cmd(const event_t *event, audio_generator_t *generator)
{
    const effect_t *effect = NULL;
    if ((effect = get_track_effect(event, ADVANCE_PHASE_FINE)) != NULL)
        generator->advance_phase(&generator->state, effect->data);
    if ((effect = get_track_effect(event, ADVANCE_PHASE)) != NULL)
        generator->advance_phase(&generator->state, (int) effect->data * 256);
}

static void process_clear_repeat_cmd(const event_t *event, audio_generator_t *generator)
{
    const effect_t *effect = get_track_effect(event, RELEASE_SAMPLE_LOOP);
    if (effect == NULL)
        generator->cancel_clear_repeat(&generator->state);
    else
        generator->clear_repeat(&generator->state, effect->data);
}

static void process_set_stereo_cmd(const event_t *event, audio_channel_t *channel)
{
    const effect_t *effect = get_track_effect(event, SET_STEREO);
    if (effect != NULL)
        channel->panning = effect->data == 0 ? PAN_CENTRE : effect->data;
}

bool portamento(const event_t *event)
{
    return get_track_effect(event, TONE_PORTAMENTO) != NULL;
}

uint8_t get_note_delay(const event_t *event)
{
    const effect_t *effect = get_track_effect(event, DELAY_SAMPLE);
    return effect != NULL ? effect->data : 0;
}

uint8_t get_sample_slice(const event_t *event)
{
    const effect_t *effect = get_track_effect(event, USE_SAMPLE_SLICE);
    return effect != NULL ? effect->data : 0;
}

/******************************************************************************
 * Global commands are those that apply to the song as a whole, not to an     *
 * individual track, although they may appear on any track. Global commands   *
 * are processed in right-to-left priority order, which means that when the   *
 * same command appears twice or more in the same pattern line, the one with  *
 * the highest track index wins; that is, the one furthest to the right from  *
 * the user's point of view. If the same global command appears twice or more *
 * in the same track, then the one with the highest effect index wins; once   *
 * again, the one furthest to the right.                                      *
 *****************************************************************************/

void process_global_event_commands(const event_t *events, const int num_tracks, player_t *player)
{
    const effect_t *effect = NULL;
    if ((effect = get_global_effect(events, num_tracks, SET_TEMPO)) != NULL)
        set_tempo(player, effect->data);
    if ((effect = get_global_effect(events, num_tracks, BREAK_TO_NEXT_PATTERN)) != NULL)
        pattern_break(&player->sequence, effect->data);
    if ((effect = get_global_effect(events, num_tracks, SEQUENCE_JUMP)) != NULL)
        sequencer_set_next_sequence_index(effect->data, &player->sequence);
    if ((effect = get_global_effect(events, num_tracks, SET_TICK_RATE)) != NULL)
        set_tempo_fine(&player->tick_scheduler, effect->data);
    if ((effect = get_global_effect(events, num_tracks, DELAY_NEXT_EVENT)) != NULL)
        delay_next_event(&player->tick_scheduler, effect->data);
    if ((effect = get_global_effect(events, num_tracks, DEFINE_PATTERN_LOOP)) != NULL)
        define_loop(&player->loop_state, &player->sequence, effect->data);
}

static void set_tempo(player_t *player, const uint8_t data)
{
    if (data <= 32)
        player->tick_scheduler.event_scheduler.ticks_per_event = data;
    else if (player->module->lines_per_beat > 0)
        player_set_bpm(player, data);
}

static void pattern_break(sequence_t *sequence, const uint8_t data)
{
    sequencer_break_pattern(sequence, data);
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

/******************************************************************************
 * Semantics of the E6 (define pattern loop) command:                         *
 * 1. An E600 command defines the start of a loop.                            *
 * 2. There is only one loop start defined in a pattern at a time, or none.   *
 * 3. An E6xy command where xy > 0 causes the playhead to jump back to the    *
 *    previously defined loop start. The command data xy specifies how many   *
 *    times the jump will occur before the playhead is able to move past the  *
 *    non-zero E6 command location.                                           *
 * 4. A non-zero E6 command has no effect if no loop start has been defined.  *
 * 5. A non-zero E6 command causes the loop start to become undefined after   *
 *    it jumps back for the final time. This means that a subsequent non-zero *
 *    E6 command in the same pattern has no effect unless another zero E6     *
 *    command has defined a new loop start on a pattern line in between.      *
 *    Therefore, multiple loops in the same pattern are possible but they may *
 *    not overlap.                                                            *
 * 6. A loop start is implicitly defined whenever a pattern is entered. This  *
 *    occurs when the sequence advances, or when a pattern break/sequence     *
 *    jump command is executed. The implicit loop start is defined at the     *
 *    line the pattern was entered at.                                        *
 * 7. If multiple E6 commands appear in the same pattern line - whether zero  *
 *    or otherwise - they are prioritised in the same rightmost-command-wins  *
 *    order as the other global commands. Thus, only one E6 command per line  *
 *    will be evaluated.                                                      *
 *****************************************************************************/

static void define_loop(pt_loop_state_t *loop_state, sequence_t *sequence, const uint8_t data)
{
    if (data == 0)
    {
        loop_state->start = sequence->pattern_index;
        loop_state->defined = true;
    }
    else if (loop_state->looping)
    {
        loop_state->counter -= 1;
        if (loop_state->counter == 0)
        {
            sequencer_clear_pattern_loop(sequence);
            loop_state->looping = false;
            loop_state->defined = false;
        }
    }
    else if (loop_state->defined)
    {
        sequencer_set_loop(sequence, loop_state->start, sequence->pattern_index, false);
        loop_state->looping = true;
        loop_state->counter = data;
    }
}
