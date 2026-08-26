#include "format_soundtracker.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "memory/heap.h"
#include "pcm/mu_law.h"

/*
 * Standard 31-instrument MOD layout.
 */
enum {
    MOD_NAME_LENGTH = 20,
    MOD_SAMPLE_COUNT = 31,
    MOD_SAMPLE_HEADER_SIZE = 30,
    MOD_SEQUENCE_SIZE = 128,
    MOD_PATTERN_LINES = 64,

    MOD_SEQUENCE_LENGTH_OFFSET = 950,
    MOD_SEQUENCE_OFFSET = 952,
    MOD_SIGNATURE_OFFSET = 1080,
    MOD_HEADER_SIZE = 1084,
    MOD_PATTERN_CELL_SIZE = 4
};

typedef struct {
    int num_tracks;
    int sequence_length;
    int num_patterns;
    size_t pattern_data_size;
    size_t sample_data_offset;
    size_t total_sample_data_size;
} mod_info_t;

static uint8_t volume_mapping[65] = {
    0,
    99,  115, 129, 137, 145, 152, 160, 164, 168, 172, 176, 180, 183, 187, 191, 193,
    195, 197, 199, 201, 203, 205, 207, 209, 211, 213, 215, 217, 219, 221, 223, 224,
    225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239, 240,
    241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254, 255, 255,
};

static void calculate_volume_map(void)
{
    float gain_curve[256];
    for (int i = 0; i <= 127; i++)
    {
        gain_curve[(i * 2) + 1] = mu_law_to_linear(255 - i);
        if (i >= 1)
            gain_curve[i * 2] = (gain_curve[(i * 2) - 1] + gain_curve[(i * 2) + 1]) / 2;
    }
    // gain_curve now maps Arctracker volume values to linear gain values.
    uint8_t internal_volume[65];
    for (int ivol = 0; ivol <= 255; ivol++)
    {
        const uint8_t linear = (uint8_t) 64.0f * gain_curve[ivol];
        if (ivol > internal_volume[linear]) internal_volume[linear] = ivol;
    }
    printf("Mod to internal volume mapping:\n");
    for (int mod_volume = 0; mod_volume <= 64; mod_volume++)
    {
        printf("[%d] = %d;\n", mod_volume, internal_volume[mod_volume]);
    }
}

/*
 * --------------------------------------------------------------------------
 * Basic binary helpers
 * --------------------------------------------------------------------------
 */

static uint16_t read_be_u16(const uint8_t *data)
{
    return ((uint16_t)data[0] << 8) | data[1];
}


static bool size_multiply(size_t a, size_t b, size_t *result)
{
    if (a != 0 && b > SIZE_MAX / a)
        return false;

    *result = a * b;
    return true;
}


static bool size_add(size_t a, size_t b, size_t *result)
{
    if (b > SIZE_MAX - a)
        return false;

    *result = a + b;
    return true;
}


static bool range_available(mapped_file_t file, size_t offset, size_t length)
{
    return offset <= file.size && length <= file.size - offset;
}


/*
 * MOD strings are fixed-width and generally NUL-padded. Some files instead
 * pad them with spaces, so trim both.
 */
static void copy_mod_string(
    char *destination,
    size_t destination_size,
    const uint8_t *source,
    size_t source_size)
{
    size_t length = source_size;

    while (length > 0 &&
           (source[length - 1] == '\0' || source[length - 1] == ' '))
        --length;

    if (length >= destination_size)
        length = destination_size - 1;

    memcpy(destination, source, length);
    destination[length] = '\0';
}


/*
 * --------------------------------------------------------------------------
 * Signature recognition
 * --------------------------------------------------------------------------
 */

static int decimal_digit(uint8_t c)
{
    if (c < '0' || c > '9')
        return -1;

    return c - '0';
}


