#include <string.h>
#include <stdio.h>
#include "module.h"
#include <math.h>
#include "messages.h"
#include "tempo.h"
#include "io/error.h"
#include "memory/bits.h"
#include "memory/heap.h"

#define INITIAL_SEQUENCE_CAPACITY 1024
#define INITIAL_PATTERN_CAPACITY 1024
#define INITIAL_SAMPLE_CAPACITY 1024
#define INITIAL_PATTERN_LINE_CAPACITY 64
#define DEFAULT_TICKS_PER_SECOND 50
#define MAX_BPM 255

static bool ensure_sequence_capacity(module_t *, int);
static bool ensure_pattern_capacity(module_t *, int);
static bool ensure_pattern_line_capacity(pattern_t *, uint32_t, int);
static void clear_added_events(event_t *, int, int, uint32_t);
static bool ensure_sample_capacity(module_t *module, int required_samples);
static bool resize_pattern(pattern_t, event_t **, uint32_t, uint32_t);
static track_t *resize_tracks(const track_t *, uint32_t, uint32_t);
static void destroy_sample(sample_t *);
static void destroy_pattern(pattern_t *);

module_t *module_create(const int num_tracks, const int sequence_len, const int num_patterns, const int num_samples)
{
    module_t *module = allocate_array(MODULE, 1, sizeof(module_t));
    if (module == NULL)
        goto fail;
    module->num_tracks = num_tracks;
    module->track_capacity = round_up_to_power_of_two(module->num_tracks);
    module->sequence_length = sequence_len;
    module->sequence_capacity = sequence_len > INITIAL_SEQUENCE_CAPACITY ? sequence_len : INITIAL_SEQUENCE_CAPACITY;
    module->num_patterns = num_patterns;
    module->pattern_capacity = num_patterns > INITIAL_PATTERN_CAPACITY ? num_patterns : INITIAL_PATTERN_CAPACITY;
    module->sample_slots = num_samples;
    module->sample_capacity = num_samples > INITIAL_SAMPLE_CAPACITY ? num_samples : INITIAL_SAMPLE_CAPACITY;
    module->sequence = allocate_array(MODULE, module->sequence_capacity, sizeof(int));
    if (module->sequence == NULL)
        goto fail;
    module->tracks = allocate_array(MODULE, (int) module->track_capacity, sizeof(track_t));
    if (module->tracks == NULL)
        goto fail;
    module->patterns = allocate_array(MODULE, module->pattern_capacity, sizeof(pattern_t));
    if (module->patterns == NULL)
        goto fail;
    module->samples = allocate_array(MODULE, module->sample_capacity, sizeof(sample_t));
    if (module->samples == NULL)
        goto fail;
    module->tempo_lookup = allocate_array(MODULE, 256, sizeof(tempo_t));
    if (module->tempo_lookup == NULL)
        goto fail;
    return module;
fail:
    if (module != NULL) module_destroy(module);
    return NULL;
}

bool module_init(module_t *module)
{
    module->initial_ticks_per_event = 6;
    module->lines_per_beat = 4;
    module->initial_bpm = 120;
    module->master_gain = 0.25f;
    for (int i = 0; i < module->num_tracks; i++)
    {
        module->tracks[i].panning = 0x80; // Centre
        module->tracks[i].effects_displayed = 1;
        module->tracks[i].muted = false;
    }
    for (int i = 0; i < NUM_INSTRUMENT_SLOTS; i++)
        module->instruments[i].assigned = false;
    if (!module_create_pattern(module, 0, 64))
        return false;
    return true;
}

tempo_t module_get_initial_tempo(const module_t *module)
{
    if (module->initial_bpm > 0 && module->lines_per_beat > 0)
    {
        return module->tempo_lookup[module->initial_bpm];
    }
    return (tempo_t) {
        .ticks_per_event = module->initial_ticks_per_event,
        .ticks_per_second = DEFAULT_TICKS_PER_SECOND,
        .actual_bpm = 0.0f,
    };
}

bool module_set_sequence(module_t *module, const int *new_sequence, const int new_sequence_len)
{
    if (!ensure_sequence_capacity(module, new_sequence_len))
        return false;
    memcpy(module->sequence, new_sequence, new_sequence_len * sizeof(int));
    module->sequence_length = new_sequence_len;
    return true;
}

