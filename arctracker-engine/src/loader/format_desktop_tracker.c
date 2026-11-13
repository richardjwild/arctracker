#include "format_desktop_tracker.h"
#include <assert.h>
#include "messages.h"
#include "io/error.h"
#include "memory/bits.h"
#include "memory/heap.h"
#include "vidc/vidc.h"

/******************************************************************************
 * The format of a Desktop Tracker module file is as follows. All multi-byte  *
 * numbers are stored in little-endian order:                                 *
 *                                                                            *
 * Offset Description                                                         *
 * ------ -----------                                                         *
 *      0 File identifier (4cc = "DskT")                                      *
 *      4 Module title (64 byte NUL-terminated)                               *
 *     68 Author name (64 byte NUL-terminated)                                *
 *    132 Flags (4 bytes):                                                    *
 *          bit 0 - linear volume (set), logarithmic volume (unset)           *
 *          bit 1 - pitch-relative portamento (set), Amiga portamento (unset) *
 *    136 Track count (4 bytes)                                               *
 *    140 Sequence length (4 bytes)                                           *
 *    144 Initial stereo positions (8 bytes, one byte per audio channel):     *
 *          1 - full left                                                     *
 *          4 - centre                                                        *
 *          7 - full right                                                    *
 *    152 Initial ticks per event (4 bytes)                                   *
 *    156 Song restart position (4 bytes)                                     *
 *    160 Pattern count (4 bytes)                                             *
 *    164 Sample count (4 bytes)                                              *
 *                                                                            *
 * Song sequence                                                              *
 * -------------                                                              *
 * At offset 168 begins the song sequence. This is an array of bytes equal in *
 * length to the sequence length given at offset 140. The byte at index n in  *
 * the array gives the number of the pattern played at sequence position n in *
 * the song. The array is zero-indexed.                                       *
 *                                                                            *
 * Pattern offsets                                                            *
 * ---------------                                                            *
 * The pattern offsets follow the song sequence, aligned to the next 4-byte   *
 * boundary. This is an array of 4-byte words equal in length to the pattern  *
 * count given at offset 160. The value of the array at index n gives the     *
 * offset into the file at which the data for pattern number n is located.    *
 * The array is zero-indexed.                                                 *
 *                                                                            *
 * Pattern lengths                                                            *
 * ---------------                                                            *
 * The pattern lengths follow the pattern offsets, aligned to the next 4-byte *
 * boundary. This is an array of 1-byte values equal in length to the pattern *
 * count given at offset 160. The value of the array at index n gives the     *
 * length (in pattern lines) of pattern number n. The array is zero-indexed.  *
 *                                                                            *
 * Samples                                                                    *
 * -------                                                                    *
 * The samples follow the pattern lengths, aligned to the next 4-byte         *
 * boundary. This is an array of structures equal in length to the sample     *
 * count given at offset 164. The structure is 64 bytes in length and its     *
 * format is:                                                                 *
 *                                                                            *
 *   Offset Description                                                       *
 *   ------ -----------                                                       *
 *        0 Base note (1 byte): 0 - C1, 35 - B3                               *
 *        1 Default volume (1 byte): 0 - minimum, 127 - maximum               *
 *        2 Unused (2 bytes)                                                  *
 *        4 Sample period (4 bytes): stored as 20.12 fixed point              *
 *        8 Sustain start (4 bytes)                                           *
 *       12 Sustain end (4 bytes)                                             *
 *       16 Repeat offset (4 bytes)                                           *
 *       20 Repeat length (4 bytes)                                           *
 *       24 Sample length (4 bytes)                                           *
 *       28 Sample name (32 bytes NUL-terminated)                             *
 *       60 Sample data offset (4 bytes)                                      *
 *                                                                            *
 * The sample data offset is the offset into the file at which the sample     *
 * data is located. The sample data is 8-bit VIDC encoded (see vidc.c for     *
 * details, in particular the vidc_to_linear function).                       *
 *                                                                            *
 * Pattern data                                                               *
 * ------------                                                               *
 * The pattern data is an array of events in row-major order, i.e.: row 0     *
 * track 0, row 0 track 1, row 0 track 2, row 0 track 3, row 1 track 0, etc.  *
 * An event may have one effect slot or four effect slots, and the data is    *
 * packed differently accordingly.                                            *
 *   For one effect slot (32 bits):                                           *
 *    0...5 Sample number                                                     *
 *    6..11 Note                                                              *
 *   12..16 Effect code                                                       *
 *   17..23 Unused (zero)                                                     *
 *   24..31 Effect data                                                       *
 *                                                                            *
 *   For four effect slots (64 bits):                                         *
 *    0...5 Sample number                                                     *
 *    6..11 Note                                                              *
 *   12..16 Effect 1 code                                                     *
 *   17..21 Effect 2 code                                                     *
 *   22..26 Effect 3 code                                                     *
 *   27..31 Effect 4 code                                                     *
 *   32..39 Effect 1 data                                                     *
 *   40..47 Effect 2 data                                                     *
 *   48..55 Effect 3 data                                                     *
 *   56..63 Effect 4 data                                                     *
 ******************************************************************************/