static int tracks_from_signature(const uint8_t *signature)
{
    if (memcmp(signature, "M.K.", 4) == 0 ||
        memcmp(signature, "M!K!", 4) == 0)
        return 4;

    /*
     * Common single-digit forms:
     *
     *     4CHN
     *     6CHN
     *     8CHN
     *
     * and similar TakeTracker/FastTracker variants.
     */
    if (signature[1] == 'C' &&
        signature[2] == 'H' &&
        signature[3] == 'N') {
        const int tracks = decimal_digit(signature[0]);

        if (tracks >= 1)
            return tracks;
    }

    /*
     * Common two-digit forms:
     *
     *     10CH
     *     12CH
     *     ...
     *
     * and TakeTracker's xxCN form.
     */
    if ((signature[2] == 'C' && signature[3] == 'H') ||
        (signature[2] == 'C' && signature[3] == 'N')) {
        const int tens = decimal_digit(signature[0]);
        const int units = decimal_digit(signature[1]);

        if (tens >= 0 && units >= 0) {
            const int tracks = (tens * 10) + units;

            if (tracks >= 1)
                return tracks;
        }
    }

    return 0;
}


/*
 * --------------------------------------------------------------------------
 * Structural inspection
 * --------------------------------------------------------------------------
 *
 * This is deliberately used by is_this_format(), not just read_module().
 *
 * Therefore, returning true means rather more than "there's a plausible
 * magic number at offset 1080": the file is large enough to contain all
 * pattern and sample data claimed by its headers.
 */

static bool inspect_mod(mapped_file_t file, mod_info_t *info)
{
    if (!range_available(file, 0, MOD_HEADER_SIZE))
        return false;

    const int num_tracks =
        tracks_from_signature(file.addr + MOD_SIGNATURE_OFFSET);

    if (num_tracks <= 0 || num_tracks > MAX_TRACKS)
        return false;

    const int sequence_length =
        file.addr[MOD_SEQUENCE_LENGTH_OFFSET];

    if (sequence_length < 1 ||
        sequence_length > MOD_SEQUENCE_SIZE)
        return false;

    /*
     * The entire 128-entry order table is conventionally inspected when
     * determining how much pattern data is stored, even though only the
     * first sequence_length entries are actually played.
     */
    int highest_pattern = 0;

    for (int i = 0; i < MOD_SEQUENCE_SIZE; ++i) {
        const int pattern = file.addr[MOD_SEQUENCE_OFFSET + i];

        if (pattern >= NUM_PATTERNS)
            return false;

        if (pattern > highest_pattern)
            highest_pattern = pattern;
    }

    const int num_patterns = highest_pattern + 1;

    size_t cells_per_pattern;
    size_t bytes_per_pattern;
    size_t pattern_data_size;

    if (!size_multiply(
            MOD_PATTERN_LINES,
            (size_t)num_tracks,
            &cells_per_pattern))
        return false;

    if (!size_multiply(
            cells_per_pattern,
            MOD_PATTERN_CELL_SIZE,
            &bytes_per_pattern))
        return false;

    if (!size_multiply(
            (size_t)num_patterns,
            bytes_per_pattern,
            &pattern_data_size))
        return false;

    size_t sample_data_offset;

    if (!size_add(
            MOD_HEADER_SIZE,
            pattern_data_size,
            &sample_data_offset))
        return false;

    if (sample_data_offset > file.size)
        return false;

    size_t total_sample_data_size = 0;

    for (int i = 0; i < MOD_SAMPLE_COUNT; ++i) {
        const size_t header_offset =
            MOD_NAME_LENGTH + (i * MOD_SAMPLE_HEADER_SIZE);

        const uint16_t length_words =
            read_be_u16(file.addr + header_offset + 22);

        const size_t sample_length =
            (size_t)length_words * 2;

        if (!size_add(
                total_sample_data_size,
                sample_length,
                &total_sample_data_size))
            return false;
    }

    if (!range_available(
            file,
            sample_data_offset,
            total_sample_data_size))
        return false;

    if (info != NULL) {
        info->num_tracks = num_tracks;
        info->sequence_length = sequence_length;
        info->num_patterns = num_patterns;
        info->pattern_data_size = pattern_data_size;
        info->sample_data_offset = sample_data_offset;
        info->total_sample_data_size = total_sample_data_size;
    }

    return true;
}


static bool is_this_format(mapped_file_t file)
{
    return inspect_mod(file, NULL);
}


/*
 * --------------------------------------------------------------------------
 * Note conversion
 * --------------------------------------------------------------------------
 *
 * The first entry (period 856) is C-1 in a normal ProTracker period table.
 * Arctracker note 1 is C0, so C1 is note 13.
 */

