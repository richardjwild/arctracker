#include "tracker_module.h"
#include "error.h"
#include "heap.h"
#include "arctracker.h"

#define SAMPLE_INVALID NULL
#define TRACKER_FORMAT "TRACKER"

module_t read_tracker_module(mapped_file_t file);

void get_patterns(void *array_start, long array_end, void **patterns);

int get_samples(void *array_start, long array_end, sample_t *samples);

sample_t *get_sample_info(void *array_start, long array_end);

bool is_tracker_format(mapped_file_t);;

const module_format tracker_format()
{
    module_format format_reader = {
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

module_t read_tracker_module(mapped_file_t file)
{
    void *chunk_address;
    long array_end = (long) file.addr + file.size;
    module_t module;

    memset(&module, 0, sizeof(module_t));
    module.format = TRACKER;
    module.format_name = TRACKER_FORMAT;
    module.initial_speed = 6;
    module.samples = allocate_array(36, sizeof(sample_t));

    if ((chunk_address = search_tff(file.addr, array_end, TINF_CHUNK, 1)) == CHUNK_NOT_FOUND)
        error("Modfile corrupt - TINF chunk not found");
    else
        strncpy(module.tracker_version, chunk_address + 8, 4);

    if ((chunk_address = search_tff(file.addr, array_end, MVOX_CHUNK, 1)) == CHUNK_NOT_FOUND)
        error("Modfile corrupt - MVOX chunk not found");
    else
        memcpy(&module.num_channels, chunk_address + 8, 4);

    if ((chunk_address = search_tff(file.addr, array_end, STER_CHUNK, 1)) == CHUNK_NOT_FOUND)
        error("Modfile corrupt - STER chunk not found");
    else
        memcpy(module.default_channel_stereo, chunk_address + 8, MAX_CHANNELS);

    if ((chunk_address = search_tff(file.addr, array_end, MNAM_CHUNK, 1)) == CHUNK_NOT_FOUND)
        error("Modfile corrupt - MNAM chunk not found");
    else
        strncpy(module.name, chunk_address + 8, MAX_LEN_TUNENAME);

    if ((chunk_address = search_tff(file.addr, array_end, ANAM_CHUNK, 1)) == CHUNK_NOT_FOUND)
        error("Modfile corrupt - ANAM chunk not found");
    else
        strncpy(module.author, chunk_address + 8, MAX_LEN_AUTHOR);

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
    void *chunk_address;
    while ((chunk_address = search_tff(array_start, array_end, PATT_CHUNK, pattern)) != CHUNK_NOT_FOUND)
    {
        pattern++;
        *(patterns++) = chunk_address + 8;
    }
    if (pattern == 1)
        error("Modfile corrupt - no patterns in module");
}

int get_samples(void *array_start, long array_end, sample_t *samples)
{
    int chunks_found = 0;
    int samples_found = 0;
    void *chunk_address;
    while ((chunk_address = search_tff(array_start, array_end, SAMP_CHUNK, chunks_found + 1)) != CHUNK_NOT_FOUND
           && chunks_found < NUM_SAMPLES)
    {
        chunks_found++;
        long chunk_length;
        memcpy(&chunk_length, chunk_address + CHUNKSIZE, 4);
        sample_t *sample;
        if ((sample = get_sample_info(chunk_address, array_end)) != SAMPLE_INVALID)
        {
            memcpy(samples, sample, sizeof(sample_t));
            samples++;
            samples_found++;
        }
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
        strncpy(sample->name, chunk_address + 8, MAX_LEN_SAMPLENAME);

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
