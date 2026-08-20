#include <string.h>
#include "format_desktop_tracker.h"
#include "loader.h"
#include "memory/bits.h"
#include "memory/heap.h"
#include "pcm/mu_law.h"
#include "io/error.h"

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
static const uint8_t VOLUME_VALUE_MASK = 0x7f;

static const uint8_t ARPEGGIO_COMMAND = 0x0;
static const uint8_t PORTUP_COMMAND = 0x1;
static const uint8_t PORTDOWN_COMMAND = 0x2;
static const uint8_t TONEPORT_COMMAND = 0x3;
// static const uint8_t VIBRATO_COMMAND = 0x4; not implemented yet
// static const uint8_t DELAYEDNOTE_COMMAND = 0x5; not implemented yet
// static const uint8_t RELEASESAMP_COMMAND = 0x6; not implemented yet
// static const uint8_t TREMOLO_COMMAND = 0x7; not implemented yet
// static const uint8_t PHASOR_COMMAND1 = 0x8; not implemented yet
// static const uint8_t PHASOR_COMMAND2 = 0x9; not implemented yet
static const uint8_t VOLSLIDE_COMMAND = 0xa;
static const uint8_t JUMP_COMMAND = 0xb;
static const uint8_t VOLUME_COMMAND = 0xc;
static const uint8_t STEREO_COMMAND = 0xd;
// static const uint8_t STEREOSLIDE_COMMAND = 0xe; not implemented yet
static const uint8_t SPEED_COMMAND = 0xf;
// static const uint8_t ARPEGGIOSPEED_COMMAND = 0x10; not implemented yet
static const uint8_t FINEPORTAMENTO_COMMAND = 0x11;
// static const uint8_t CLEAREPEAT_COMMAND = 0x12; not implemented yet
// static const uint8_t SETVIBRATOWAVEFORM_COMMAND = 0x14; not implemented yet
// static const uint8_t LOOP_COMMAND = 0x16; not implemented yet
// static const uint8_t SETTREMOLOWAVEFORM_COMMAND = 0x17; not implemented yet
static const uint8_t SETFINETEMPO_COMMAND = 0x18;
// static const uint8_t RETRIGGERSAMPLE_COMMAND = 0x19; not implemented yet
static const uint8_t FINEVOLSLIDE_COMMAND = 0x1a;
// static const uint8_t HOLD_COMMAND = 0x1b; not implemented yet
// static const uint8_t NOTECUT_COMMAND = 0x1c; not implemented yet
// static const uint8_t NOTEDELAY_COMMAND = 0x1d; not implemented yet
// static const uint8_t PATTERNDELAY_COMMAND = 0x1e; not implemented yet
// static const uint8_t CALLLINKEDCODE_COMMAND = 0x1f; not implemented yet

typedef struct
{
    uint32_t identifier;
    char name[MAX_LEN_TUNENAME_DSKT];
    char author[MAX_LEN_AUTHOR_DSKT];
    uint32_t flags;
    uint32_t num_tracks;
    uint32_t tune_length;
    uint8_t initial_stereo[8];
    uint32_t initial_speed;
    uint32_t restart;
    uint32_t num_patterns;
    uint32_t num_samples;
} dtt_file_format_t;

typedef struct
{
    uint8_t note;
    uint8_t volume;
    uint16_t unused;
    uint32_t period;
    uint32_t sustain_start;
    uint32_t sustain_end;
    uint32_t repeat_offset;
    uint32_t repeat_length;
    uint32_t sample_length;
    char name[MAX_LEN_SAMPLENAME_DSKT];
    uint32_t sample_data_offset;
} dtt_sample_format_t;

static const int PANNING[] = {1, 43, 86, 128, 170, 213, 255};

static bool is_desktop_tracker_format(mapped_file_t);
static module_t *read_desktop_tracker_module(mapped_file_t);
static bool decode_dtt_patterns(uint8_t *, const uint32_t *, module_t *, const int *);
static size_t decode_desktop_tracker_event(const uint8_t *, event_t *);
static effect_t effect(uint8_t, uint8_t);
static command_t desktop_tracker_command(uint8_t, uint8_t);
static void copy_int_array(const uint8_t *, int *, int);
static bool get_samples(module_t *, dtt_sample_format_t *, uint8_t *);

format_t desktop_tracker_format(void)
{
    format_t format_reader = {
        .is_this_format = is_desktop_tracker_format,
        .read_module = read_desktop_tracker_module,
        .write_module = NULL,
    };
    return format_reader;
}

