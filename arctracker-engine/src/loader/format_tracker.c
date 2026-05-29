#include "format_tracker.h"
#include <string.h>
#include "io/error.h"
#include "pcm/mu_law.h"
#include "memory/heap.h"
#include "memory/bits.h"

uint8_t *CHUNK_NOT_FOUND = NULL;
sample_t *get_sample_info_failed = NULL;

static const int CHUNK_ID_LENGTH = 4;
static const int CHUNK_HEADER_LENGTH = 8;
static const int MAX_LEN_TUNENAME_TRK = 32;
static const int MAX_LEN_AUTHOR_TRK = 32;
static const int MAX_LEN_SAMPLENAME_TRK = 20;
static const int NUM_SAMPLES = 36;

static const char *TRACKER_FORMAT = "TRACKER";

static const char *MUSX_CHUNK = "MUSX";
static const char *MVOX_CHUNK = "MVOX";
static const char *STER_CHUNK = "STER";
static const char *MNAM_CHUNK = "MNAM";
static const char *ANAM_CHUNK = "ANAM";
static const char *MLEN_CHUNK = "MLEN";
static const char *PNUM_CHUNK = "PNUM";
static const char *PLEN_CHUNK = "PLEN";
static const char *SEQU_CHUNK = "SEQU";
static const char *PATT_CHUNK = "PATT";
static const char *SAMP_CHUNK = "SAMP";
static const char *SNAM_CHUNK = "SNAM";
static const char *SVOL_CHUNK = "SVOL";
static const char *SLEN_CHUNK = "SLEN";
static const char *ROFS_CHUNK = "ROFS";
static const char *RLEN_CHUNK = "RLEN";
static const char *SDAT_CHUNK = "SDAT";

static const uint8_t ARPEGGIO_COMMAND = 0;      // 0
static const uint8_t PORTUP_COMMAND = 1;        // 1
static const uint8_t PORTDOWN_COMMAND = 2;      // 2
static const uint8_t TONEPORT_COMMAND = 3;      // 3
static const uint8_t BREAK_COMMAND = 11;        // B
static const uint8_t STEREO_COMMAND = 14;       // E
static const uint8_t VOLSLIDEUP_COMMAND = 16;   // G
static const uint8_t VOLSLIDEDOWN_COMMAND = 17; // H
static const uint8_t JUMP_COMMAND = 19;         // J
static const uint8_t SPEED_COMMAND = 28;        // S
static const uint8_t VOLUME_COMMAND = 31;       // V

static bool is_tracker_format(mapped_file_t);
static module_t *read_tracker_module(mapped_file_t);
static uint8_t *search_tff(uint8_t *, long, const char *);
static void calculate_gain_curve(float *);
static bool decode_patterns(uint8_t *, long, module_t *, const int *);
static size_t decode_tracker_event(const uint8_t *, event_t *);
static effect_t effect(uint8_t code, uint8_t);
static int get_samples(void *, long, sample_t *, float);
static bool get_sample_info(void *, long, float, sample_t *);
static void copy_int_array(uint8_t *, int *, int);

format_t tracker_format(void)
{
    format_t format_reader = {
            .is_this_format = is_tracker_format,
            .read_module = read_tracker_module
    };
    return format_reader;
}

static bool is_tracker_format(mapped_file_t file)
{
    long array_end = (long) file.addr + file.size;
    return (search_tff(file.addr, array_end, MUSX_CHUNK) != CHUNK_NOT_FOUND);
}