//
// The Desktop Tracker manual can be found archived (for now) at:
// https://web.archive.org/web/20171019160851/http://www.tribbeck.com/software/sonor/dtt/manual/
//

#define DSKT_FILE_IDENTIFIER "DskT"
#define DSKT_HEADER_SIZE 168
#define DSKT_SAMPLE_META_DATA_SIZE 64
#define DSKT_MODULE_NAME_LEN 64
#define DSKT_AUTHOR_NAME_LEN 64
#define DSKT_SAMPLE_NAME_LEN 32
#define DSKT_MAX_TRACKS 16
#define DSKT_MAX_AUDIO_CHANNELS 8
#define DSKT_MAX_SEQUENCE_LENGTH 4095
#define DSKT_MAX_PATTERNS 256
#define DSKT_MAX_SAMPLES 63
#define DSKT_MAX_SPEED 255
#define DSKT_NOTE_C1 0
#define DSKT_NOTE_B3 35
#define DSKT_MAX_VOLUME 127
#define DSKT_EVENT_SIZE_SINGLE_EFFECT 4
#define DSKT_EVENT_SIZE_MULTIPLE_EFFECT 8
#define DSKT_ARPEGGIO_CMD 0x0
#define DSKT_PORTAMENTO_UP_CMD 0x1
#define DSKT_PORTAMENTO_DOWN_CMD 0x2
#define DSKT_TONE_PORTAMENTO_CMD 0x3
#define DSKT_VIBRATO_CMD 0x4
#define DSKT_RELEASE_SAMPLE_CMD 0x6
#define DSKT_TREMOLO_CMD 0x7
#define DSKT_PHASOR_2_CMD 0x8
#define DSKT_PHASOR_CMD_1 0x9
#define DSKT_VOLUME_SLIDE_CMD 0xa
#define DSKT_JUMP_CMD 0xb
#define DSKT_SET_VOLUME_CMD 0xc
#define DSKT_SET_STEREO_CMD 0xd
#define DSKT_STEREO_SLIDE_CMD 0xe
#define DSKT_SET_SPEED_CMD 0xf
#define DSKT_SET_ARPEGGIO_SPEED_CMD 0x10
#define DSKT_FINE_PORTAMENTO_CMD 0x11
#define DSKT_CLEAR_REPEAT_CMD 0x12
#define DSKT_SET_VIBRATO_WAVEFORM_CMD 0x14
#define DSKT_SET_LOOP_CMD 0x16
#define DSKT_SET_TREMOLO_WAVEFORM_CMD 0x17
#define DSKT_SET_FINE_TEMPO_CMD 0x18
#define DSKT_RETRIGGER_SAMPLE_CMD 0x19
#define DSKT_FINE_VOLUME_SLIDE_CMD 0x1a
#define DSKT_NOTE_CUT_CMD 0x1c
#define DSKT_NOTE_DELAY_CMD 0x1d
#define DSKT_PATTERN_DELAY_CMD 0x1e

static const int PANNING[] = {1, 43, 86, 128, 170, 213, 255};

static const instrument_t UNASSIGNED_INSTRUMENT = {
    .assigned = false,
};

