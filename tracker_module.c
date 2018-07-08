#include "tracker_module.h"
#include "arctracker.h"
#include "error.h"
#include "heap.h"
#include "read_mod.h"
#include "bits.h"

module_t read_tracker_module(mapped_file_t file);

void get_patterns(void *array_start, long array_end, void **patterns);

int get_samples(void *array_start, long array_end, sample_t *samples);

sample_t *get_sample_info(void *array_start, long array_end);

bool is_tracker_format(mapped_file_t);;

const format_t tracker_format()
{
    format_t format_reader = {
            .is_this_format = is_tracker_format,
            .read_module = read_tracker_module
    };
    return format_reader;
}

bool is_tracker_format(mapped_file_t file)
{
    long array_end = (long) file.addr + file.size;
    return (search_tff(file.addr, array_end, MUSX_CHUNK, 1) != CHUNK_NOT_FOUND);
}

static inline
command_t tracker_command(__uint8_t code, __uint8_t data)
{
    switch (code)
    {
        case VOLUME_COMMAND_TRK:
            return SET_VOLUME_TRACKER;
        case SPEED_COMMAND_TRK:
            return SET_TEMPO;
        case STEREO_COMMAND_TRK:
            return SET_TRACK_STEREO;
        case VOLSLIDEUP_COMMAND_TRK:
            return VOLUME_SLIDE_UP;
        case VOLSLIDEDOWN_COMMAND_TRK:
            return VOLUME_SLIDE_DOWN;
        case PORTUP_COMMAND_TRK:
            return PORTAMENTO_UP;
        case PORTDOWN_COMMAND_TRK:
            return PORTAMENTO_DOWN;
        case TONEPORT_COMMAND_TRK:
            return TONE_PORTAMENTO;
        case BREAK_COMMAND_TRK:
            return BREAK_PATTERN;
        case JUMP_COMMAND_TRK:
            return JUMP_TO_POSITION;
        case ARPEGGIO_COMMAND_TRK:
            return (data == 0) ? NO_EFFECT : ARPEGGIO;
        default:
            return NO_EFFECT;
    }
}

static inline
effect_t effect(const __uint8_t code, const __uint8_t data)
{
    const effect_t effect = {
            .code = code,
            .data = data,
            .command = tracker_command(code, data)
    };
    return effect;
}

size_t decode_tracker_event(const __uint32_t *raw, channel_event_t *decoded)
{
    decoded->sample = MASK_8_SHIFT_RIGHT(*raw, 16);
    decoded->note = MASK_8_SHIFT_RIGHT(*raw, 24);
    decoded->effects[0] = effect(MASK_8_SHIFT_RIGHT(*raw, 8), MASK_8_SHIFT_RIGHT(*raw, 0));
    for (int i = 1; i <= 3; i++)
    {
        decoded->effects[i] = effect(0, 0);
    }
    return EVENT_SIZE_SINGLE_EFFECT;
}

module_t read_tracker_module(mapped_file_t file)
{
    void *chunk_address;
    long array_end = (long) file.addr + file.size;
    module_t module;

    memset(&module, 0, sizeof(module_t));
    module.format = TRACKER_FORMAT;
    module.decode_event = decode_tracker_event;
    module.initial_speed = 6;
    module.samples = allocate_array(36, sizeof(sample_t));

    if ((chunk_address = search_tff(file.addr, array_end, MVOX_CHUNK, 1)) == CHUNK_NOT_FOUND)
        error("Modfile corrupt - MVOX chunk not found");
    else
        memcpy(&module.num_channels, chunk_address + 8, 4);

    if ((chunk_address = search_tff(file.addr, array_end, STER_CHUNK, 1)) == CHUNK_NOT_FOUND)
        error("Modfile corrupt - STER chunk not found");
    else
        memcpy(module.default_channel_stereo, chunk_address + 8, MAX_CHANNELS_TRK);

    if ((chunk_address = search_tff(file.addr, array_end, MNAM_CHUNK, 1)) == CHUNK_NOT_FOUND)
        error("Modfile corrupt - MNAM chunk not found");
    else
        strncpy(module.name, chunk_address + 8, MAX_LEN_TUNENAME_TRK);

    if ((chunk_address = search_tff(file.addr, array_end, ANAM_CHUNK, 1)) == CHUNK_NOT_FOUND)
        error("Modfile corrupt - ANAM chunk not found");
    else
        strncpy(module.author, chunk_address + 8, MAX_LEN_AUTHOR_TRK);

    if ((chunk_address = search_tff(file.addr, array_end, MLEN_CHUNK, 1)) == CHUNK_NOT_FOUND)
        error("Modfile corrupt - MLEN chunk not found");
    else
        memcpy(&module.tune_length, chunk_address + 8, 4);

    if ((chunk_address = search_tff(file.addr, array_end, PNUM_CHUNK, 1)) == CHUNK_NOT_FOUND)
        error("Modfile corrupt - PNUM chunk not found");
    else
        memcpy(&module.num_patterns, chunk_address + 8, 4);

    if ((chunk_address = search_tff(file.addr, array_end, PLEN_CHUNK, 1)) == CHUNK_NOT_FOUND)
        error("Modfile corrupt - PLEN chunk not found");
    else
        memcpy(module.pattern_length, chunk_address + 8, NUM_PATTERNS);

    if ((chunk_address = search_tff(file.addr, array_end, SEQU_CHUNK, 1)) == CHUNK_NOT_FOUND)
        error("Modfile corrupt - SEQU chunk not found");
    else
        memcpy(module.sequence, chunk_address + 8, MAX_TUNELENGTH);

    get_patterns(file.addr, array_end, module.patterns);
    module.num_samples = get_samples(file.addr, array_end, module.samples);

    return module;
}

