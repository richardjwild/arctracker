#include "tracker_module.h"
#include "arctracker.h"
#include "error.h"
#include "heap.h"
#include "read_mod.h"
#include "bits.h"

void *search_tff(void *array_start, long array_end, const void *to_find);

module_t read_tracker_module(mapped_file_t file);

static int *convert_int_array(__uint8_t *unsigned_bytes, int num_elements);

void get_patterns(void *array_start, long array_end, void **patterns);

static int get_samples(void *array_start, long array_end, sample_t *samples);

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
    return (search_tff(file.addr, array_end, MUSX_CHUNK) != CHUNK_NOT_FOUND);
}

void *search_tff(void *array_start, const long array_end, const void *to_find)
{
    while ((long) array_start <= (array_end - CHUNK_ID_LENGTH))
    {
        if (memcmp(to_find, array_start, CHUNK_ID_LENGTH) == 0)
            return array_start;
        else
            array_start += 1;
    }
    return CHUNK_NOT_FOUND;
}

static inline
command_t tracker_command(int code, __uint8_t data)
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

    if ((chunk_address = search_tff(file.addr, array_end, MVOX_CHUNK)) == CHUNK_NOT_FOUND)
        error("Modfile corrupt - MVOX chunk not found");
    else
        memcpy(&module.num_channels, chunk_address + 8, 4);

    if ((chunk_address = search_tff(file.addr, array_end, STER_CHUNK)) == CHUNK_NOT_FOUND)
        error("Modfile corrupt - STER chunk not found");
    else
        module.initial_panning = convert_int_array(chunk_address + 8, module.num_channels);

    if ((chunk_address = search_tff(file.addr, array_end, MNAM_CHUNK)) == CHUNK_NOT_FOUND)
        error("Modfile corrupt - MNAM chunk not found");
    else
        strncpy(module.name, chunk_address + 8, MAX_LEN_TUNENAME_TRK);

    if ((chunk_address = search_tff(file.addr, array_end, ANAM_CHUNK)) == CHUNK_NOT_FOUND)
        error("Modfile corrupt - ANAM chunk not found");
    else
        strncpy(module.author, chunk_address + 8, MAX_LEN_AUTHOR_TRK);

    if ((chunk_address = search_tff(file.addr, array_end, MLEN_CHUNK)) == CHUNK_NOT_FOUND)
        error("Modfile corrupt - MLEN chunk not found");
    else
        memcpy(&module.tune_length, chunk_address + 8, 4);

    if ((chunk_address = search_tff(file.addr, array_end, PNUM_CHUNK)) == CHUNK_NOT_FOUND)
        error("Modfile corrupt - PNUM chunk not found");
    else
        memcpy(&module.num_patterns, chunk_address + 8, 4);

    if ((chunk_address = search_tff(file.addr, array_end, PLEN_CHUNK)) == CHUNK_NOT_FOUND)
        error("Modfile corrupt - PLEN chunk not found");
    else
        module.pattern_lengths = convert_int_array(chunk_address + 8, NUM_PATTERNS);

    if ((chunk_address = search_tff(file.addr, array_end, SEQU_CHUNK)) == CHUNK_NOT_FOUND)
        error("Modfile corrupt - SEQU chunk not found");
    else
        module.sequence = convert_int_array(chunk_address + 8, module.tune_length);

    get_patterns(file.addr, array_end, module.patterns);
    module.num_samples = get_samples(file.addr, array_end, module.samples);

    return module;
}

static int *convert_int_array(__uint8_t *unsigned_bytes, int num_elements)
{
    int *int_array = allocate_array(num_elements, sizeof(int));
    for (int i = 0; i < num_elements; i++)
        int_array[i] = unsigned_bytes[i];
    return int_array;
}

void get_patterns(void *array_start, long array_end, void **patterns)
{
    int patterns_found = 0;
    void *chunk_address = search_tff(array_start, array_end, PATT_CHUNK);
    while (chunk_address != CHUNK_NOT_FOUND)
    {
        *(patterns++) = chunk_address + CHUNK_HEADER_LENGTH;
        patterns_found++;
        chunk_address = search_tff(chunk_address + CHUNK_ID_LENGTH, array_end, PATT_CHUNK);
    }
    if (patterns_found == 0)
        error("Modfile corrupt - no patterns in module");
}

static int get_samples(void *array_start, long array_end, sample_t *samples)
{
    int chunks_found = 0;
    int samples_found = 0;
    void *chunk_address = search_tff(array_start, array_end, SAMP_CHUNK);
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
        chunk_address = search_tff(chunk_address + CHUNK_ID_LENGTH, array_end, SAMP_CHUNK);
    }
    if (chunks_found == 0)
    {
        error("Modfile corrupt - no samples in module");
    }
    return samples_found;
}

sample_t *get_sample_info(void *array_start, long array_end)
{
    sample_t *sample = allocate_array(1, sizeof(sample_t));
    memset(sample, 0, sizeof(sample_t));
    void *chunk_address;

    if ((chunk_address = search_tff(array_start, array_end, SNAM_CHUNK)) == CHUNK_NOT_FOUND)
        return SAMPLE_INVALID;
    else
        strncpy(sample->name, chunk_address + CHUNK_HEADER_LENGTH, MAX_LEN_SAMPLENAME_TRK);

    if ((chunk_address = search_tff(array_start, array_end, SVOL_CHUNK)) == CHUNK_NOT_FOUND)
        return SAMPLE_INVALID;
    else
    {
        __uint32_t default_gain;
        memcpy(&default_gain, chunk_address + CHUNK_HEADER_LENGTH, sizeof(__uint32_t));
        sample->default_gain = default_gain;
    }

    if ((chunk_address = search_tff(array_start, array_end, SLEN_CHUNK)) == CHUNK_NOT_FOUND)
        return SAMPLE_INVALID;
    else
    {
        __uint32_t sample_length;
        memcpy(&sample_length, chunk_address + CHUNK_HEADER_LENGTH, sizeof(__uint32_t));
        sample->sample_length = sample_length;
    }

    if ((chunk_address = search_tff(array_start, array_end, ROFS_CHUNK)) == CHUNK_NOT_FOUND)
        return SAMPLE_INVALID;
    else
    {
        __uint32_t repeat_offset;
        memcpy(&repeat_offset, chunk_address + CHUNK_HEADER_LENGTH, sizeof(__uint32_t));
        sample->repeat_offset = repeat_offset;
    }

    if ((chunk_address = search_tff(array_start, array_end, RLEN_CHUNK)) == CHUNK_NOT_FOUND)
        return SAMPLE_INVALID;
    else
    {
        __uint32_t repeat_length;
        memcpy(&repeat_length, chunk_address + CHUNK_HEADER_LENGTH, sizeof(__uint32_t));
        sample->repeat_length = repeat_length;
    }

    if ((chunk_address = search_tff(array_start, array_end, SDAT_CHUNK)) == CHUNK_NOT_FOUND)
        return SAMPLE_INVALID;
    else
        sample->sample_data = chunk_address + CHUNK_HEADER_LENGTH;

    // transpose all notes up an octave when playing a Tracker module
    // compensating for the greater chromatic range in a Desktop Tracker module
    sample->transpose = 12;
    sample->repeats = (sample->repeat_length != 2);

    return sample;
}