typedef struct {
    const uint8_t *file_start;
    const uint8_t *sequence;
    const uint8_t *pattern_offsets;
    const uint8_t *pattern_lengths;
    const uint8_t *sample_definitions;
    const uint8_t *file_end;
} format_map_t;

typedef struct {
    uint8_t pattern_no;
    const uint8_t *pattern_data;
    uint8_t pattern_length;
    const uint8_t *file_end;
} pattern_read_context_t;

static bool is_desktop_tracker_format(mapped_file_t);
static module_t *read_desktop_tracker_module(mapped_file_t);
static module_t *init_module(mapped_file_t);
static void init_tracks(module_t *, uint32_t, const uint8_t *);
static bool get_format_map(mapped_file_t, const module_t *, format_map_t *);
static bool read_sequence(module_t *, format_map_t);
static bool read_samples(module_t *, format_map_t);
static bool read_sample(module_t *, format_map_t, const uint8_t *, int);
static bool read_patterns(module_t *, format_map_t);
static bool read_pattern(module_t *, pattern_read_context_t);
static bool decode_desktop_tracker_event(pattern_read_context_t, instrument_t *, const sample_t *, event_t *, size_t *);
static bool is_single_effect(uint32_t);
static effect_t effect(uint8_t, uint8_t);
static bool decode_single_effect(uint32_t, instrument_t *, const sample_t *, effect_t *);
static bool decode_multiple_effects(pattern_read_context_t, instrument_t *, const sample_t *, effect_t *);
static bool find_or_create_sample_slice(uint32_t, instrument_t *, const sample_t *, uint8_t *);

format_t desktop_tracker_format_2(void)
{
    return (format_t) {
        .is_this_format = is_desktop_tracker_format,
        .read_module = read_desktop_tracker_module,
        .write_module = NULL, // Writing is not supported for this format.
    };
}

static bool is_desktop_tracker_format(const mapped_file_t file)
{
    return file.size >= 4 && memcmp(DSKT_FILE_IDENTIFIER, file.addr, 4) == 0;
}

static module_t *read_desktop_tracker_module(const mapped_file_t file)
{
    module_t *module = init_module(file);
    if (module == NULL)
    {
        return NULL;
    }
    format_map_t format_map;
    if (!get_format_map(file, module, &format_map))
    {
        goto failed;
    }
    if (!read_sequence(module, format_map) || !read_samples(module, format_map) || !read_patterns(module, format_map))
    {
        goto failed;
    }
    return module;

failed:
    module_destroy(module);
    return NULL;
}

