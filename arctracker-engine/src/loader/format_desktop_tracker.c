#include <string.h>
#include "format_desktop_tracker.h"
#include "loader.h"
#include "messages.h"
#include "memory/bits.h"
#include "memory/heap.h"
#include "vidc/vidc.h"
#include "io/error.h"

#define MAX_LEN_TUNENAME_DSKT 64
#define MAX_LEN_AUTHOR_DSKT 64

#define MAX_LEN_SAMPLENAME_DSKT 32

static const char *DESKTOP_TRACKER_FORMAT = "DESKTOP TRACKER";
static const char *DTT_FILE_IDENTIFIER = "DskT";
static const uint8_t VOLUME_VALUE_MASK = 0x7f;

static const uint8_t ARPEGGIO_COMMAND = 0x0;
static const uint8_t PORTUP_COMMAND = 0x1;
static const uint8_t PORTDOWN_COMMAND = 0x2;
static const uint8_t TONEPORT_COMMAND = 0x3;
static const uint8_t VIBRATO_COMMAND = 0x4;
// static const uint8_t DELAYEDNOTE_COMMAND = 0x5; not implemented yet
static const uint8_t RELEASESAMP_COMMAND = 0x6;
static const uint8_t TREMOLO_COMMAND = 0x7;
static const uint8_t PHASOR_COMMAND2 = 0x8;
static const uint8_t PHASOR_COMMAND1 = 0x9;
static const uint8_t VOLSLIDE_COMMAND = 0xa;
static const uint8_t JUMP_COMMAND = 0xb;
static const uint8_t VOLUME_COMMAND = 0xc;
static const uint8_t STEREO_COMMAND = 0xd;
// static const uint8_t STEREOSLIDE_COMMAND = 0xe; not implemented yet
static const uint8_t SPEED_COMMAND = 0xf;
static const uint8_t ARPEGGIOSPEED_COMMAND = 0x10;
static const uint8_t FINEPORTAMENTO_COMMAND = 0x11;
static const uint8_t CLEAREPEAT_COMMAND = 0x12;
static const uint8_t SETVIBRATOWAVEFORM_COMMAND = 0x14;
static const uint8_t LOOP_COMMAND = 0x16;
static const uint8_t SETTREMOLOWAVEFORM_COMMAND = 0x17;
static const uint8_t SETFINETEMPO_COMMAND = 0x18;
static const uint8_t RETRIGGERSAMPLE_COMMAND = 0x19;
static const uint8_t FINEVOLSLIDE_COMMAND = 0x1a;
// static const uint8_t HOLD_COMMAND = 0x1b; not implemented yet
static const uint8_t NOTECUT_COMMAND = 0x1c;
static const uint8_t NOTEDELAY_COMMAND = 0x1d;
static const uint8_t PATTERNDELAY_COMMAND = 0x1e;

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
static bool decode_dtt_patterns(const uint8_t *, const uint32_t *, module_t *, const int *);
static bool decode_desktop_tracker_event(const uint8_t *, instrument_t *, const sample_t *, event_t *, size_t *);
static bool is_single_effect(uint32_t);
static bool decode_single_effect(uint32_t, instrument_t *, const sample_t *, effect_t *);
static bool decode_multiple_effects(const uint32_t *raw, instrument_t *, const sample_t *, effect_t *);
static effect_t effect(uint8_t, uint8_t);
static command_t desktop_tracker_command(uint8_t, uint8_t);
static void copy_int_array(const uint8_t *, int *, int);
static bool get_samples(module_t *, dtt_sample_format_t *, uint8_t *);
static bool find_or_create_sample_slice(uint32_t, instrument_t *, const sample_t *, uint8_t *);

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
    module->volume_mapping_type = VOLUME_ARCHIMEDES;
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
    uint8_t *pattern_offsets_start = positions_start + align_to_word(module->sequence_length);
    uint8_t *pattern_lengths_start = pattern_offsets_start + (module->num_patterns * sizeof(uint32_t));
    pattern_lengths = allocate_array(MODULE, module->num_patterns, sizeof(int));
    if (pattern_lengths == NULL)
        goto fail;
    copy_int_array(pattern_lengths_start, pattern_lengths, module->num_patterns);
    uint8_t *samples_start = pattern_lengths_start + align_to_word(module->num_patterns);
    if (!get_samples(module, (dtt_sample_format_t *) samples_start, file.addr))
        goto fail;
    if (!decode_dtt_patterns(file.addr, (uint32_t *) pattern_offsets_start, module, pattern_lengths))
        goto fail;
    deallocate(MODULE, pattern_lengths);
    return module;

