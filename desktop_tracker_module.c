#include "desktop_tracker_module.h"
#include "heap.h"
#include "arctracker.h"

#define DESKTOP_TRACKER_FORMAT "DESKTOP TRACKER"

bool is_desktop_tracker_format(mapped_file_t);

module_t read_desktop_tracker_module(mapped_file_t file);

module_format desktop_tracker_format()
{
    module_format format_reader = {
            .is_this_format = is_desktop_tracker_format,
            .read_module = read_desktop_tracker_module
    };
    return format_reader;
}

bool is_desktop_tracker_format(mapped_file_t file)
{
    long array_end = (long) file.addr + file.size;
    return (search_tff(file.addr, array_end, DSKT_CHUNK, 1) != CHUNK_NOT_FOUND);
}

module_t read_desktop_tracker_module(mapped_file_t file)
{
    void *tmp_ptr;
    long foo;
    int i;
    module_t module;

    long array_end = (long) file.addr + file.size;
    void *chunk_address = search_tff(file.addr, array_end, DSKT_CHUNK, 1);
    file.addr = chunk_address;

    memset(&module, 0, sizeof(module_t));
    module.format = DESKTOP_TRACKER;
    module.format_name = DESKTOP_TRACKER_FORMAT;
    module.initial_speed = 6;

    strncpy(module.name, file.addr + 4, MAX_LEN_TUNENAME_DSKT);

    strncpy(module.author, file.addr + 68, MAX_LEN_AUTHOR_DSKT);

    memcpy(&module.num_channels, file.addr + 136, 4);

    memcpy(&module.tune_length, file.addr + 140, 4);

    memcpy(module.default_channel_stereo, file.addr + 144, MAX_CHANNELS_DSKT);

    memcpy(&module.initial_speed, file.addr + 152, 4);

    memcpy(&module.num_patterns, file.addr + 160, 4);

    memcpy(&module.num_samples, file.addr + 164, 4);

    memcpy(module.sequence, file.addr + 168, (size_t) module.tune_length);

    tmp_ptr = file.addr + 168 + (((module.tune_length + 3) >> 2) << 2); /* align to word boundary */
    for (i = 0; i < module.num_patterns; i++)
    {
        memcpy(&foo, tmp_ptr, 4);
        module.patterns[i] = file.addr + foo;
        tmp_ptr += 4;
    }

    for (i = 0; i < module.num_patterns; i++)
    {
        module.pattern_length[i] = *(unsigned char *) tmp_ptr;
        tmp_ptr++;
    }

    if (module.num_patterns % 4)
        tmp_ptr = tmp_ptr + (4 - (module.num_patterns % 4));

    module.samples = allocate_array(module.num_samples, sizeof(sample_t));
    memset(module.samples, 0, sizeof(sample_t) * module.num_samples);
    for (i = 0; i < module.num_samples; i++)
    {
        module.samples[i].transpose = 26 - *(unsigned char *) tmp_ptr++;
        unsigned char sample_volume = *(unsigned char *) tmp_ptr;
        module.samples[i].default_gain = (sample_volume * 2) + 1;
        tmp_ptr += 3;
        memcpy(&(module.samples[i].period), tmp_ptr, 4);
        tmp_ptr += 4;
        memcpy(&(module.samples[i].sustain_start), tmp_ptr, 4);
        tmp_ptr += 4;
        memcpy(&(module.samples[i].sustain_length), tmp_ptr, 4);
        tmp_ptr += 4;
        memcpy(&(module.samples[i].repeat_offset), tmp_ptr, 4);
        tmp_ptr += 4;
        memcpy(&(module.samples[i].repeat_length), tmp_ptr, 4);
        tmp_ptr += 4;
        memcpy(&(module.samples[i].sample_length), tmp_ptr, 4);
        tmp_ptr += 4;
        strncpy(module.samples[i].name, tmp_ptr, MAX_LEN_SAMPLENAME_DSKT);
        tmp_ptr += MAX_LEN_SAMPLENAME_DSKT;
        memcpy(&foo, tmp_ptr, 4);
        module.samples[i].sample_data = file.addr + foo;
        module.samples[i].repeats = (module.samples[i].repeat_length != 0);
        tmp_ptr += 4;
    }

    return module;
}