static module_t *init_module(const mapped_file_t file)
{
    if (file.size < DSKT_HEADER_SIZE)
    {
        return NULL;
    }
    const char *module_name_offset = (char *) file.addr + 4;
    const char *author_name_offset = (char *) file.addr + 4 + DSKT_MODULE_NAME_LEN;
    const uint8_t *header_offset = file.addr + 4 + DSKT_MODULE_NAME_LEN + DSKT_AUTHOR_NAME_LEN;
    //
    // Read the module header and validate what we find there.
    //
    const uint32_t flags = read_u32_le(header_offset);
    const int32_t track_count = read_s32_le(header_offset + 4);
    if (track_count < 1 || track_count > DSKT_MAX_TRACKS)
    {
        error(INVALID_TRACK_COUNT);
        return NULL;
    }
    const int32_t sequence_length = read_s32_le(header_offset + 8);
    if (sequence_length < 1 || sequence_length > DSKT_MAX_SEQUENCE_LENGTH)
    {
        error(INVALID_SEQUENCE_LENGTH);
        return NULL;
    }
    uint8_t initial_stereo[DSKT_MAX_AUDIO_CHANNELS];
    for (int track = 0; track < DSKT_MAX_AUDIO_CHANNELS; track++)
    {
        initial_stereo[track] = read_u8(header_offset + 12 + track);
    }
    const int32_t initial_ticks_per_event = read_s32_le(header_offset + 20);
    if (initial_ticks_per_event < 1 || initial_ticks_per_event >= MIN_BPM)
    {
        // TODO:
        // Desktop Tracker's actual maximum speed value is 255, but this can't be represented in Arctracker currently
        // because Arctracker interprets a speed value of 33 or greater to be a BPM value, not a ticks/event value.
        // (I should probably drop that bit of ProTracker baggage and separate the commands for BPM and ticks/event).
        error(MODFILE_INVALID_INITIAL_SPEED);
        return NULL;
    }
    int32_t restart_position = read_s32_le(header_offset + 24);
    if (restart_position < 0 || restart_position >= sequence_length)
    {
        restart_position = 0;
    }
    const int32_t pattern_count = read_s32_le(header_offset + 28);
    if (pattern_count < 1 || pattern_count > DSKT_MAX_PATTERNS)
    {
        error(INVALID_PATTERN_COUNT);
        return NULL;
    }
    const int32_t sample_count = read_s32_le(header_offset + 32);
    if (sample_count < 0 || sample_count > DSKT_MAX_SAMPLES)
    {
        error(INVALID_SAMPLE_COUNT);
        return NULL;
    }
    //
    // If we reached here, everything looks good so far. We can now try to create the module structure.
    //
    module_t *module = module_create(track_count, sequence_length, pattern_count, sample_count);
    if (module == NULL)
    {
        return NULL;
    }
    snprintf(module->name, DSKT_MODULE_NAME_LEN, "%s", module_name_offset);
    snprintf(module->author, DSKT_AUTHOR_NAME_LEN, "%s", author_name_offset);
    module->initial_ticks_per_event = initial_ticks_per_event;
    module->volume_mapping_type = flags & 1 ? VOLUME_AMIGA : VOLUME_ARCHIMEDES;
    module->relative_pitch_bend = (flags & 2) == 0;
    module->master_gain = 0.25f;
    module->default_pattern_length = 64;
    module->interpolation_type = NONE;
    module->restart_position = restart_position;
    init_tracks(module, track_count, initial_stereo);
    return module;
}

static void init_tracks(module_t *module, const uint32_t track_count, const uint8_t *initial_stereo)
{
    assert(module->tracks != NULL);
    for (uint32_t track = 0; track < track_count; track++)
    {
        module->tracks[track].muted = false;
        module->tracks[track].effects_displayed = 1;
        //
        // The DskT format has up to eight initial stereo positions, because the Archimedes had eight audio channels.
        // However, Desktop Tracker could play up to sixteen tracks by time multiplexing two tracks on a single audio
        // channel. Therefore, the stereo positions of tracks 9-16 follow those of tracks 1-8.
        //
        if (track < DSKT_MAX_AUDIO_CHANNELS)
        {
            const uint8_t initial_track_stereo = initial_stereo[track];
            if (initial_track_stereo == 0 || initial_track_stereo > 7)
                module->tracks[track].panning = 128; // Centre.
            else
                module->tracks[track].panning = PANNING[initial_track_stereo - 1];
        }
        else
        {
            module->tracks[track].panning = module->tracks[track - DSKT_MAX_AUDIO_CHANNELS].panning;
        }
    }
}

static bool get_format_map(const mapped_file_t file, const module_t *module, format_map_t *format_map)
{
    const size_t sequence = DSKT_HEADER_SIZE;
    const size_t pattern_offsets = sequence + align_to_4byte(module->sequence_length);
    const size_t pattern_lengths = pattern_offsets + module->num_patterns * sizeof(uint32_t);
    const size_t sample_definitions = pattern_lengths + align_to_4byte(module->num_patterns);
    const size_t sample_definitions_length = module->sample_slots * DSKT_SAMPLE_META_DATA_SIZE;
    if (sample_definitions > file.size || sample_definitions_length > file.size - sample_definitions)
    {
        error("Map too big for file!");
        return false;
    }
    *format_map = (format_map_t) {
        .file_start = file.addr,
        .sequence = file.addr + sequence,
        .pattern_offsets = file.addr + pattern_offsets,
        .pattern_lengths =  file.addr + pattern_lengths,
        .sample_definitions = file.addr + sample_definitions,
        .file_end = file.addr + file.size,
    };
    return true;
}