static bool ensure_sequence_capacity(module_t *module, const int required_sequence_len)
{
    if (module->sequence_capacity >= required_sequence_len)
        return true;
    int required_capacity = module->sequence_capacity;
    while (required_capacity < required_sequence_len)
        required_capacity += INITIAL_SEQUENCE_CAPACITY;
    int *new_sequence = reallocate_array(MODULE, module->sequence, required_capacity, sizeof(int));
    if (new_sequence == NULL)
        return false;
    module->sequence = new_sequence;
    module->sequence_capacity = required_capacity;
    return true;
}

bool module_create_pattern(module_t *module, const int pattern_no, const int num_lines)
{
    if (pattern_no < module->pattern_capacity && module->patterns[pattern_no].events != NULL)
    {
        error(PATTERN_ALREADY_EXISTS);
        return false;
    }
    if (!ensure_pattern_capacity(module, pattern_no + 1))
        return false;
    const int line_capacity = num_lines > INITIAL_PATTERN_LINE_CAPACITY ? num_lines : INITIAL_PATTERN_LINE_CAPACITY;
    const pattern_t pattern = (pattern_t) {
        .num_lines = num_lines,
        .line_capacity = line_capacity,
        .events = allocate_array(MODULE, line_capacity * (int) module->track_capacity, sizeof(event_t)),
    };
    if (pattern.events == NULL)
        return false;
    module->patterns[pattern_no] = pattern;
    if (pattern_no >= module->num_patterns)
        module->num_patterns = pattern_no + 1;
    return true;
}

void module_delete_pattern(module_t *module, const int pattern_no)
{
    if (pattern_no != module->num_patterns - 1)
    {
        // TODO: Make this capable of deleting a pattern from the middle of the array.
        return;
    }
    deallocate(MODULE, module->patterns[pattern_no].events);
    module->patterns[pattern_no].num_lines = 0;
    module->patterns[pattern_no].events = NULL;
    module->num_patterns = pattern_no;
}

static bool ensure_pattern_capacity(module_t *module, const int required_patterns)
{
    if (module->pattern_capacity >= required_patterns)
        return true;
    int required_capacity = module->pattern_capacity;
    while (required_capacity < required_patterns)
        required_capacity += INITIAL_PATTERN_CAPACITY;
    pattern_t *new_patterns = reallocate_array(MODULE, module->patterns, required_capacity, sizeof(pattern_t));
    if (new_patterns == NULL)
        return false;
    module->patterns = new_patterns;
    module->pattern_capacity = required_capacity;
    return true;
}

bool module_set_pattern_length(const module_t *module, const int pattern_no, const int new_length)
{
    pattern_t pattern = module->patterns[pattern_no];
    if (!ensure_pattern_line_capacity(&pattern, module->track_capacity, new_length)) return false;
    pattern.num_lines = new_length;
    module->patterns[pattern_no] = pattern;
    return true;
}

static bool ensure_pattern_line_capacity(pattern_t *pattern, const uint32_t track_capacity, const int required_lines)
{
    if (pattern->line_capacity >= required_lines)
        return true;
    event_t *new_events = reallocate_array(MODULE, pattern->events, required_lines * track_capacity, sizeof(event_t));
    if (new_events == NULL) return false;
    pattern->events = new_events;
    clear_added_events(pattern->events, pattern->line_capacity, required_lines, track_capacity);
    pattern->line_capacity = required_lines;
    return true;
}

static void clear_added_events(event_t *events, const int old_capacity, const int new_capacity, const uint32_t track_capacity)
{
    const int added_events_start = old_capacity * track_capacity;
    const int added_events_end = new_capacity * track_capacity - 1;
    for (int i = added_events_start; i <= added_events_end; i++)
        events[i] = (event_t) {0};
}

void module_get_info(module_t *module, ui_module_info_t *module_info)
{
    snprintf(module_info->name, sizeof module_info->name, "%s", module->name);
    snprintf(module_info->author, sizeof module_info->author, "%s", module->author);
    module_info->num_tracks = module->num_tracks;
    module_info->tune_length = module->sequence_length;
    module_info->num_patterns = module->num_patterns;
    module_info->master_gain = module->master_gain;
    module_info->lines_per_beat = module->lines_per_beat;
    module_info->initial_bpm = module->initial_bpm;
}