fail:
    if (module != NULL)
        module_destroy(module);
    if (pattern_lengths != NULL)
        deallocate(MODULE, pattern_lengths);
    return NULL;
}

static bool decode_dtt_patterns(const uint8_t *base_address, const uint32_t *pattern_offsets, module_t *module, const int *pattern_lengths)
{
    for (int pno = 0; pno < module->num_patterns; pno++)
    {
        const int pattern_length = pattern_lengths[pno];
        if (!module_create_pattern(module, pno, pattern_length))
        {
            return false;
        }
        const uint8_t *raw_pattern_data = base_address + pattern_offsets[pno];
        for (int line = 0; line < pattern_length; line++)
        {
            for (uint32_t track = 0; track < module->track_capacity; track++)
            {
                const uint32_t event_index = (line * module->track_capacity) + track;
                event_t *event = module->patterns[pno].events + event_index;
                if (track < (uint32_t) module->num_tracks)
                {
                    size_t event_size = 0;
                    if (!decode_desktop_tracker_event(raw_pattern_data, module->instruments, module->samples, event, &event_size))
                    {
                        return false;
                    }
                    raw_pattern_data += event_size;
                }
                else
                    *event = (event_t) {0};
            }
        }
    }
    return true;
}

/*
 * An event in Desktop Tracker may have either one effect slot or four. The data is packed differently accordingly.
 * For one effect slot (32 bits):
 *  0...5 Sample number
 *  6..11 Note
 * 12..16 Effect code
 * 17..23 Unused (zero)
 * 24..31 Effect data
 *
 * For four effect slots (64 bits):
 *  0...5 Sample number
 *  6..11 Note
 * 12..16 Effect 1 code
 * 17..21 Effect 2 code
 * 22..26 Effect 3 code
 * 27..31 Effect 4 code
 * 32..39 Effect 1 data
 * 40..47 Effect 2 data
 * 48..55 Effect 3 data
 * 56..63 Effect 4 data
 */
static bool decode_desktop_tracker_event(const uint8_t *event_p, instrument_t *instruments, const sample_t *samples, event_t *decoded, size_t *event_size)
{
    const uint32_t *raw = (uint32_t *) event_p;
    decoded->instrument_no = (int) mask_6_shift_right(*raw, 0);
    instrument_t *instrument = &instruments[decoded->instrument_no];
    const int note = (int) mask_6_shift_right(*raw, 6);
    decoded->note = note == 0 ? 0 : note + 12;
    if (is_single_effect(*raw))
    {
        *event_size = EVENT_SIZE_SINGLE_EFFECT;
        for (int slot = 1; slot <= 3; slot++) decoded->effects[slot] = effect(0, 0);
        return decode_single_effect(*raw, instrument, samples, &decoded->effects[0]);
    }
    *event_size = EVENT_SIZE_MULTIPLE_EFFECT;
    return decode_multiple_effects(raw, instrument, samples, decoded->effects);
}

/*
 * The Desktop Tracker manual states the recommended method of detecting 1 or 4 effects is thus:
 * ; R0 is event data
 * TST R0,#&1F<<17
 * BEQ is_1_effect
 * BNE is_4_effects
 */
static bool is_single_effect(const uint32_t raw_event)
{
    return (raw_event & 0x1f << 17) == 0;
}

static bool decode_single_effect(const uint32_t raw, instrument_t *instrument, const sample_t *samples, effect_t *decoded_effect)
{
    *decoded_effect = effect(mask_5_shift_right(raw, 12), mask_8_shift_right(raw, 24));
    if (decoded_effect->command == USE_SAMPLE_SLICE)
    {
        const uint32_t required_offset = decoded_effect->data * 256;
        if (!find_or_create_sample_slice(required_offset, instrument, samples, &decoded_effect->data))
        {
            error(TOO_MANY_SAMPLE_SLICES);
            return false;
        }
    }
    return true;
}