static bool read_sequence(module_t *module, const format_map_t format_map)
{
    assert(module->sequence != NULL);
    const uint8_t *sequence_in = format_map.sequence;
    for (int sequence_index = 0; sequence_index < module->sequence_length; sequence_index++)
    {
        const uint8_t pattern_no = sequence_in[sequence_index];
        if (pattern_no < module->num_patterns)
        {
            module->sequence[sequence_index] = (int) pattern_no;
        }
        else
        {
            error(INVALID_PATTERN_NUMBER);
            return false;
        }
    }
    return true;
}

static bool read_samples(module_t *module, const format_map_t format_map)
{
    assert(module->samples != NULL);
    assert(module->instruments != NULL);
    const uint8_t *sample_definition = format_map.sample_definitions;
    for (int sample_index = 0; sample_index < module->sample_slots; sample_index++)
    {
        if (!read_sample(module, format_map, sample_definition, sample_index))
        {
            return false;
        }
        sample_definition += DSKT_SAMPLE_META_DATA_SIZE;
    }
    return true;
}

static bool read_sample(module_t *module, const format_map_t format_map, const uint8_t *sample_definition, const int sample_index)
{
    const uint8_t base_note = read_u8(sample_definition);
    if (base_note > DSKT_NOTE_B3)
    {
        error(INVALID_BASE_NOTE);
        return false;
    }
    const uint8_t volume = read_u8(sample_definition + 1);
    if (volume > DSKT_MAX_VOLUME)
    {
        error(MODFILE_INVALID_VOLUME);
        return false;
    }
    //
    // Bytes 2,3 are unused.
    //
    const uint32_t period_in = read_u32_le(sample_definition + 4);
    if (period_in == 0)
    {
        error(MODFILE_CORRUPT_INVALID_SAMPLE_PERIOD);
        return false;
    }
    //
    // Sustain start and end are ignored by Arctracker.
    //
    const uint32_t repeat_start = read_u32_le(sample_definition + 16);
    const uint32_t repeat_length = read_u32_le(sample_definition + 20);
    const uint32_t sample_length = read_u32_le(sample_definition + 24);
    if (sample_length == 0)
    {
        // Sample length zero means the sample slot is unused, so mark the instrument unassigned and return.
        module_set_instrument(module, sample_index, UNASSIGNED_INSTRUMENT);
        return true;
    }
    const size_t sample_data_offset = read_u32_le(sample_definition + 60);
    if (sample_data_offset > (size_t) format_map.file_end - sample_length)
    {
        error(MODFILE_CORRUPT_MISSING_SAMPLE_DATA);
        return false;
    }
    const bool repeats = repeat_length > 0;
    const uint8_t *sample_data_in = format_map.file_start + sample_data_offset;
    float *sample_data = allocate_array(MODULE, sample_length + 2, sizeof(float));
    if (sample_data == NULL)
    {
        return false;
    }
    for (uint32_t sample = 0; sample < sample_length; sample++)
    {
        sample_data[sample] = vidc_to_linear(sample_data_in[sample]);
    }
    const float period = (float) period_in / 4096;
    const float sample_rate = (float) VIDC_SYSTEM_CLOCK_MULTIPLE / period;
    if (!module_set_sample(module, sample_data, (int) sample_length, sample_rate, base_note + 11, 0, sample_index))
    {
        deallocate(MODULE, sample_data);
        return false;
    }
    //
    // Arctracker does not require a 1:1 relationship between samples and instruments, but Desktop Tracker has no
    // concept of an instrument, so for a DskT format file, sample and instrument are synonyms.
    //
    instrument_t instrument = {
        .assigned = true,
        .default_volume = volume * 2,
        .transpose = 0,
        .repeats = repeats,
        .repeat_offset = (int) repeat_start,
        .repeat_length = (int) repeat_length,
        .sample_index = sample_index,
    };
    snprintf(instrument.name, DSKT_SAMPLE_NAME_LEN, "%s", (char *) sample_definition + 28);
    module_set_instrument(module, sample_index, instrument);
    return true;
}