static int mod_period_to_arctracker_note(uint16_t period)
{
    static const uint16_t periods[] = {
        /* C-1 .. B-1 */
        856, 808, 762, 720, 678, 640,
        604, 570, 538, 508, 480, 453,

        /* C-2 .. B-2 */
        428, 404, 381, 360, 339, 320,
        302, 285, 269, 254, 240, 226,

        /* C-3 .. B-3 */
        214, 202, 190, 180, 170, 160,
        151, 143, 135, 127, 120, 113
    };

    if (period == 0)
        return 0;

    /*
     * Don't turn obviously out-of-range periods into arbitrary notes.
     */
    if (period > periods[0] ||
        period < periods[(sizeof periods / sizeof periods[0]) - 1])
        return 0;

    size_t closest = 0;
    uint16_t closest_difference =
        period > periods[0]
            ? period - periods[0]
            : periods[0] - period;

    for (size_t i = 1;
         i < sizeof periods / sizeof periods[0];
         ++i) {
        const uint16_t difference =
            period > periods[i]
                ? period - periods[i]
                : periods[i] - period;

        if (difference < closest_difference) {
            closest = i;
            closest_difference = difference;
        }
    }

    return 1 + (int)closest;
}


/*
 * --------------------------------------------------------------------------
 * Effect conversion
 * --------------------------------------------------------------------------
 */

static uint8_t scale_mod_volume(uint8_t volume)
{
    if (volume > 0x40)
        volume = 0x40;

    return volume_mapping[volume];
}


static void decode_effect(
    uint8_t mod_effect,
    uint8_t mod_data,
    effect_t *effect)
{
    effect->command = NO_EFFECT;
    effect->data = 0;
    switch (mod_effect) {
        case 0x0:
            /*
             * 000 is conventionally "no effect", despite effect 0 being
             * arpeggio.
             */
            if (mod_data != 0) {
                effect->command = ARPEGGIO;
                effect->data = mod_data;
            }
            break;

        case 0x1:
            effect->command = PITCH_SLIDE_UP;
            effect->data = mod_data;
            break;

        case 0x2:
            effect->command = PITCH_SLIDE_DOWN;
            effect->data = mod_data;
            break;

        case 0x3:
            effect->command = PORTAMENTO;
            effect->data = mod_data;
            break;

        case 0x9:
            effect->command = USE_SAMPLE_SLICE;
            effect->data = mod_data;
            break;

        case 0xA:
            if ((mod_data & 0xf0) != 0) {
                effect->command = CRESCENDO;
                effect->data = 2 * (mod_data >> 4);
            }
            else if ((mod_data & 0x0f) != 0) {
                effect->command = DECRESCENDO;
                effect->data = 2 * (mod_data & 0x0f);
            }
            break;

        case 0xB:
            /*
             * MOD Bxx and Arctracker SEQUENCE_JUMP both use the parameter
             * as the destination sequence position.
             */
            effect->command = SEQUENCE_JUMP;
            effect->data = mod_data;
            break;

        case 0xC:
            effect->command = SET_VOLUME;
            effect->data = scale_mod_volume(mod_data);
            break;

        case 0xD:
            /*
             * ProTracker Dxx represents the target pattern line as decimal, not hex.
             */
            effect->command = PATTERN_BREAK;
            effect->data = 10 * (mod_data >> 4) + (mod_data & 0xf);
            break;

        case 0xE:
            const uint8_t e_cmd = mod_data >> 4;
            const uint8_t e_cmd_data = mod_data & 0xf;
            if (e_cmd == 10)
            {
                effect->command = FINE_CRESCENDO;
                effect->data = e_cmd_data;
            }
            if (e_cmd == 11)
            {
                effect->command = FINE_DECRESCENDO;
                effect->data = e_cmd_data;
            }
            break;

        case 0xF:
            if (mod_data >= 1 && mod_data <= 0x20) {
                effect->command = SET_TEMPO;
                effect->data = mod_data;
            } else
            {
                const float ticks_per_second = (float) mod_data * 2.0f / 5.0f;
                effect->command = SET_TICKS_PER_SECOND;
                effect->data = (uint8_t) ticks_per_second + 0.5f;
            }
            break;

        default:
            /*
             * Unsupported effects are intentionally discarded rather than
             * making the whole module unloadable.
             */
            break;
    }

}