void get_patterns(void *array_start, long array_end, void **patterns)
{
    int pattern = 1;
    void *chunk_address = search_tff(array_start, array_end, PATT_CHUNK, pattern);
    while (chunk_address != CHUNK_NOT_FOUND)
    {
        pattern++;
        *(patterns++) = chunk_address + 8;
        chunk_address = search_tff(chunk_address + CHUNK_SIZE, array_end, PATT_CHUNK, pattern);
    }
    if (pattern == 1)
        error("Modfile corrupt - no patterns in module");
}

int get_samples(void *array_start, long array_end, sample_t *samples)
{
    int chunks_found = 0;
    int samples_found = 0;
    void *chunk_address = search_tff(array_start, array_end, SAMP_CHUNK, 1);
    while (chunk_address != CHUNK_NOT_FOUND && chunks_found < NUM_SAMPLES)
    {
        chunks_found++;
        sample_t *sample = get_sample_info(chunk_address, array_end);
        if (sample != SAMPLE_INVALID)
        {
            memcpy(samples, sample, sizeof(sample_t));
            samples++;
            samples_found++;
        }
        chunk_address = search_tff(chunk_address + CHUNK_SIZE, array_end, SAMP_CHUNK, 1);
    }
    if (chunks_found == 0)
    {
        error("Modfile corrupt - no samples in module");
    }
    return samples_found;
}

sample_t *get_sample_info(void *array_start, long array_end)
{
    void *chunk_address;
    sample_t *sample = allocate_array(1, sizeof(sample_t));
    memset(sample, 0, sizeof(sample_t));

    if ((chunk_address = search_tff(array_start, array_end, SNAM_CHUNK, 1)) == CHUNK_NOT_FOUND)
        return SAMPLE_INVALID;
    else
        strncpy(sample->name, chunk_address + 8, MAX_LEN_SAMPLENAME_TRK);

    if ((chunk_address = search_tff(array_start, array_end, SVOL_CHUNK, 1)) == CHUNK_NOT_FOUND)
        return SAMPLE_INVALID;
    else
        memcpy(&sample->default_gain, chunk_address + 8, 4);

    if ((chunk_address = search_tff(array_start, array_end, SLEN_CHUNK, 1)) == CHUNK_NOT_FOUND)
        return SAMPLE_INVALID;
    else
        memcpy(&sample->sample_length, chunk_address + 8, 4);

    if ((chunk_address = search_tff(array_start, array_end, ROFS_CHUNK, 1)) == CHUNK_NOT_FOUND)
        return SAMPLE_INVALID;
    else
        memcpy(&sample->repeat_offset, chunk_address + 8, 4);

    if ((chunk_address = search_tff(array_start, array_end, RLEN_CHUNK, 1)) == CHUNK_NOT_FOUND)
        return SAMPLE_INVALID;
    else
        memcpy(&sample->repeat_length, chunk_address + 8, 4);

    if ((chunk_address = search_tff(array_start, array_end, SDAT_CHUNK, 1)) == CHUNK_NOT_FOUND)
        return SAMPLE_INVALID;
    else
        sample->sample_data = chunk_address + 8;

    // transpose all notes up an octave when playing a Tracker module
    // compensating for the greater chromatic range in a Desktop Tracker module
    sample->transpose = 12;
    sample->repeats = (sample->repeat_length != 2);

    return sample;
}