static command_t map_dskt_command(const uint8_t code, const uint8_t data)
{
    if (code == DSKT_ARPEGGIO_CMD) return data == 0 ? NO_EFFECT : CHIPTUNE_ARPEGGIO;
    if (code == DSKT_PORTAMENTO_UP_CMD) return PORTAMENTO_UP;
    if (code == DSKT_PORTAMENTO_DOWN_CMD) return PORTAMENTO_DOWN;
    if (code == DSKT_TONE_PORTAMENTO_CMD) return TONE_PORTAMENTO;
    if (code == DSKT_VIBRATO_CMD) return VIBRATO;
    // Command 0x5 (delayed note on another track) is not implemented, because it is incompatible with the Arctracker
    // model of globally-scoped versus track-scoped commands.
    if (code == DSKT_RELEASE_SAMPLE_CMD) return USE_SAMPLE_SLICE;
    if (code == DSKT_TREMOLO_CMD) return TREMOLO;
    if (code == DSKT_PHASOR_2_CMD) return ADVANCE_PHASE_FINE;
    if (code == DSKT_PHASOR_CMD_1) return ADVANCE_PHASE;
    if (code == DSKT_VOLUME_SLIDE_CMD) return VOLUME_SLIDE;
    if (code == DSKT_JUMP_CMD) return SEQUENCE_JUMP;
    if (code == DSKT_SET_VOLUME_CMD) return SET_VOLUME;
    if (code == DSKT_SET_STEREO_CMD) return SET_STEREO;
    if (code == DSKT_STEREO_SLIDE_CMD) return DSKT_STEREO_SLIDE;
    if (code == DSKT_SET_SPEED_CMD) return SET_TEMPO;
    if (code == DSKT_SET_ARPEGGIO_SPEED_CMD) return SET_ARPEGGIO_SPEED;
    if (code == DSKT_FINE_PORTAMENTO_CMD && (data & 0x80) == 0) return PORTAMENTO_UP_FINE;
    if (code == DSKT_FINE_PORTAMENTO_CMD && (data & 0x80) > 0) return PORTAMENTO_DOWN_FINE;
    if (code == DSKT_CLEAR_REPEAT_CMD) return RELEASE_SAMPLE_LOOP;
    if (code == DSKT_SET_VIBRATO_WAVEFORM_CMD) return SET_VIBRATO_WAVEFORM;
    if (code == DSKT_SET_LOOP_CMD) return DEFINE_PATTERN_LOOP;
    if (code == DSKT_SET_TREMOLO_WAVEFORM_CMD) return SET_TREMOLO_WAVEFORM;
    if (code == DSKT_SET_FINE_TEMPO_CMD) return SET_TICK_RATE;
    if (code == DSKT_RETRIGGER_SAMPLE_CMD) return RETRIGGER_SAMPLE;
    if (code == DSKT_FINE_VOLUME_SLIDE_CMD && (data & 0x80) == 0) return VOLUME_SLIDE_UP_FINE;
    if (code == DSKT_FINE_VOLUME_SLIDE_CMD && (data & 0x80) > 0) return VOLUME_SLIDE_DOWN_FINE;
    // Command 0x1B (hold) is not implemented, because its behaviour in DTT differs substantially from the manual and
    // empirical testing found behaviour dependent on unrelated playback state, with no sensible semantics to reproduce.
    // This is why the sustain start and end are ignored when we load the sample meta data.
    if (code == DSKT_NOTE_CUT_CMD) return SILENCE_SAMPLE_AFTER_DELAY;
    if (code == DSKT_NOTE_DELAY_CMD) return DELAY_SAMPLE;
    // The manual states the pattern delay command causes the next pattern to be delayed, but its behaviour has been
    // observed in practice to be the same as the Arctracker delay next event command.
    if (code == DSKT_PATTERN_DELAY_CMD) return DELAY_NEXT_EVENT;
    // Command 0x1F (call linked code) is not possible to implement.
    return NO_EFFECT;
}