static bool decode_multiple_effects(const uint32_t *raw, instrument_t *instrument, const sample_t *samples, effect_t *decoded_effects)
{
    for (int slot = 0; slot <= 3; slot++)
    {
        const int code_offset = 12 + slot * 5;
        const int data_offset = slot * 8;
        const uint8_t code = mask_5_shift_right(raw[0], code_offset);
        const uint8_t data = mask_8_shift_right(raw[1], data_offset);
        effect_t decoded_effect = effect(code, data);
        if (decoded_effect.command != USE_SAMPLE_SLICE)
        {
            decoded_effects[slot] = decoded_effect;
            continue;
        }
        uint8_t most_significant_bits = 0;
        uint8_t least_significant_bits = 0;
        if (slot < 3)
        {
            // Desktop Tracker 0x6 (play end part of sample) command consumes the following effect slot as extra data.
            // The data is packed in this strange way so that it behaves the same way the Protracker 0x9 command does
            // if the 0x6 command is in the last slot, or if the event is single effect.
            const int next_slot = slot + 1;
            most_significant_bits = mask_5_shift_right(raw[0], 12 + next_slot * 5);
            least_significant_bits = mask_8_shift_right(raw[1], next_slot * 8);
        }
        const uint32_t required_offset = least_significant_bits | (uint32_t) data << 8 | (uint32_t) most_significant_bits << 16;
        if (!find_or_create_sample_slice(required_offset, instrument, samples, &decoded_effect.data))
        {
            error(TOO_MANY_SAMPLE_SLICES);
            return false;
        }
        if (slot < 3)
        {
            // Clear the next effect lane and increment the slot number to ensure it is skipped over.
            slot += 1;
            decoded_effects[slot] = effect(0, 0);
        }
    }
    return true;
}

static effect_t effect(const uint8_t code, const uint8_t data)
{
    const command_t command = desktop_tracker_command(code, data);
    uint8_t effect_data = data;
    if (command == SET_VOLUME)
        effect_data = (data & VOLUME_VALUE_MASK) * 2;
    if (command == FINE_CRESCENDO || (command == VOLUME_SLIDE && data < 128))
    {
        // DSKT volume slide parameter has half the resolution of the equivalent Tracker effect.
        effect_data = (data * 2) | 0x80;
    }
    if (command == FINE_DECRESCENDO || (command == VOLUME_SLIDE && data >= 128))
    {
        // DSKT volume slide command data is a signed integer: negative values slide the volume down.
        const int amount = (256 - data) * 2;
        effect_data = amount > UINT8_MAX ? UINT8_MAX : amount;
    }
    if (code == FINE_PORTAMENTO_DOWN)
    {
        effect_data = 256 - data;
    }
    if (command == SET_PANNING)
    {
        if (data == 0 || data > 7) effect_data = 128; // Pathological value, centre it.
        else effect_data = PANNING[data - 1];
    }
    if (command == DELAY_NEXT_EVENT)
    {
        // TODO:
        // Find out whether command 0x1E really delays the next pattern, as it says in the manual,
        // or whether it actually works the same way as the Protracker EEy command (as I suspect).
    }
    return (effect_t) {
        .data = effect_data,
        .command = command,
    };
}

