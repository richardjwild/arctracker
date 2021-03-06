#include "format_desktop_tracker.h"
#include <audio/gain.h>
#include <io/read_mod.h>
#include <memory/bits.h>
#include <memory/heap.h>
#include <pcm/mu_law.h>

// As you can see, about half of the effects are left unimplemented. Neither
// is the sample sustain feature. I could in principle have a stab at doing
// some of them, but I do not posses any modfiles that make use of these
// features so I have no means to test them properly. Therefore I have elected
// not to implement them, unless I should ever come into possession of some
// modfiles that could serve as an acceptance test, which seems quite unlikely.

#define MAX_LEN_TUNENAME_DSKT 64
#define MAX_LEN_AUTHOR_DSKT 64
#define MAX_LEN_SAMPLENAME_DSKT 32
#define IS_MULTIPLE_EFFECT(raw_event) \
    ((raw_event) & (0x1f << 17)) > 0

static const char *DESKTOP_TRACKER_FORMAT = "DESKTOP TRACKER";
static const char *DTT_FILE_IDENTIFIER = "DskT";
static const int MODULE_GAIN_MAX = 127;
static const uint8_t VOLUME_VALUE_MASK = 0x7f;

static const uint8_t ARPEGGIO_COMMAND = 0x0;
static const uint8_t PORTUP_COMMAND = 0x1;
static const uint8_t PORTDOWN_COMMAND = 0x2;
static const uint8_t TONEPORT_COMMAND = 0x3;
static const uint8_t VIBRATO_COMMAND_NOT_IMPLEMENTED = 0x4;
static const uint8_t DELAYEDNOTE_COMMAND_NOT_IMPLEMENTED = 0x5;
static const uint8_t RELEASESAMP_COMMAND_NOT_IMPLEMENTED = 0x6;
static const uint8_t TREMOLO_COMMAND_NOT_IMPLEMENTED = 0x7;
static const uint8_t PHASOR_COMMAND1_NOT_IMPLEMENTED = 0x8;
static const uint8_t PHASOR_COMMAND2_NOT_IMPLEMENTED = 0x9;
static const uint8_t VOLSLIDE_COMMAND = 0xa;
static const uint8_t JUMP_COMMAND = 0xb;
static const uint8_t VOLUME_COMMAND = 0xc;
static const uint8_t STEREO_COMMAND = 0xd;
static const uint8_t STEREOSLIDE_COMMAND_NOT_IMPLEMENTED = 0xe;
static const uint8_t SPEED_COMMAND = 0xf;
static const uint8_t ARPEGGIOSPEED_COMMAND_NOT_IMPLEMENTED = 0x10;
static const uint8_t FINEPORTAMENTO_COMMAND = 0x11;
static const uint8_t CLEAREPEAT_COMMAND_NOT_IMPLEMENTED = 0x12;
static const uint8_t SETVIBRATOWAVEFORM_COMMAND_NOT_IMPLEMENTED = 0x14;
static const uint8_t LOOP_COMMAND_NOT_IMPLEMENTED = 0x16;
static const uint8_t SETTREMOLOWAVEFORM_COMMAND_NOT_IMPLEMENTED = 0x17;
static const uint8_t SETFINETEMPO_COMMAND = 0x18;
static const uint8_t RETRIGGERSAMPLE_COMMAND_NOT_IMPLEMENTED = 0x19;
static const uint8_t FINEVOLSLIDE_COMMAND = 0x1a;
static const uint8_t HOLD_COMMAND_NOT_IMPLEMENTED = 0x1b;
static const uint8_t NOTECUT_COMMAND_NOT_IMPLEMENTED = 0x1c;
static const uint8_t NOTEDELAY_COMMAND_NOT_IMPLEMENTED = 0x1d;
static const uint8_t PATTERNDELAY_COMMAND_NOT_IMPLEMENTED = 0x1e;
static const uint8_t CALLLINKEDCODE_COMMAND_NOT_IMPLEMENTED = 0x1f;