static bool read_patterns(module_t *module, const format_map_t format_map)
{
    const uint8_t *pattern_offsets = format_map.pattern_offsets;
    const uint8_t *pattern_lengths = format_map.pattern_lengths;
    for (int pattern_no = 0; pattern_no < module->num_patterns; pattern_no++)
    {
        const uint8_t pattern_length = pattern_lengths[pattern_no];
        if (!module_set_pattern_length(module, pattern_no, pattern_length))
        {
            return false;
        }
        const size_t pattern_data_offset = read_u32_le(pattern_offsets + pattern_no * 4);
        if (format_map.file_start > format_map.file_end - pattern_data_offset)
        {
            error(MODFILE_INVALID_MISSING_PATTERN_DATA);
            return false;
        }
        const pattern_read_context_t read_pattern_info = {
            .pattern_no = pattern_no,
            .pattern_data = (uint8_t *) format_map.file_start + pattern_data_offset,
            .pattern_length = pattern_length,
            .file_end = format_map.file_end,
        };
        if (!read_pattern(module, read_pattern_info))
        {
            return false;
        }
    }
    return true;
}

static bool read_pattern(module_t *module, pattern_read_context_t context)
{
    for (int pattern_index = 0; pattern_index < context.pattern_length; pattern_index++)
    {
        for (uint32_t track = 0; track < module->track_capacity; track++)
        {
            const uint32_t event_index = pattern_index * module->track_capacity + track;
            event_t *event = &module->patterns[context.pattern_no].events[event_index];
            if (track < (uint32_t) module->num_tracks)
            {
                size_t event_size = 0;
                if (context.pattern_data > context.file_end - 4)
                {
                    error(MODFILE_INVALID_MISSING_PATTERN_DATA);
                    return false;
                }
                if (!decode_desktop_tracker_event(context, module->instruments, module->samples, event, &event_size))
                {
                    return false;
                }
                context.pattern_data += event_size;
            }
            else
            {
                *event = (event_t) {0};
            }
            int highest_effect_used = 0;
            //
            // Set effects_displayed on the track so that all the effect lanes that are used are visible. We don't
            // check effect lane 0 because our policy is to display at least one lane.
            //
            for (int effect_lane = 1; effect_lane <= 3; effect_lane++)
            {
                if (event->effects[effect_lane].command != NO_EFFECT)
                    highest_effect_used = effect_lane;
            }
            if (highest_effect_used >= module->tracks[track].effects_displayed)
                module->tracks[track].effects_displayed = highest_effect_used + 1;
        }
    }
    return true;
}

static bool decode_desktop_tracker_event(const pattern_read_context_t context, instrument_t *instruments, const sample_t *samples, event_t *decoded, size_t *event_size)
{
    const uint8_t *event_p = context.pattern_data;
    const uint32_t event = read_u32_le(event_p);
    decoded->instrument_no = (int) mask_6_shift_right(event, 0);
    instrument_t *instrument = decoded->instrument_no > 0 ? &instruments[decoded->instrument_no] : NULL;
    const int note = (int) mask_6_shift_right(event, 6);
    decoded->note = note == 0 ? 0 : note + 12;
    if (is_single_effect(event))
    {
        *event_size = DSKT_EVENT_SIZE_SINGLE_EFFECT;
        decoded->effects[1] = effect(0, 0);
        decoded->effects[2] = effect(0, 0);
        decoded->effects[3] = effect(0, 0);
        return decode_single_effect(event, instrument, samples, &decoded->effects[0]);
    }
    *event_size = DSKT_EVENT_SIZE_MULTIPLE_EFFECT;
    return decode_multiple_effects(context, instrument, samples, decoded->effects);
}

/*
 * The Desktop Tracker manual states the recommended method of detecting 1 or 4 effects is thus:
 * ; R0 is event data
 * TST R0,#&1F<<17
 * BEQ is_1_effect
 * BNE is_4_effects
 */
static bool is_single_effect(const uint32_t event)
{
    return (event & 0x1f << 17) == 0;
}