/*
 * --------------------------------------------------------------------------
 * Pattern loading
 * --------------------------------------------------------------------------
 */

static bool ensure_pattern_storage(
    pattern_t *pattern,
    int num_tracks)
{
    const int required_events = MOD_PATTERN_LINES * num_tracks;

    /*
     * This accommodates either possible module_create() policy:
     *
     *  - it creates the pattern structures but not their event arrays; or
     *  - it has already created sufficiently large arrays.
     *
     * If module_create() has a different invariant, this is the one small
     * helper that should need adapting.
     */
    if (pattern->events == NULL) {
        pattern->events = allocate_array(
            MODULE,
            required_events,
            sizeof(event_t)
        );

        if (pattern->events == NULL)
            return false;

        pattern->line_capacity = MOD_PATTERN_LINES;
    }
    else if (pattern->line_capacity < MOD_PATTERN_LINES) {
        return false;
    }

    pattern->num_lines = MOD_PATTERN_LINES;

    return true;
}


static event_t *event_at(
    pattern_t *pattern,
    int num_tracks,
    int line,
    int track)
{
    /*
     * Assumption: pattern event storage is row-major:
     *
     *     row 0 track 0
     *     row 0 track 1
     *     ...
     *     row 1 track 0
     *
     * This is deliberately isolated here in case Arctracker indexes pattern
     * storage differently.
     */
    return &pattern->events[(line * num_tracks) + track];
}


static bool load_patterns(
    module_t *module,
    mapped_file_t file,
    const mod_info_t *info)
{
    size_t offset = MOD_HEADER_SIZE;

    for (int pattern_no = 0;
         pattern_no < info->num_patterns;
         ++pattern_no) {
        pattern_t *pattern =
            &module->patterns[pattern_no];

        if (!ensure_pattern_storage(
                pattern,
                info->num_tracks))
            return false;

        for (int line = 0;
             line < MOD_PATTERN_LINES;
             ++line) {
            for (int track = 0;
                 track < info->num_tracks;
                 ++track) {
                if (!range_available(
                        file,
                        offset,
                        MOD_PATTERN_CELL_SIZE))
                    return false;

                const uint8_t byte0 = file.addr[offset + 0];
                const uint8_t byte1 = file.addr[offset + 1];
                const uint8_t byte2 = file.addr[offset + 2];
                const uint8_t byte3 = file.addr[offset + 3];

                offset += MOD_PATTERN_CELL_SIZE;

                const int instrument_no =
                    (byte0 & 0xf0) |
                    ((byte2 & 0xf0) >> 4);

                const uint16_t period =
                    ((uint16_t)(byte0 & 0x0f) << 8) |
                    byte1;

                const uint8_t mod_effect =
                    byte2 & 0x0f;

                event_t *event =
                    event_at(
                        pattern,
                        info->num_tracks,
                        line,
                        track
                    );

                event->note =
                    mod_period_to_arctracker_note(period);

                event->instrument_no =
                    instrument_no;

                decode_effect(
                    mod_effect,
                    byte3,
                    &event->effects[0]
                );
            }
        }
    }

    return true;
}


/*
 * --------------------------------------------------------------------------
 * Instrument/sample loading
 * --------------------------------------------------------------------------
 */

