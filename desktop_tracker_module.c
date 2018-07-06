#include "desktop_tracker_module.h"
#include "heap.h"
#include "read_mod.h"
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

static inline
command_t desktop_tracker_command(__uint8_t code, __uint8_t data)
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
        case JUMP_COMMAND_DSKT: return JUMP_TO_POSITION;
        case SETFINETEMPO_COMMAND_DSKT: return SET_TEMPO_FINE;
        case FINEPORTAMENTO_COMMAND_DSKT: return PORTAMENTO_FINE;
        case FINEVOLSLIDE_COMMAND_DSKT: return VOLUME_SLIDE_FINE;
        case ARPEGGIO_COMMAND_DSKT: return (data == 0) ? NO_EFFECT : ARPEGGIO;
        default: return NO_EFFECT;
    }
}

static inline
effect_t effect(const __uint8_t code, const __uint8_t data) {
    const effect_t effect = {
            .code = code,
            .data = data,
            .command = desktop_tracker_command(code, data)
    };
    return effect;
}

size_t decode_desktop_tracker_event(const __uint32_t *raw, channel_event_t *decoded)
{
    decoded->sample = MASK_6_SHIFT_RIGHT(*raw, 0);
    decoded->note = MASK_6_SHIFT_RIGHT(*raw, 6);
    if (*raw & (0x1f << 17)) {
        decoded->effects[0] = effect(MASK_5_SHIFT_RIGHT(*raw, 12), MASK_8_SHIFT_RIGHT(*(raw + 1), 0));
        decoded->effects[1] = effect(MASK_5_SHIFT_RIGHT(*raw, 17), MASK_8_SHIFT_RIGHT(*(raw + 1), 8));
        decoded->effects[2] = effect(MASK_5_SHIFT_RIGHT(*raw, 22), MASK_8_SHIFT_RIGHT(*(raw + 1), 16));
        decoded->effects[3] = effect(MASK_5_SHIFT_RIGHT(*raw, 27), MASK_8_SHIFT_RIGHT(*(raw + 1), 24));
        return EVENT_SIZE_MULTIPLE_EFFECT;
    } else {
        decoded->effects[0] = effect(MASK_5_SHIFT_RIGHT(*raw, 12), MASK_8_SHIFT_RIGHT(*raw, 24));
        decoded->effects[1] = effect(0, 0);
        decoded->effects[2] = effect(0, 0);
        decoded->effects[3] = effect(0, 0);
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
    module.format_name = DESKTOP_TRACKER_FORMAT;
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
