#include "tracker_module.h"
#include <audio/gain.h>
#include <io/error.h>
#include <pcm/mu_law.h>
#include <memory/heap.h>
#include <memory/bits.h>
#include <arctracker.h>

#define SAMPLE_INVALID NULL
#define CHUNK_NOT_FOUND NULL

static const int CHUNK_ID_LENGTH = 4;
static const int CHUNK_HEADER_LENGTH = 8;
static const int MAX_LEN_TUNENAME_TRK = 32;
static const int MAX_LEN_AUTHOR_TRK = 32;
static const int MAX_LEN_SAMPLENAME_TRK = 20;
static const int NUM_SAMPLES = 256;

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

static const __uint8_t ARPEGGIO_COMMAND_TRK = 0;      // 0
static const __uint8_t PORTUP_COMMAND_TRK = 1;        // 1
static const __uint8_t PORTDOWN_COMMAND_TRK = 2;      // 2
static const __uint8_t TONEPORT_COMMAND_TRK = 3;      // 3
static const __uint8_t BREAK_COMMAND_TRK = 11;        // B
static const __uint8_t STEREO_COMMAND_TRK = 14;       // E
static const __uint8_t VOLSLIDEUP_COMMAND_TRK = 16;   // G
static const __uint8_t VOLSLIDEDOWN_COMMAND_TRK = 17; // H
static const __uint8_t JUMP_COMMAND_TRK = 19;         // J
static const __uint8_t SPEED_COMMAND_TRK = 28;        // S
static const __uint8_t VOLUME_COMMAND_TRK = 31;       // V

void *search_tff(void *, long, const void *);

module_t read_tracker_module(mapped_file_t);

static int *convert_int_array(__uint8_t *, int);

void get_patterns(void *, long, void **);

static int get_samples(void *, long, sample_t *);

sample_t *get_sample_info(void *, long);

bool is_tracker_format(mapped_file_t);

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
    if (code == VOLUME_COMMAND_TRK)
        return SET_VOLUME;
    else if (code == SPEED_COMMAND_TRK)
        return SET_TEMPO;
    else if (code == STEREO_COMMAND_TRK)
        return SET_TRACK_STEREO;
    else if (code == VOLSLIDEUP_COMMAND_TRK)
        return VOLUME_SLIDE_UP;
    else if (code == VOLSLIDEDOWN_COMMAND_TRK)
        return VOLUME_SLIDE_DOWN;
    else if (code == PORTUP_COMMAND_TRK)
        return PORTAMENTO_UP;
    else if (code == PORTDOWN_COMMAND_TRK)
        return PORTAMENTO_DOWN;
    else if (code == TONEPORT_COMMAND_TRK)
        return TONE_PORTAMENTO;
    else if (code == BREAK_COMMAND_TRK)
        return BREAK_PATTERN;
    else if (code == JUMP_COMMAND_TRK)
        return JUMP_TO_POSITION;
    else if (code == ARPEGGIO_COMMAND_TRK)
        return (data == 0) ? NO_EFFECT : ARPEGGIO;
    else
        return NO_EFFECT;
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
    module_gain_goes_to(255);

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

int read_word(__uint32_t *address)
{
    __uint32_t value;
    memcpy(&value, address, sizeof(__uint32_t));
    return value;
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
        sample->default_gain = read_word(chunk_address + CHUNK_HEADER_LENGTH);

    if ((chunk_address = search_tff(array_start, array_end, SLEN_CHUNK)) == CHUNK_NOT_FOUND)
        return SAMPLE_INVALID;
    else
        sample->sample_length = read_word(chunk_address + CHUNK_HEADER_LENGTH);

    if ((chunk_address = search_tff(array_start, array_end, ROFS_CHUNK)) == CHUNK_NOT_FOUND)
        return SAMPLE_INVALID;
    else
        sample->repeat_offset = read_word(chunk_address + CHUNK_HEADER_LENGTH);

    if ((chunk_address = search_tff(array_start, array_end, RLEN_CHUNK)) == CHUNK_NOT_FOUND)
        return SAMPLE_INVALID;
    else
    {
        int repeat_length = read_word(chunk_address + CHUNK_HEADER_LENGTH);
        if (repeat_length == 2 && sample->repeat_offset != 0)
            sample->repeat_length = sample->sample_length - sample->repeat_offset;
        else if (repeat_length + sample->repeat_offset > sample->sample_length)
            sample->repeat_length = sample->sample_length - sample->repeat_offset;
        else
            sample->repeat_length = repeat_length;
    }

    if ((chunk_address = search_tff(array_start, array_end, SDAT_CHUNK)) == CHUNK_NOT_FOUND)
        return SAMPLE_INVALID;
    else
    {
        __uint8_t *sample_data_mu_law = chunk_address + CHUNK_HEADER_LENGTH;
        sample->sample_data = convert_mu_law_to_linear_pcm(sample_data_mu_law, sample->sample_length);
    }

    // transpose all notes up an octave when playing a Tracker module
    // compensating for the greater chromatic range in a Desktop Tracker module
    sample->transpose = 12;
    sample->repeats = (sample->repeat_offset != 0 || sample->repeat_length != 2);

    return sample;
}