static module_t *read_tracker_module(mapped_file_t file)
{
    module_t *module = NULL;
    int *pattern_lengths = NULL;
    uint8_t *chunk_address;
    long array_end = (long) file.addr + file.size;
    if ((chunk_address = search_tff(file.addr, array_end, MVOX_CHUNK)) == CHUNK_NOT_FOUND)
    {
        error("Modfile corrupt - MVOX chunk not found");
        goto fail;
    }
    uint32_t num_channels = *(uint32_t *) (chunk_address + 8);
    if ((chunk_address = search_tff(file.addr, array_end, MLEN_CHUNK)) == CHUNK_NOT_FOUND)
    {
        error("Modfile corrupt - MLEN chunk not found");
        goto fail;
    }
    uint32_t sequence_len = *(uint32_t *) (chunk_address + 8);
    if ((chunk_address = search_tff(file.addr, array_end, PNUM_CHUNK)) == CHUNK_NOT_FOUND)
    {
        error("Modfile corrupt - PNUM chunk not found");
        goto fail;
    }
    uint32_t num_patterns = *(uint32_t *) (chunk_address + 8);
    module = module_create(num_channels, sequence_len, num_patterns, 36);
    if (module == NULL)
    {
        goto fail;
    }
    module->format = TRACKER_FORMAT;
    module->initial_speed = 6;
    module->volume_cmd_gain_factor = 1.0f;
    module->master_gain = 0.25f;
    calculate_gain_curve(module->gain_curve);
    if ((chunk_address = search_tff(file.addr, array_end, STER_CHUNK)) == CHUNK_NOT_FOUND)
    {
        error("Modfile corrupt - STER chunk not found");
        goto fail;
    }
    memcpy(module->initial_panning, chunk_address + 8, sizeof(int) * module->num_channels);
    if ((chunk_address = search_tff(file.addr, array_end, MNAM_CHUNK)) == CHUNK_NOT_FOUND)
    {
        error("Modfile corrupt - MNAM chunk not found");
        goto fail;
    }
    strncpy(module->name, (char *) chunk_address + 8, MAX_LEN_TUNENAME_TRK);
    if ((chunk_address = search_tff(file.addr, array_end, ANAM_CHUNK)) == CHUNK_NOT_FOUND)
    {
        error("Modfile corrupt - ANAM chunk not found");
        goto fail;
    }
    strncpy(module->author, (char *) chunk_address + 8, MAX_LEN_AUTHOR_TRK);
    if ((chunk_address = search_tff(file.addr, array_end, PLEN_CHUNK)) == CHUNK_NOT_FOUND)
    {
        error("Modfile corrupt - PLEN chunk not found");
        goto fail;
    }
    pattern_lengths = allocate_array(MODULE, NUM_PATTERNS, sizeof(int));
    if (pattern_lengths == NULL)
    {
        goto fail;
    }
    copy_int_array(chunk_address + 8, pattern_lengths, NUM_PATTERNS);
    if ((chunk_address = search_tff(file.addr, array_end, SEQU_CHUNK)) == CHUNK_NOT_FOUND)
    {
        error("Modfile corrupt - SEQU chunk not found");
        goto fail;
    }
    copy_int_array(chunk_address + 8, module->sequence, module->tune_length);
    if (!decode_patterns(file.addr, array_end, module, pattern_lengths))
        goto fail;
    module->num_samples = get_samples(file.addr, array_end, module->samples, module->volume_cmd_gain_factor);
    destroy_encoding_buffer();
    deallocate(MODULE, pattern_lengths);
    return module;
fail:
    if (module != NULL)
        module_destroy(module);
    if (pattern_lengths != NULL)
        deallocate(MODULE, pattern_lengths);
    destroy_encoding_buffer();
    return NULL;
}

static uint8_t *search_tff(uint8_t *array_start, const long array_end, const char *to_find)
{
    while ((long) array_start <= (array_end - CHUNK_ID_LENGTH))
    {
        if (memcmp(to_find, array_start, CHUNK_ID_LENGTH) == 0)
            return array_start;
        array_start++;
    }
    return CHUNK_NOT_FOUND;
}

static void calculate_gain_curve(float *gain_curve)
{
    for (int i = 0; i <= 127; i++)
    {
        float linear = mu_law_to_linear(255 - i);
        gain_curve[(i * 2) + 1] = linear;
        if (i >= 1)
            gain_curve[i * 2] = (gain_curve[(i * 2) - 1] + gain_curve[(i * 2) + 1]) / 2;
    }
    gain_curve[0] = 0.0f;
    gain_curve[1] = gain_curve[2] / 2;
}

static command_t tracker_command(int code, uint8_t data)
{
    switch (code)
    {
        case VOLUME_COMMAND: return SET_VOLUME;
        case SPEED_COMMAND: return SET_TEMPO;
        case STEREO_COMMAND: return SET_TRACK_STEREO;
        case VOLSLIDEUP_COMMAND: return VOLUME_SLIDE_UP;
        case VOLSLIDEDOWN_COMMAND: return VOLUME_SLIDE_DOWN;
        case PORTUP_COMMAND: return PORTAMENTO_UP;
        case PORTDOWN_COMMAND: return PORTAMENTO_DOWN;
        case TONEPORT_COMMAND: return TONE_PORTAMENTO;
        case BREAK_COMMAND: return BREAK_PATTERN;
        case JUMP_COMMAND: return JUMP_TO_POSITION;
        case ARPEGGIO_COMMAND: return (data == 0) ? NO_EFFECT : ARPEGGIO;
        default: return NO_EFFECT;
    }
}

static bool decode_patterns(uint8_t *array_start, const long array_end, module_t *module, const int *pattern_lengths)
{
    int patterns_found = 0;
    uint8_t *chunk_address = search_tff(array_start, array_end, PATT_CHUNK);
    while (chunk_address != CHUNK_NOT_FOUND)
    {
        const int pno = patterns_found++;
        const int pattern_length = pattern_lengths[pno];
        if (!module_create_pattern(module, pno, pattern_length))
        {
            return false;
        }
        const uint8_t *raw_pattern_data = chunk_address + CHUNK_HEADER_LENGTH;
        for (int line = 0; line < pattern_length; line++)
        {
            for (int channel = 0; channel < module->num_channels; channel++)
            {
                const int event_index = (line * module->num_channels) + channel;
                event_t *event = module->patterns[pno]->events + event_index;
                raw_pattern_data += decode_tracker_event(raw_pattern_data, event);
            }
        }
        chunk_address = search_tff(chunk_address + CHUNK_ID_LENGTH, array_end, PATT_CHUNK);
    }
    if (patterns_found == 0)
    {
        error("Modfile corrupt - no patterns in module");
        return false;
    }
    return true;
}