static bool load_samples(
    module_t *module,
    mapped_file_t file,
    const mod_info_t *info)
{
    size_t sample_offset = info->sample_data_offset;

    for (int sample_no = 0;
         sample_no < MOD_SAMPLE_COUNT;
         ++sample_no) {
        const size_t header_offset =
            MOD_NAME_LENGTH +
            (sample_no * MOD_SAMPLE_HEADER_SIZE);

        const uint8_t *header =
            file.addr + header_offset;

        const uint16_t sample_length_words =
            read_be_u16(header + 22);

        const uint8_t finetune =
            header[24] & 0xF;

        const uint8_t mod_volume =
            header[25];

        const uint16_t loop_start_words =
            read_be_u16(header + 26);

        const uint16_t loop_length_words =
            read_be_u16(header + 28);

        const int sample_length =
            (int)sample_length_words * 2;

        const int loop_start =
            (int)loop_start_words * 2;

        const int loop_length =
            (int)loop_length_words * 2;

        /*
         * MOD instrument numbers are 1..31. Instruments array is zero-indexed.
         */

        instrument_t *instrument =
            &module->instruments[sample_no];

        sample_t *sample =
            &module->samples[sample_no];

        copy_mod_string(
            instrument->name,
            sizeof instrument->name,
            header,
            22
        );

        instrument->assigned =
            sample_length > 0 ||
            instrument->name[0] != '\0';

        instrument->default_volume =
            scale_mod_volume(mod_volume);

        instrument->transpose = 12;
        instrument->finetune = finetune < 8 ? finetune : finetune - 16;
        instrument->sample_index = sample_no;

        /*
         * Set up the slice offsets to match what the 0x9 command requires.
         */
        for (int offset = 0; offset < 256; offset++)
            instrument->slice_offsets[offset] = (4096 * offset >> 4) + (256 * offset & 0xf);

        /*
         * A loop length of one word (two bytes) conventionally means
         * "no loop".
         *
         * Invalid loops are disabled rather than causing the entire module
         * load to fail.
         */
        if (loop_length_words > 1 &&
            loop_start >= 0 &&
            loop_start < sample_length &&
            loop_length > 0 &&
            loop_length <= sample_length - loop_start) {
            instrument->repeats = true;
            instrument->repeat_offset = loop_start;
            instrument->repeat_length = loop_length;
        }
        else {
            instrument->repeats = false;
            instrument->repeat_offset = 0;
            instrument->repeat_length = 0;
        }

        sample->sample_length = sample_length;

        if (sample_length == 0) {
            sample->sample_data = NULL;
            continue;
        }

        if (!range_available(
                file,
                sample_offset,
                (size_t)sample_length))
            return false;

        float *sample_data =
            allocate_array(
                MODULE,
                sample_length + 2,
                sizeof(float)
            );

        if (sample_data == NULL)
            return false;

        for (int i = 0; i < sample_length; ++i) {
            const uint8_t raw =
                file.addr[sample_offset + i];

            /*
             * Convert the raw byte explicitly rather than relying on
             * uint8_t -> int8_t conversion for values > 127.
             */
            const int signed_sample =
                raw < 128
                    ? raw
                    : (int)raw - 256;

            /*
             * Signed 8-bit PCM has the range -128..127, so this produces
             * -1.0 .. 0.9921875. This is the usual lossless normalisation
             * of signed 8-bit PCM into floating point.
             */
            sample_data[i] =
                (float)signed_sample / 128.0f;
        }

        sample->sample_data = sample_data;

        sample_offset += (size_t)sample_length;
    }

    return true;
}


/*
 * --------------------------------------------------------------------------
 * Main loader
 * --------------------------------------------------------------------------
 */

static module_t *read_module(mapped_file_t file)
{
    mod_info_t info;
    // calculate_volume_map();
    if (!inspect_mod(file, &info))
        return NULL;

    module_t *module =
        module_create(
            info.num_tracks,
            info.sequence_length,
            info.num_patterns,
            MOD_SAMPLE_COUNT
        );

    if (module == NULL)
        return NULL;

    module->format = "MOD";

    copy_mod_string(
        module->name,
        sizeof module->name,
        file.addr,
        MOD_NAME_LENGTH
    );

    module->author[0] = '\0';

    module->initial_ticks_per_event = 6;
    module->default_pattern_length = MOD_PATTERN_LINES;
    module->master_gain = 0.25f;
    module->interpolation_type = NONE;

    for (int track = 0; track < module->num_tracks; track++)
    {
        module->tracks[track].effects_displayed = 1;
        module->tracks[track].muted = false;
        module->tracks[track].panning = 0x80;
    }

    for (int i = 0;
         i < info.sequence_length;
         ++i) {
        module->sequence[i] =
            file.addr[MOD_SEQUENCE_OFFSET + i];
    }

    if (!load_patterns(module, file, &info) ||
        !load_samples(module, file, &info)) {
        module_destroy(module);
        return NULL;
    }

    return module;
}


/*
 * --------------------------------------------------------------------------
 * Public format interface
 * --------------------------------------------------------------------------
 */

format_t soundtracker_format(void)
{
    return (format_t) {
        .is_this_format = is_this_format,
        .read_module = read_module,
        .write_module = NULL
    };
}