void module_get_instrument_info(const module_t *module, const int instrument_index, ui_instrument_info_t *instrument_info)
{
    instrument_t instrument = module->instruments[instrument_index];
    instrument_info->assigned = instrument.assigned;
    if (instrument.assigned)
    {
        const sample_t sample = module->samples[instrument.sample_index];
        snprintf(instrument_info->name, sizeof instrument_info->name, "%s", instrument.name);
        instrument_info->default_volume = instrument.default_volume;
        instrument_info->transpose = instrument.transpose;
        instrument_info->sample_info.sample_index = instrument.sample_index;
        instrument_info->sample_info.sample_length = sample.sample_length;
        instrument_info->repeats = instrument.repeats;
        instrument_info->repeat_offset = instrument.repeat_offset;
        instrument_info->repeat_length = instrument.repeat_length;
    }
    else
    {
        instrument_info->name[0] = '\0';
        instrument_info->default_volume = 255;
        instrument_info->transpose = 13;
        instrument_info->sample_info.sample_index = 0;
        instrument_info->sample_info.sample_length = 0;
        instrument_info->repeats = false;
        instrument_info->repeat_offset = 0;
        instrument_info->repeat_length = 0;
    }
}

void module_set_instrument(module_t *module, const int instrument_index, const instrument_t instrument_update)
{
    module->instruments[instrument_index] = instrument_update;
}

bool module_link_sample(module_t *module, const float *sample_data, const int sample_length, int *sample_index)
{
    bool found_empty_slot = false;
    int new_sample_index = 0;
    for (int i = 0; i < module->sample_slots; i++)
    {
        if (module->samples[i].sample_length == 0)
        {
            new_sample_index = i;
            found_empty_slot = true;
            break;
        }
    }
    if (!found_empty_slot)
    {
        new_sample_index = module->sample_slots;
        if (!ensure_sample_capacity(module, new_sample_index + 1))
            return false;
        module->sample_slots++;
    }
    module->samples[new_sample_index] = (sample_t) {
        .sample_data = sample_data,
        .sample_length = sample_length,
    };
    *sample_index = new_sample_index;
    return true;
}

bool module_set_sample(module_t *module, const float *sample_data, const int sample_length, const int sample_index)
{
    if (!ensure_sample_capacity(module, sample_index))
        return false;
    module->samples[sample_index] = (sample_t) {
        .sample_data = sample_data,
        .sample_length = sample_length,
    };
    if (sample_index >= module->sample_slots)
        module->sample_slots = sample_index + 1;
    return true;
}

static bool ensure_sample_capacity(module_t *module, const int required_samples)
{
    if (module->sample_capacity >= required_samples)
        return true;
    int required_capacity = module->sample_capacity;
    while (required_capacity < required_samples)
        required_capacity += INITIAL_SAMPLE_CAPACITY;
    sample_t *new_samples = reallocate_array(MODULE, module->samples, required_capacity, sizeof(sample_t));
    if (new_samples == NULL)
        return false;
    for (int sample = module->sample_slots; sample < required_capacity; sample++)
        module->samples[sample] = (sample_t) {0};
    module->samples = new_samples;
    module->sample_capacity = required_capacity;
    return true;
}

void module_set_name(module_t *module, const char *name)
{
    snprintf(module->name, sizeof module->name, "%s", name);
}

void module_set_author(module_t *module, const char *author)
{
    snprintf(module->author, sizeof module->author, "%s", author);
}

bool module_adjust_track_capacity(module_t *module, const uint32_t new_track_capacity)
{
    const uint32_t old_track_capacity = module->track_capacity;
    track_t *resized_tracks = NULL;
    event_t **resized_patterns = NULL;
    resized_patterns = allocate_array(MODULE, module->num_patterns, sizeof(event_t *));
    if (resized_patterns == NULL) goto track_capacity_adjustment_failed;
    for (int pno = 0; pno < module->num_patterns; pno++)
    {
        const pattern_t pattern = module->patterns[pno];
        event_t *resized_pattern_data;
        if (!resize_pattern(pattern, &resized_pattern_data, old_track_capacity, new_track_capacity))
            goto track_capacity_adjustment_failed;
        resized_patterns[pno] = resized_pattern_data;
    }
    resized_tracks = resize_tracks(module->tracks, old_track_capacity, new_track_capacity);
    if (resized_tracks == NULL) goto track_capacity_adjustment_failed;
    //
    // If we reach here, we are good and can now commit the changes.
    //
    for (int p = 0; p < module->num_patterns; p++)
    {
        pattern_t pattern = module->patterns[p];
        deallocate(MODULE, pattern.events);
        pattern.events = resized_patterns[p];
        module->patterns[p] = pattern;
    }
    deallocate(MODULE, module->tracks);
    module->tracks = resized_tracks;
    module->track_capacity = new_track_capacity;
    deallocate(MODULE, resized_patterns);
    return true;

track_capacity_adjustment_failed:
    deallocate(MODULE, resized_tracks);
    if (resized_patterns == NULL) return false;
    for (int p = 0; p < module->num_patterns; p++)
        deallocate(MODULE, resized_patterns[p]);
    deallocate(MODULE, resized_patterns);
    return false;
}

