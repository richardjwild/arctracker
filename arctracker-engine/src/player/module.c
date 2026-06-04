#include <string.h>
#include <stdio.h>
#include "module.h"
#include "memory/heap.h"

#define INITIAL_SEQUENCE_CAPACITY 1024
#define INITIAL_PATTERN_CAPACITY 1024

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
    module->sequence = allocate_array(MODULE, module->sequence_capacity, sizeof(int));
    if (module->sequence == NULL)
        goto fail;
    module->initial_panning = allocate_array(MODULE, num_channels, sizeof(int));
    if (module->initial_panning == NULL)
        goto fail;
    module->patterns = allocate_array(MODULE, module->pattern_capacity, sizeof(pattern_t));
    if (module->patterns == NULL)
        goto fail;
    module->samples = allocate_array(MODULE, num_samples, sizeof(sample_t));
    if (module->samples == NULL)
        goto fail;
    module->gain_curve = allocate_array(MODULE, INTERNAL_GAIN_MAX + 1, sizeof(float));
    if (module->gain_curve == NULL)
        goto fail;
    return module;
fail:
    if (module != NULL) module_destroy(module);
    return NULL;
}

bool module_init(module_t *module)
{
    module->initial_speed = 6;
    for (int i = 0; i < module->num_channels; i++)
        module->initial_panning[i] = 3; // Centre
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

void module_destroy(module_t *module)
{
    for (int i = 0; i < module->num_samples; i++)
        deallocate(MODULE, module->samples[i].sample_data);
    for (int i = 0; i < module->num_patterns; i++)
        deallocate(MODULE, module->patterns[i].events);
    deallocate(MODULE, module->patterns);
    deallocate(MODULE, module->gain_curve);
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
    module_info->num_samples = module->num_samples;
    module_info->num_patterns = module->num_patterns;
}

void module_get_sample_info(module_t *module, int sample_no, ui_sample_info_t *sample_info)
{
    sample_t sample = module->samples[sample_no];
    snprintf(sample_info->name, sizeof sample_info->name, "%s", sample.name);
    sample_info->default_gain = sample.default_gain;
    sample_info->sample_length = sample.sample_length;
    sample_info->repeats = sample.repeats;
    sample_info->repeat_offset = sample.repeat_offset;
    sample_info->repeat_length = sample.repeat_length;
    sample_info->transpose = sample.transpose;
}
