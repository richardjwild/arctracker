#include <string.h>
#include <stdio.h>
#include "module.h"
#include "memory/heap.h"

#define INITIAL_SEQUENCE_CAPACITY 1024
#define INITIAL_PATTERN_CAPACITY 1024
#define INITIAL_SAMPLE_CAPACITY 1024

module_t *module_create(int num_channels, int sequence_len, int num_patterns, int num_samples)
{
    module_t *module = allocate_array(MODULE, 1, sizeof(module_t));
    if (module == NULL)
        goto fail;
    module->num_channels = num_channels;
    module->tune_length = sequence_len;
    module->sequence_capacity = sequence_len > INITIAL_SEQUENCE_CAPACITY ? sequence_len : INITIAL_SEQUENCE_CAPACITY;
    module->num_patterns = num_patterns;
    module->pattern_capacity = num_patterns > INITIAL_PATTERN_CAPACITY ? num_patterns : INITIAL_PATTERN_CAPACITY;
    module->num_samples = num_samples;
    module->sample_capacity = num_samples > INITIAL_SAMPLE_CAPACITY ? num_samples : INITIAL_SAMPLE_CAPACITY;
    module->sequence = allocate_array(MODULE, module->sequence_capacity, sizeof(int));
    if (module->sequence == NULL)
        goto fail;
    module->initial_panning = allocate_array(MODULE, num_channels, sizeof(int));
    if (module->initial_panning == NULL)
        goto fail;
    module->patterns = allocate_array(MODULE, module->pattern_capacity, sizeof(pattern_t));
    if (module->patterns == NULL)
        goto fail;
    module->samples = allocate_array(MODULE, module->sample_capacity, sizeof(sample_t));
    if (module->samples == NULL)
        goto fail;
    return module;
fail:
    if (module != NULL) module_destroy(module);
    return NULL;
}

bool module_init(module_t *module)
{
    module->initial_speed = 6;
    module->master_gain = 0.25f;
    for (int i = 0; i < module->num_channels; i++)
        module->initial_panning[i] = 0x80; // Centre
    for (int i = 0; i < NUM_INSTRUMENT_SLOTS; i++)
        module->instruments[i].assigned = false;
    module->patterns[0] = (pattern_t) {
        .num_lines = 64,
        .events = allocate_array(MODULE, 64 * module->num_channels, sizeof(event_t)),
    };
    if (module->patterns[0].events == NULL)
        return false;
    return true;
}

bool module_create_pattern(module_t *module, int pattern_no, int num_lines)
{
    pattern_t pattern = (pattern_t) {
        .num_lines = num_lines,
        .events = allocate_array(MODULE, num_lines * module->num_channels, sizeof(event_t)),
    };
    if (pattern.events == NULL)
        return false;
    module->patterns[pattern_no] = pattern;
    return true;
}

void module_delete_pattern(module_t *module, int pattern_no)
{
    if (pattern_no != module->num_patterns - 1)
    {
        // TODO: Make this capable of deleting a pattern from the middle of the array.
        return;
    }
    deallocate(MODULE, module->patterns[pattern_no].events);
    module->patterns[pattern_no] = (pattern_t) {
        .num_lines = 0,
        .events = NULL,
    };
    module->num_patterns--;
}

void module_destroy(module_t *module)
{
    for (int i = 0; i < module->num_samples; i++)
        deallocate(MODULE, module->samples[i].sample_data);
    for (int i = 0; i < module->num_patterns; i++)
        deallocate(MODULE, module->patterns[i].events);
    deallocate(MODULE, module->patterns);
    deallocate(MODULE, module->initial_panning);
    deallocate(MODULE, module->sequence);
    deallocate(MODULE, module->samples);
    deallocate(MODULE, module);
}

void module_get_info(module_t *module, ui_module_info_t *module_info)
{
    snprintf(module_info->name, sizeof module_info->name, "%s", module->name);
    snprintf(module_info->author, sizeof module_info->author, "%s", module->author);
    module_info->num_channels = module->num_channels;
    module_info->tune_length = module->tune_length;
    module_info->num_patterns = module->num_patterns;
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

bool module_link_sample(module_t *module, float *sample_data, const int sample_length, int *sample_index)
{
    *sample_index = module->num_samples;
    module->samples[*sample_index] = (sample_t) {
        .sample_data = sample_data,
        .sample_length = sample_length,
    };
    module->num_samples++;
    return true;
}