typedef struct
{
    __uint32_t identifier;
    char name[MAX_LEN_TUNENAME_DSKT];
    char author[MAX_LEN_AUTHOR_DSKT];
    __uint32_t flags;
    __uint32_t num_channels;
    __uint32_t tune_length;
    uint8_t initial_stereo[8];
    __uint32_t initial_speed;
    __uint32_t restart;
    __uint32_t num_patterns;
    __uint32_t num_samples;
} dtt_file_format_t;

typedef struct
{
    uint8_t note;
    uint8_t volume;
    __uint16_t unused;
    __uint32_t period;
    __uint32_t sustain_start;
    __uint32_t sustain_end;
    __uint32_t repeat_offset;
    __uint32_t repeat_length;
    __uint32_t sample_length;
    char name[MAX_LEN_SAMPLENAME_DSKT];
    __uint32_t sample_data_offset;
} dtt_sample_format_t;

bool is_desktop_tracker_format(mapped_file_t);

module_t read_desktop_tracker_module(mapped_file_t);

static module_t create_module(dtt_file_format_t *);

static int *convert_int_array(const uint8_t *, int);

static void set_pattern_starts(module_t *, __uint32_t *, void *);

static sample_t *get_samples(int, dtt_sample_format_t *, void *);

const format_t desktop_tracker_format()
{
    format_t format_reader = {
            .is_this_format = is_desktop_tracker_format,
            .read_module = read_desktop_tracker_module
    };
    return format_reader;
}

bool is_desktop_tracker_format(mapped_file_t file)
{
    return memcmp(file.addr, DTT_FILE_IDENTIFIER, strlen(DTT_FILE_IDENTIFIER)) == 0;
}

static inline
command_t desktop_tracker_command(int code, uint8_t data)
{
    if (code == VOLUME_COMMAND)
        return SET_VOLUME;
    else if (code == SPEED_COMMAND)
        return SET_TEMPO;
    else if (code == STEREO_COMMAND)
        return SET_TRACK_STEREO;
    else if (code == VOLSLIDE_COMMAND)
        return VOLUME_SLIDE;
    else if (code == PORTUP_COMMAND)
        return PORTAMENTO_UP;
    else if (code == PORTDOWN_COMMAND)
        return PORTAMENTO_DOWN;
    else if (code == TONEPORT_COMMAND)
        return TONE_PORTAMENTO;
    else if (code == JUMP_COMMAND)
        return JUMP_TO_POSITION;
    else if (code == SETFINETEMPO_COMMAND)
        return SET_TEMPO_FINE;
    else if (code == FINEPORTAMENTO_COMMAND)
        return PORTAMENTO_FINE;
    else if (code == FINEVOLSLIDE_COMMAND)
        return VOLUME_SLIDE_FINE;
    else if (code == ARPEGGIO_COMMAND)
        return (data == 0) ? NO_EFFECT : ARPEGGIO;
    else
        return NO_EFFECT;
}

static inline
effect_t effect(const uint8_t code, const uint8_t data)
{
    const effect_t effect = {
            .code = code,
            .data = (code == VOLUME_COMMAND) ? (data & VOLUME_VALUE_MASK) : data,
            .command = desktop_tracker_command(code, data)
    };
    return effect;
}

size_t decode_desktop_tracker_event(const __uint32_t *raw, channel_event_t *decoded)
{
    decoded->sample = MASK_6_SHIFT_RIGHT(*raw, 0);
    decoded->note = MASK_6_SHIFT_RIGHT(*raw, 6);
    if (IS_MULTIPLE_EFFECT(*raw))
    {
        decoded->effects[0] = effect(MASK_5_SHIFT_RIGHT(*raw, 12), MASK_8_SHIFT_RIGHT(*(raw + 1), 0));
        decoded->effects[1] = effect(MASK_5_SHIFT_RIGHT(*raw, 17), MASK_8_SHIFT_RIGHT(*(raw + 1), 8));
        decoded->effects[2] = effect(MASK_5_SHIFT_RIGHT(*raw, 22), MASK_8_SHIFT_RIGHT(*(raw + 1), 16));
        decoded->effects[3] = effect(MASK_5_SHIFT_RIGHT(*raw, 27), MASK_8_SHIFT_RIGHT(*(raw + 1), 24));
        return EVENT_SIZE_MULTIPLE_EFFECT;
    }
    else
    {
        decoded->effects[0] = effect(MASK_5_SHIFT_RIGHT(*raw, 12), MASK_8_SHIFT_RIGHT(*raw, 24));
        decoded->effects[1] = effect(0, 0);
        decoded->effects[2] = effect(0, 0);
        decoded->effects[3] = effect(0, 0);
        return EVENT_SIZE_SINGLE_EFFECT;
    }
}