static bool resize_pattern(const pattern_t pattern, event_t **resized_pattern_data, const uint32_t old_track_capacity, const uint32_t new_track_capacity)
{
    const int new_pattern_event_capacity = pattern.line_capacity * (int) new_track_capacity;
    event_t *resized_data = allocate_array(MODULE, new_pattern_event_capacity, sizeof(event_t));
    if (resized_data == NULL)
        return false;
    for (int line = 0; line < pattern.num_lines; line++) {
        const event_t *src = &pattern.events[line * old_track_capacity];
        event_t *dest = &resized_data[line * new_track_capacity];
        memcpy(dest, src, old_track_capacity * sizeof(event_t));
    }
    *resized_pattern_data = resized_data;
    return true;
}

static track_t *resize_tracks(const track_t *tracks, const uint32_t old_track_capacity, const uint32_t new_track_capacity)
{
    track_t *resized_tracks = allocate_array(MODULE, (int) new_track_capacity, sizeof(track_t));
    if (resized_tracks == NULL)
        return NULL;
    for (uint32_t tno = 0; tno < old_track_capacity; tno++)
        resized_tracks[tno] = tracks[tno];
    return resized_tracks;
}

void module_set_num_tracks(module_t *module, const uint32_t num_tracks)
{
    const uint32_t old_num_tracks = module->num_tracks;
    module->num_tracks = (int) num_tracks;
    if (num_tracks <= old_num_tracks) return;
    for (uint32_t tno = old_num_tracks; tno < num_tracks; tno++)
        module->tracks[tno] = (track_t) {
            .muted = false,
            .panning = 0x80,
            .effects_displayed = 1,
        };
    for (int pno = 0; pno < module->num_patterns; pno++)
    {
        const pattern_t pattern = module->patterns[pno];
        const int num_lines = pattern.num_lines;
        event_t *events = pattern.events;
        for (int line = 0; line < num_lines; line++)
            for (uint32_t tno = old_num_tracks; tno < num_tracks; tno++)
                events[line * module->track_capacity + tno] = (event_t) {0};
    }
}

void module_toggle_mute_state(const module_t *module, const int track)
{
    const bool current_state = module->tracks[track].muted;
    module->tracks[track].muted = !current_state;
}

void module_set_effects_displayed(const module_t *module, const int track, const int effects_displayed)
{
    module->tracks[track].effects_displayed = effects_displayed;
}

void module_set_lines_per_beat(module_t *module, const uint8_t lines_per_beat)
{
    module->lines_per_beat = lines_per_beat;
    for (int bpm = 0; bpm <= MAX_BPM; bpm++) module->tempo_lookup[bpm] = (tempo_t) {0};
    if (module->lines_per_beat > 0)
    {
        calculate_tempo(module->tempo_lookup, lines_per_beat, MAX_BPM);
    }
}

void module_set_initial_bpm(module_t *module, const uint8_t beats_per_minute)
{
    module->initial_bpm = beats_per_minute;
}

void module_destroy(module_t *module)
{
    for (int i = 0; i < module->sample_slots; i++)
        destroy_sample(&module->samples[i]);
    for (int i = 0; i < module->num_patterns; i++)
        destroy_pattern(&module->patterns[i]);
    deallocate(MODULE, module->patterns);
    deallocate(MODULE, module->tracks);
    deallocate(MODULE, module->sequence);
    deallocate(MODULE, module->samples);
    deallocate(MODULE, module->tempo_lookup);
    deallocate(MODULE, module);
}

static void destroy_sample(sample_t *sample)
{
    deallocate(MODULE, (void *) sample->sample_data);
    sample->sample_data = NULL;
    sample->sample_length = 0;
}

static void destroy_pattern(pattern_t *pattern)
{
    deallocate(MODULE, pattern->events);
    pattern->events = NULL;
    pattern->num_lines = 0;
    pattern->line_capacity = 0;
}