static command_t desktop_tracker_command(const uint8_t code, const uint8_t data)
{
    if (code == ARPEGGIO_COMMAND) return (data == 0) ? NO_EFFECT : ARPEGGIO;
    if (code == PORTUP_COMMAND) return PITCH_SLIDE_UP;
    if (code == PORTDOWN_COMMAND) return PITCH_SLIDE_DOWN;
    if (code == TONEPORT_COMMAND) return PORTAMENTO;
    if (code == VIBRATO_COMMAND) return VIBRATO;
    if (code == RELEASESAMP_COMMAND) return USE_SAMPLE_SLICE;
    if (code == TREMOLO_COMMAND) return TREMOLO;
    if (code == PHASOR_COMMAND2) return FINE_ADVANCE_PHASE;
    if (code == PHASOR_COMMAND1) return ADVANCE_PHASE;
    if (code == VOLSLIDE_COMMAND) return VOLUME_SLIDE;
    if (code == JUMP_COMMAND) return SEQUENCE_JUMP;
    if (code == VOLUME_COMMAND) return SET_VOLUME;
    if (code == STEREO_COMMAND) return SET_PANNING;
    if (code == SPEED_COMMAND) return SET_TEMPO;
    if (code == ARPEGGIOSPEED_COMMAND) return SET_ARPEGGIO_SPEED;
    if (code == FINEPORTAMENTO_COMMAND && (data & 0x80) == 0) return FINE_PORTAMENTO_UP;
    if (code == FINEPORTAMENTO_COMMAND && (data & 0x80) > 0) return FINE_PORTAMENTO_DOWN;
    if (code == CLEAREPEAT_COMMAND) return CLEAR_REPEAT;
    if (code == SETVIBRATOWAVEFORM_COMMAND) return SET_VIBRATO_WAVEFORM;
    if (code == LOOP_COMMAND) return SET_LOOP;
    if (code == SETTREMOLOWAVEFORM_COMMAND) return SET_TREMOLO_WAVEFORM;
    if (code == SETFINETEMPO_COMMAND) return SET_TICKS_PER_SECOND;
    if (code == RETRIGGERSAMPLE_COMMAND) return RETRIGGER_SAMPLE;
    if (code == FINEVOLSLIDE_COMMAND && (data & 0x80) == 0) return FINE_CRESCENDO;
    if (code == FINEVOLSLIDE_COMMAND && (data & 0x80) > 0) return FINE_DECRESCENDO;
    if (code == NOTECUT_COMMAND) return SILENCE_SAMPLE_AFTER_DELAY;
    if (code == NOTEDELAY_COMMAND) return DELAY_SAMPLE;
    if (code == PATTERNDELAY_COMMAND) return DELAY_NEXT_EVENT;
    // Command 0x1F (call linked code) is, obviously, impossible to implement.
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
        if (sample_data == NULL) return false;
        for (int s = 0; s < sample->sample_length; s++)
        {
            sample_data[s] = vidc_to_linear(sample_data_mu_law[s]);
        }
        sample->sample_data = sample_data;
        const float period = (float) file_sample.period / 4096; // Period is stored as 20.12 fixed point.
        sample->sample_rate = (float) VIDC_SYSTEM_CLOCK_MULTIPLE / period;
        sample->base_note = file_sample.note + 11;
        sample->finetune = 0;
        if (sample->sample_length > 0)
        {
            instrument->assigned = true;
            strncpy(instrument->name, file_sample.name, MAX_LEN_SAMPLENAME_DSKT);
            instrument->sample_index = i;
            instrument->transpose = 0;
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

static bool find_or_create_sample_slice(const uint32_t required_offset, instrument_t *instrument, const sample_t *samples, uint8_t *slice_index)
{
    if (!instrument->assigned)
    {
        // Don't worry about it because it won't be played anyway.
        *slice_index = 0;
        return true;
    }
    for (int candidate = 0; candidate <= 255; candidate++)
    {
        const sample_slice_t *slice = &instrument->sample_slices[candidate];
        if (slice->length > 0 && slice->offset == required_offset)
        {
            *slice_index = candidate;
            return true;
        }
    }
    //
    // No slice with the required offset exists yet, create one if possible.
    //
    const int sample_length = samples[instrument->sample_index].sample_length;
    if (required_offset >= sample_length)
    {
        // TODO: Figure out what Desktop Tracker does in this case.
        // Use that to inform the correct behaviour here.
    }
    for (int candidate = 0; candidate <= 255; candidate++)
    {
        sample_slice_t *slice = &instrument->sample_slices[candidate];
        if (slice->length == 0)
        {
            slice->offset = required_offset;
            slice->length = sample_length - required_offset;
            *slice_index = candidate;
            return true;
        }
    }
    return false;
}