module_t read_desktop_tracker_module(mapped_file_t file)
{
    module_t module = create_module(file.addr);
    void *positions = file.addr + sizeof(dtt_file_format_t);
    void *pattern_offsets = positions + ALIGN_TO_WORD(module.tune_length);
    void *pattern_lengths = pattern_offsets + (module.num_patterns * sizeof(__uint32_t));
    void *samples = pattern_lengths + ALIGN_TO_WORD(module.num_patterns);
    module.sequence = convert_int_array(positions, module.tune_length);
    set_pattern_starts(&module, pattern_offsets, file.addr);
    module.pattern_lengths = convert_int_array(pattern_lengths, module.num_patterns);
    module.samples = get_samples(module.num_samples, samples, file.addr);
    return module;
}

static double *calculate_gain_curve()
{
    double *gain_curve = allocate_array(INTERNAL_GAIN_MAX + 1, sizeof(double));
    for (int i = 0; i <= 127; i++)
    {
        gain_curve[(i * 2) + 1] = mu_law_to_linear(255 - i);
        if (i >= 1)
            gain_curve[i * 2] = (gain_curve[(i * 2) - 1] + gain_curve[(i * 2) + 1]) / 2;
    }
    gain_curve[0] = 0.0;
    gain_curve[1] = gain_curve[2] / 2;
    return gain_curve;
}

static module_t create_module(dtt_file_format_t *file_format)
{
    module_t module;
    memset(&module, 0, sizeof(module_t));
    module.format = DESKTOP_TRACKER_FORMAT;
    module.decode_event = decode_desktop_tracker_event;
    strncpy(module.name, file_format->name, MAX_LEN_TUNENAME_DSKT);
    strncpy(module.author, file_format->author, MAX_LEN_AUTHOR_DSKT);
    module.num_channels = file_format->num_channels;
    module.tune_length = file_format->tune_length;
    module.initial_panning = convert_int_array(file_format->initial_stereo, module.num_channels);
    module.initial_speed = file_format->initial_speed;
    module.num_patterns = file_format->num_patterns;
    module.num_samples = file_format->num_samples;
    module.gain_maximum = MODULE_GAIN_MAX;
    module.gain_curve = calculate_gain_curve();
    return module;
}

static int *convert_int_array(const uint8_t *unsigned_bytes, int num_elements)
{
    int *int_array = allocate_array(num_elements, sizeof(int));
    for (int i = 0; i < num_elements; i++)
        int_array[i] = unsigned_bytes[i];
    return int_array;
}

static void set_pattern_starts(module_t *module, __uint32_t *pattern_offsets, void *base_address)
{
    for (int i = 0; i < module->num_patterns; i++)
        module->patterns[i] = base_address + pattern_offsets[i];
}

static sample_t *get_samples(int num_samples, dtt_sample_format_t *file_samples, void *base_address)
{
    sample_t *samples = allocate_array(num_samples, sizeof(sample_t));
    memset(samples, 0, sizeof(sample_t) * num_samples);
    for (int i = 0; i < num_samples; i++)
    {
        strncpy(samples[i].name, file_samples[i].name, MAX_LEN_SAMPLENAME_DSKT);
        samples[i].transpose = 26 - file_samples[i].note;
        samples[i].default_gain = file_samples[i].volume;
        samples[i].repeat_offset = file_samples[i].repeat_offset;
        samples[i].repeat_length = file_samples[i].repeat_length;
        samples[i].sample_length = file_samples[i].sample_length;
        uint8_t *sample_data_mu_law = base_address + file_samples[i].sample_data_offset;
        samples[i].sample_data = convert_vidc_encoded_sample(sample_data_mu_law, samples[i].sample_length);
        samples[i].repeats = (samples[i].repeat_length != 0);
    }
    return samples;
}