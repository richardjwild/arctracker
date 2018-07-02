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

command_t get_desktop_tracker_command(int code)
{
    switch (code)
    {
        case VOLUME_COMMAND_DSKT: return SET_VOLUME_DESKTOP_TRACKER;
        case SPEED_COMMAND_DSKT: return SET_TEMPO;
        case STEREO_COMMAND_DSKT: return SET_TRACK_STEREO;
        case VOLSLIDE_COMMAND_DSKT: return VOLUME_SLIDE;
        case PORTUP_COMMAND_DSKT: return PORTAMENTO_UP;
        case PORTDOWN_COMMAND_DSKT: return PORTAMENTO_DOWN;
        case TONEPORT_COMMAND_DSKT: return TONE_PORTAMENTO;
        case ARPEGGIO_COMMAND_DSKT: return ARPEGGIO;
        case JUMP_COMMAND_DSKT: return JUMP_TO_POSITION;
        case SETFINETEMPO_COMMAND_DSKT: return SET_TEMPO_FINE;
        case FINEPORTAMENTO_COMMAND_DSKT: return PORTAMENTO_FINE;
        case FINEVOLSLIDE_COMMAND_DSKT: return VOLUME_SLIDE_FINE;
        default: return NO_EFFECT;
    }
}

size_t decode_desktop_tracker_event(__uint32_t *raw, channel_event_t *decoded)
{
    decoded->note = (*raw & 0xfc0) >> 6;
    decoded->sample = *raw & 0x3f;

    if (*raw & (0x1f << 17)) {
        decoded->command0 = (*raw & 0x1f000) >> 12;    //                0001 1111 0000 0000 0000
        decoded->command1 = (*raw & 0x3e0000) >> 17;   //           0011 1110 0000 0000 0000 0000
        decoded->command2 = (*raw & 0x7c00000) >> 22;  //      0111 1100 0000 0000 0000 0000 0000
        decoded->command3 = (*raw & 0xf8000000) >> 27; // 1111 1000 0000 0000 0000 0000 0000 0000
        raw += 1;
        decoded->data0 = *raw & 0xff;
        decoded->data1 = (*raw & 0xff00) >> 8;
        decoded->data2 = (*raw & 0xff0000) >> 16;
        decoded->data3 = (*raw & 0xff000000) >> 24;
        return EVENT_SIZE_MULTIPLE_EFFECT;
    } else {
        decoded->data0 = (*raw & 0xff000000) >> 24;
        decoded->data1 = 0;
        decoded->data2 = 0;
        decoded->data3 = 0;
        decoded->command0 = (*raw & 0x1f000) >> 12;
        decoded->command1 = 0;
        decoded->command2 = 0;
        decoded->command3 = 0;
        return EVENT_SIZE_SINGLE_EFFECT;
    }
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
    module.get_command = get_desktop_tracker_command;
    module.decode_event = decode_desktop_tracker_event;
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