static size_t decode_tracker_event(const uint8_t *event_p, event_t *decoded)
{
    const uint32_t *raw = (uint32_t *) event_p;
    decoded->sample_no = MASK_8_SHIFT_RIGHT(*raw, 16);
    decoded->note = MASK_8_SHIFT_RIGHT(*raw, 24);
    decoded->effects[0] = effect(MASK_8_SHIFT_RIGHT(*raw, 8), MASK_8_SHIFT_RIGHT(*raw, 0));
    for (int i = 1; i <= 3; i++)
    {
        decoded->effects[i] = effect(0, 0);
    }
    return EVENT_SIZE_SINGLE_EFFECT;
}

static effect_t effect(const uint8_t code, const uint8_t data)
{
    const effect_t effect = {
            .data = data,
            .command = tracker_command(code, data)
    };
    return effect;
}

static int get_samples(void *array_start, long array_end, sample_t *samples, float gain_factor)
{
    int chunks_found = 0;
    int samples_found = 0;
    uint8_t *chunk_address = search_tff(array_start, array_end, SAMP_CHUNK);
    while (chunk_address != CHUNK_NOT_FOUND && chunks_found < NUM_SAMPLES)
    {
        chunks_found++;
        if (get_sample_info(chunk_address, array_end, gain_factor, samples))
        {
            samples++;
            samples_found++;
        }
        chunk_address = search_tff(chunk_address + CHUNK_ID_LENGTH, array_end, SAMP_CHUNK);
    }
    if (chunks_found == 0)
        error("Modfile corrupt - no samples in module");
    return samples_found;
}

static bool get_sample_info(void *array_start, long array_end, float gain_factor, sample_t *sample)
{
    uint8_t *chunk_address;

    // Sample name.
    if ((chunk_address = search_tff(array_start, array_end, SNAM_CHUNK)) == CHUNK_NOT_FOUND)
        goto get_sample_info_failed;
    strncpy(sample->name, (char *) chunk_address + CHUNK_HEADER_LENGTH, MAX_LEN_SAMPLENAME_TRK);

    // Sample volume.
    if ((chunk_address = search_tff(array_start, array_end, SVOL_CHUNK)) == CHUNK_NOT_FOUND)
        goto get_sample_info_failed;
    sample->default_gain = *(int32_t *) (chunk_address + CHUNK_HEADER_LENGTH) * gain_factor;

    // Sample length.
    if ((chunk_address = search_tff(array_start, array_end, SLEN_CHUNK)) == CHUNK_NOT_FOUND)
        goto get_sample_info_failed;
    sample->sample_length = *(int32_t *) (chunk_address + CHUNK_HEADER_LENGTH);

    // Repeat offset.
    if ((chunk_address = search_tff(array_start, array_end, ROFS_CHUNK)) == CHUNK_NOT_FOUND)
        goto get_sample_info_failed;
    sample->repeat_offset = *(int32_t *) (chunk_address + CHUNK_HEADER_LENGTH);

    // Repeat length.
    if ((chunk_address = search_tff(array_start, array_end, RLEN_CHUNK)) == CHUNK_NOT_FOUND)
        goto get_sample_info_failed;
    int repeat_length = *(int32_t *) (chunk_address + CHUNK_HEADER_LENGTH);
    if (repeat_length == 2 && sample->repeat_offset != 0)
        sample->repeat_length = sample->sample_length - sample->repeat_offset;
    else if (repeat_length + sample->repeat_offset > sample->sample_length)
        sample->repeat_length = sample->sample_length - sample->repeat_offset;
    else
        sample->repeat_length = repeat_length;

    // Sample data.
    if ((chunk_address = search_tff(array_start, array_end, SDAT_CHUNK)) == CHUNK_NOT_FOUND)
        goto get_sample_info_failed;
    uint8_t *sample_data_mu_law = chunk_address + CHUNK_HEADER_LENGTH;
    if (sample->sample_length == 0)
    {
        sample->sample_data = NULL;
        sample->repeats = false;
    }
    else
    {
        sample->sample_data = allocate_array(MODULE, sample->sample_length + 2, sizeof(float));
        if (!convert_vidc_encoded_sample(sample->sample_data, sample_data_mu_law, sample->sample_length))
            goto get_sample_info_failed;
        sample->repeats = (sample->repeat_offset != 0 || sample->repeat_length != 2);
        if (sample->repeats) {
            sample->sample_data[sample->sample_length] = sample->sample_data[sample->repeat_offset];
            sample->sample_data[sample->sample_length + 1] = sample->sample_data[sample->repeat_offset + 1];
        }
    }

    // Transpose all notes up an octave when playing a Tracker module
    // because Desktop Tracker has 5 octaves compared to Tracker's 3.
    sample->transpose = 12;

    return true;

get_sample_info_failed:
    return false;
}

static void copy_int_array(uint8_t *dest, int *source, int num_elements)
{
    for (int i = 0; i < num_elements; i++)
        source[i] = dest[i];
}