static effect_t effect(const uint8_t code, const uint8_t data)
{
    const command_t command = map_dskt_command(code, data);
    uint8_t effect_data = data;
    //
    // Now we have to fiddle with some of the command parameter values.
    //
    if (command == SET_VOLUME)
    {
        // DSKT volume resolution (0-127) is half that of Arctracker (0-255). (Tracker purports to run from 0 to 255
        // but it does not really: odd numbered values are the same volume as [value-1] but the polarity is inverted.
        // Arctracker actually does have 256 discrete volume values).
        effect_data = (data & DSKT_MAX_VOLUME) * 2;
    }
    if (command == VOLUME_SLIDE_UP_FINE || (command == VOLUME_SLIDE && data < 128))
    {
        // DSKT volume slide parameter has half the resolution of the equivalent Tracker effect.
        effect_data = data * 2 | 0x80;
    }
    if (command == VOLUME_SLIDE_DOWN_FINE || (command == VOLUME_SLIDE && data >= 128))
    {
        // DSKT volume slide command data is a signed integer: negative values slide the volume down.
        const int amount = (256 - data) * 2;
        effect_data = amount > UINT8_MAX ? UINT8_MAX : amount;
    }
    if (command == PORTAMENTO_DOWN_FINE)
    {
        effect_data = 256 - data;
    }
    if (command == SET_STEREO)
    {
        if (data == 0 || data > 7) effect_data = 128; // Pathological value, centre it.
        else effect_data = PANNING[data - 1];
    }
    return (effect_t) {
        .data = effect_data,
        .command = command,
    };
}

static bool decode_single_effect(const uint32_t event, instrument_t *instrument, const sample_t *samples, effect_t *decoded_effect)
{
    *decoded_effect = effect(mask_5_shift_right(event, 12), mask_8_shift_right(event, 24));
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

static bool decode_multiple_effects(const pattern_read_context_t context, instrument_t *instrument, const sample_t *samples, effect_t *decoded_effects)
{
    if (context.pattern_data > context.file_end - 8)
    {
        error(MODFILE_INVALID_MISSING_PATTERN_DATA);
        return false;
    }
    const uint32_t word_1 = read_u32_le(context.pattern_data);
    const uint32_t word_2 = read_u32_le(context.pattern_data + 4);
    for (int slot = 0; slot <= 3; slot++)
    {
        const int code_offset = 12 + slot * 5;
        const int data_offset = slot * 8;
        const uint8_t code = mask_5_shift_right(word_1, code_offset);
        const uint8_t data = mask_8_shift_right(word_2, data_offset);
        effect_t decoded_effect = effect(code, data);
        if (decoded_effect.command != USE_SAMPLE_SLICE)
        {
            decoded_effects[slot] = decoded_effect;
            continue;
        }
        //
        // The bullshit below is only for the 06 (play end part of sample) command.
        //
        uint8_t most_significant_bits = 0;
        uint8_t least_significant_bits = 0;
        if (slot < 3)
        {
            // Desktop Tracker 0x6 (play end part of sample) command consumes the following effect slot as extra data.
            // The data is packed in this strange way so that it behaves the same way the ProTracker 0x9 command does
            // if the 0x6 command is in the last slot, or if the event is single effect.
            const int next_slot = slot + 1;
            most_significant_bits = mask_5_shift_right(word_1, 12 + next_slot * 5);
            least_significant_bits = mask_8_shift_right(word_2, next_slot * 8);
        }
        const uint32_t required_offset = least_significant_bits | (uint32_t) data << 8 | (uint32_t) most_significant_bits << 16;
        if (!find_or_create_sample_slice(required_offset, instrument, samples, &decoded_effect.data))
        {
            error(TOO_MANY_SAMPLE_SLICES);
            return false;
        }
        decoded_effects[slot] = decoded_effect;
        if (slot < 3)
        {
            // Clear the next effect lane and increment the slot number to ensure it is skipped over.
            slot += 1;
            decoded_effects[slot] = effect(0, 0);
        }
    }
    return true;
}

static bool find_or_create_sample_slice(const uint32_t required_offset, instrument_t *instrument, const sample_t *samples, uint8_t *slice_index)
{
    if (instrument == NULL)
    {
        // TODO:
        // Find out if Desktop Tracker honours 06 commands on events without an instrument number. (If it does, I have
        // got a problem).
        *slice_index = 0;
        return true;
    }
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
    const uint32_t sample_length = samples[instrument->sample_index].sample_length;
    if (required_offset >= sample_length)
    {
        // TODO:
        // Figure out what Desktop Tracker does in this case. Use that to inform the correct behaviour here.
        *slice_index = 0;
        return true;
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