static bool is_desktop_tracker_format(mapped_file_t file)
{
    return memcmp(file.addr, DTT_FILE_IDENTIFIER, strlen(DTT_FILE_IDENTIFIER)) == 0;
}

static module_t *read_desktop_tracker_module(mapped_file_t file)
{
    module_t *module = NULL;
    int *pattern_lengths = NULL;
    dtt_file_format_t *file_format = (dtt_file_format_t *) file.addr;
    if (file_format->num_tracks < 1 || file_format->num_tracks > 16)
    {
        error("Modfile corrupt: invalid number of tracks");
        goto fail;
    }
    module = module_create(file_format->num_tracks, file_format->tune_length, file_format->num_patterns, file_format->num_samples);
    if (module == NULL)
        goto fail;
    module->format = DESKTOP_TRACKER_FORMAT;
    module->initial_ticks_per_event = file_format->initial_speed;
    module->master_gain = 0.25f;
    module->default_pattern_length = 64;
    module->interpolation_type = NONE;
    strncpy(module->name, file_format->name, MAX_LEN_TUNENAME_DSKT);
    strncpy(module->author, file_format->author, MAX_LEN_AUTHOR_DSKT);
    for (int track = 0; track < module->num_tracks; track++)
    {
        const uint8_t track_panning = file_format->initial_stereo[track];
        if (track_panning == 0 || track_panning > 7)
        {
            module->tracks[track].panning = 128;
            continue;
        }
        module->tracks[track].panning = PANNING[track_panning - 1];
        module->tracks[track].muted = false;
        module->tracks[track].effects_displayed = 1;
    }
    uint8_t *positions_start = file.addr + sizeof(dtt_file_format_t);
    copy_int_array(positions_start, module->sequence, module->sequence_length);
    uint8_t *pattern_offsets_start = positions_start + ALIGN_TO_WORD(module->sequence_length);
    uint8_t *pattern_lengths_start = pattern_offsets_start + (module->num_patterns * sizeof(uint32_t));
    pattern_lengths = allocate_array(MODULE, module->num_patterns, sizeof(int));
    if (pattern_lengths == NULL)
        goto fail;
    copy_int_array(pattern_lengths_start, pattern_lengths, module->num_patterns);
    if (!decode_dtt_patterns(file.addr, (uint32_t *) pattern_offsets_start, module, pattern_lengths))
        goto fail;
    uint8_t *samples_start = pattern_lengths_start + ALIGN_TO_WORD(module->num_patterns);
    if (!get_samples(module, (dtt_sample_format_t *) samples_start, file.addr))
        goto fail;
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

static bool decode_dtt_patterns(uint8_t *base_address, const uint32_t *pattern_offsets, module_t *module, const int *pattern_lengths)
{
    for (int pno = 0; pno < module->num_patterns; pno++)
    {
        const int pattern_length = pattern_lengths[pno];
        if (!module_create_pattern(module, pno, pattern_length))
        {
            return false;
        }
        uint8_t *raw_pattern_data = base_address + pattern_offsets[pno];
        for (int line = 0; line < pattern_length; line++)
        {
            for (uint32_t track = 0; track < module->track_capacity; track++)
            {
                const uint32_t event_index = (line * module->track_capacity) + track;
                event_t *event = module->patterns[pno].events + event_index;
                if (track < (uint32_t) module->num_tracks)
                    raw_pattern_data += decode_desktop_tracker_event(raw_pattern_data, event);
                else
                    *event = (event_t) {0};
            }
        }
    }
    return true;
}

static size_t decode_desktop_tracker_event(const uint8_t *event_p, event_t *decoded)
{
    const uint32_t *raw = (uint32_t *) event_p;
    decoded->instrument_no = MASK_6_SHIFT_RIGHT(*raw, 0);
    decoded->note = MASK_6_SHIFT_RIGHT(*raw, 6);
    if (IS_MULTIPLE_EFFECT(*raw))
    {
        decoded->effects[0] = effect(MASK_5_SHIFT_RIGHT(*raw, 12), MASK_8_SHIFT_RIGHT(*(raw + 1), 0));
        decoded->effects[1] = effect(MASK_5_SHIFT_RIGHT(*raw, 17), MASK_8_SHIFT_RIGHT(*(raw + 1), 8));
        decoded->effects[2] = effect(MASK_5_SHIFT_RIGHT(*raw, 22), MASK_8_SHIFT_RIGHT(*(raw + 1), 16));
        decoded->effects[3] = effect(MASK_5_SHIFT_RIGHT(*raw, 27), MASK_8_SHIFT_RIGHT(*(raw + 1), 24));
        return EVENT_SIZE_MULTIPLE_EFFECT;
    }
    decoded->effects[0] = effect(MASK_5_SHIFT_RIGHT(*raw, 12), MASK_8_SHIFT_RIGHT(*raw, 24));
    decoded->effects[1] = effect(0, 0);
    decoded->effects[2] = effect(0, 0);
    decoded->effects[3] = effect(0, 0);
    return EVENT_SIZE_SINGLE_EFFECT;
}

static effect_t effect(const uint8_t code, const uint8_t data)
{
    const char command = desktop_tracker_command(code, data);
    uint8_t effect_data = data;
    if (command == SET_VOLUME)
        effect_data = (data & VOLUME_VALUE_MASK) * 2;
    if (command == CRESCENDO || command == FINE_CRESCENDO)
    {
        // DSKT volume slide parameter has twice the resolution of the equivalent Tracker effect.
        effect_data = (data * 2);
    }
    if (command == DECRESCENDO || command == FINE_DECRESCENDO)
    {
        // DSKT volume slide command data is a signed integer: negative values slide the volume down.
        const int amount = (256 - data) * 2;
        effect_data = amount > UINT8_MAX ? UINT8_MAX : amount;
    }
    if (command == SET_PANNING)
    {
        if (data == 0 || data > 7) effect_data = 128; // Pathological value, centre it.
        else effect_data = PANNING[data - 1];
    }
    return (effect_t) {
        .data = effect_data,
        .command = command,
    };
}

static command_t desktop_tracker_command(const uint8_t code, const uint8_t data)
{
    if (code == VOLUME_COMMAND) return SET_VOLUME;
    if (code == SPEED_COMMAND) return SET_TEMPO;
    if (code == STEREO_COMMAND) return SET_PANNING;
    if (code == VOLSLIDE_COMMAND && (data & 0x80) == 0) return CRESCENDO;
    if (code == VOLSLIDE_COMMAND && (data & 0x80) > 0) return DECRESCENDO;
    if (code == PORTUP_COMMAND) return PITCH_SLIDE_UP;
    if (code == PORTDOWN_COMMAND) return PITCH_SLIDE_DOWN;
    if (code == TONEPORT_COMMAND) return PORTAMENTO;
    if (code == JUMP_COMMAND) return SEQUENCE_JUMP;
    if (code == SETFINETEMPO_COMMAND) return SET_TICKS_PER_SECOND;
    if (code == FINEPORTAMENTO_COMMAND) return FINE_PORTAMENTO;
    if (code == FINEVOLSLIDE_COMMAND && (data & 0x80) == 0) return FINE_CRESCENDO;
    if (code == FINEVOLSLIDE_COMMAND && (data & 0x80) > 0) return FINE_DECRESCENDO;
    if (code == ARPEGGIO_COMMAND) return (data == 0) ? NO_EFFECT : ARPEGGIO;
    return NO_EFFECT;
}

static bool get_samples(module_t *module, dtt_sample_format_t *file_samples, uint8_t *base_address)
{
    for (int i = 0; i < module->sample_slots; i++)
    {
        sample_t *sample = &module->samples[i];
        instrument_t *instrument = &module->instruments[i];
        const dtt_sample_format_t file_sample = file_samples[i];
        instrument->repeat_offset = file_sample.repeat_offset;
        instrument->repeat_length = file_sample.repeat_length;
        if (file_sample.repeat_offset + file_sample.repeat_length > file_sample.sample_length)
            sample->sample_length = instrument->repeat_offset + instrument->repeat_length;
        else
            sample->sample_length = file_sample.sample_length;
        instrument->repeats = (instrument->repeat_length != 0);
        const uint8_t *sample_data_mu_law = base_address + file_sample.sample_data_offset;
        float *sample_data = allocate_array(MODULE, sample->sample_length + 2, sizeof(float));
        if (!convert_vidc_encoded_sample(sample_data, sample_data_mu_law, sample->sample_length))
            return false;
        sample->sample_data = sample_data;
        if (sample->sample_length > 0)
        {
            instrument->assigned = true;
            strncpy(instrument->name, file_sample.name, MAX_LEN_SAMPLENAME_DSKT);
            instrument->sample_index = i;
            instrument->transpose = 26 - file_sample.note;
            instrument->default_volume = file_sample.volume * 2;
        }
        else
        {
            instrument->assigned = false;
        }
    }
    return true;
}

static void copy_int_array(const uint8_t *source, int *dest, int num_elements)
{
    for (int i = 0; i < num_elements; i++)
        dest[i] = source[i];
}

