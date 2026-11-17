#include "format_musx.h"
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include "messages.h"
#include "io/error.h"
#include "memory/bits.h"
#include "memory/heap.h"
#include "player/period.h"
#include "vidc/vidc.h"

/*********************************************************************************************************************
 * The format of a Tracker file (also referred to as an STModule file) is similar to the RIFF file format. It is     *
 * organised into chunks which are all structured like this:                                                         *
 *                                                                                                                   *
 *   Offset  Size  Description                                                                                       *
 *   0       4     Chunk id, ASCII FourCC                                                                            *
 *   4       4     Chunk data length in bytes, excluding this 8-byte header                                          *
 *   8       N     Chunk data                                                                                        *
 *                                                                                                                   *
 * The chunks can be and are packed recursively, so the contents of the MUSX chunk is the entirety of the file,      *
 * which itself is split into chunks. And again, the SAMP chunk contents are further split into sub chunks.          *
 *                                                                                                                   *
 * All multi-byte number values are stored in little-endian byte order.                                              *
 *                                                                                                                   *
 * As already mentioned, a Tracker module file begins with an MUSX chunk, the presence of which is used to identify  *
 * it as a Tracker module. Its length should be equal to the length of the file less 8. Its sub chunks are:          *
 *                                                                                                                   *
 * TINF                                                                                                              *
 *   - Tracker version identifier                                                                                    *
 *   - 4 bytes                                                                                                       *
 *   - This is a date stored as packed BCD. Offset 0 is the year, 1 is the century, 2 is the month, 3 is the day.    *
 *                                                                                                                   *
 * MVOX                                                                                                              *
 *   - Number of tracks in the module                                                                                *
 *   - 4 bytes                                                                                                       *
 *                                                                                                                   *
 * STER                                                                                                              *
 *   - Initial stereo positions                                                                                      *
 *   - 8 bytes                                                                                                       *
 *   - One byte per track. There are always eight irrespective of how many tracks the module has. Values are:        *
 *       1: pan full left                                                                                            *
 *       2: pan two thirds left                                                                                      *
 *       3: pan one third left                                                                                       *
 *       4: centre                                                                                                   *
 *       5: pan one third right                                                                                      *
 *       6: pan two thirds right                                                                                     *
 *       7: pan full right                                                                                           *
 *                                                                                                                   *
 * MNAM                                                                                                              *
 *   - Module name                                                                                                   *
 *   - 32 bytes ASCII                                                                                                *
 *                                                                                                                   *
 * ANAM                                                                                                              *
 *   - Author name                                                                                                   *
 *   - 32 bytes ASCII                                                                                                *
 *                                                                                                                   *
 * MLEN                                                                                                              *
 *   - Sequence length                                                                                               *
 *   - 4 bytes                                                                                                       *
 *                                                                                                                   *
 * PNUM                                                                                                              *
 *   - Number of patterns in the module                                                                              *
 *   - 4 bytes                                                                                                       *
 *   - Minimum: 1, maximum: 64.                                                                                      *
 *                                                                                                                   *
 * PLEN                                                                                                              *
 *   - Pattern lengths                                                                                               *
 *   - 64 bytes                                                                                                      *
 *   - One byte per pattern, the byte at offset 0 is the length of pattern 0, etc. The maximum length of any pattern *
 *     in a Tracker module is 64. This array is always 64 bytes long irrespective of the number specified in PNUM.   *
 *                                                                                                                   *
 * PATT                                                                                                              *
 *   - Pattern data                                                                                                  *
 *   - (4 * num tracks * pattern length) bytes                                                                       *
 *   - The manual states that PATT chunks are always 2048 bytes in length but this is not the case. The manual also  *
 *     states that events are stored for all eight tracks and 64 lines irrespective of the number of tracks in the   *
 *     module and the number of lines in the pattern. This is also not the case: the PATT chunk data contains the    *
 *     number of events that are visible in the pattern, i.e. (num tracks * pattern length). The pattern events are  *
 *     stored in row-major order, i.e. (for a 4-track module) row 0 track 0, row 0 track 1, row 0 track 2, row 0     *
 *     track 3, then row 1 track 0, row 1 track 1, etc. The events are packed into a 4-byte word like this:          *
 *                                                                                                                   *
 *       Offset Description                                                                                          *
 *       ------ -------------------                                                                                  *
 *            0 Effect data                                                                                          *
 *            1 Effect command code                                                                                  *
 *            2 Sample number                                                                                        *
 *            3 Note                                                                                                 *
 *                                                                                                                   *
 *     There is one PATT chunk per pattern used in the module. Unused patterns are not stored. One would hope that   *
 *     the number of PATT chunks in the file is equal to that specified in the PNUM chunk, although Arctracker only  *
 *     requires that there be at least as many PATT chunks as specified.                                             *
 *                                                                                                                   *
 * SAMP                                                                                                              *
 *   - Sample data                                                                                                   *
 *   - (84 + sample length) bytes                                                                                    *
 *   - The manual states there are always 36 SAMP chunks irrespective of how many sample slots are used in the       *
 *     module, although Arctracker will load the module anyway if there are fewer, provided all the chunk lengths    *
 *     are correct and all the data is well formed. The SAMP chunk data is comprised of sub chunks:                  *
 *                                                                                                                   *
 *     SNAM                                                                                                          *
 *       - Sample name                                                                                               *
 *       - 20 bytes ASCII                                                                                            *
 *                                                                                                                   *
 *     SVOL                                                                                                          *
 *       - Sample volume                                                                                             *
 *       - 4 bytes                                                                                                   *
 *       - Minimum: 0, maximum: 255                                                                                  *
 *                                                                                                                   *
 *     SLEN                                                                                                          *
 *       - Sample length                                                                                             *
 *       - 4 bytes                                                                                                   *
 *                                                                                                                   *
 *     ROFS                                                                                                          *
 *       - Repeat offset                                                                                             *
 *       - 4 bytes                                                                                                   *
 *                                                                                                                   *
 *     RLEN                                                                                                          *
 *       - Repeat length                                                                                             *
 *       - 4 bytes                                                                                                   *
 *       - A repeat offset of 0 plus a repeat length of 2 is interpreted by Tracker as meaning no loop, but          *
 *         Arctracker uses the Amiga MOD convention of repeat length <= 2 meaning no loop irrespective of offset.    *
 *                                                                                                                   *
 *     SDAT                                                                                                          *
 *       - Sample data                                                                                               *
 *       - (sample length as given by SLEN) bytes                                                                    *
 *       - The sample data is stored as 8-bit VIDC encoded. See vidc_to_linear() in vidc.c for more information.     *
 *********************************************************************************************************************/

#define MUSX_CHUNK_ID_LENGTH 4
#define MUSX_CHUNK_HEADER_LENGTH 8
#define MUSX_TUNE_NAME_LEN 32
#define MUSX_AUTHOR_NAME_LEN 32
#define MUSX_NUM_TRACKS 8
#define MUSX_NUM_PATTERNS 64
#define MUSX_MAX_PATTERN_LEN 64
#define MUSX_SEQUENCE_LEN 128
#define MUSX_PATT_CHUNK_LEN 2048
#define MUSX_MIN_SAMP_CHUNK_LEN 84
#define MUSX_NUM_SAMPLES 36
#define MUSX_SAMPLE_NAME_LEN 20
#define MUSX_EVENT_SIZE_BYTES 4
#define MUSX_VOLUME_MAX 255
#define MUSX_BASE_NOTE 24

typedef struct {
    uint32_t tracker_version_id[3];
    uint32_t num_tracks;
    uint8_t stereo_positions[8];
    char tune_name[MUSX_TUNE_NAME_LEN + 1];
    char author_name[MUSX_AUTHOR_NAME_LEN + 1];
    uint32_t sequence_length;
    uint32_t num_patterns;
    uint8_t pattern_lengths[MUSX_NUM_PATTERNS];
    uint8_t sequence[MUSX_SEQUENCE_LEN];
    uint8_t patterns_discovered;
    const uint8_t *pattern_data[MUSX_NUM_PATTERNS];
    size_t pattern_data_sizes[MUSX_NUM_PATTERNS];
    uint8_t samples_discovered;
    const uint8_t *sample_chunks[MUSX_NUM_SAMPLES];
    size_t sample_chunk_sizes[MUSX_NUM_SAMPLES];
    const uint8_t *file_end;
} musx_file_t;

typedef struct {
    char name[MUSX_SAMPLE_NAME_LEN];
    uint32_t volume;
    uint32_t length;
    uint32_t repeat_offset;
    uint32_t repeat_length;
    const uint8_t *data;
    size_t data_size;
} sample_definition_t;

static const char* MUSX_CHUNK_ID = "MUSX";
static const char* TINF_CHUNK_ID = "TINF";
static const char* MVOX_CHUNK_ID = "MVOX";
static const char* STER_CHUNK_ID = "STER";
static const char* MNAM_CHUNK_ID = "MNAM";
static const char* ANAM_CHUNK_ID = "ANAM";
static const char* MLEN_CHUNK_ID = "MLEN";
static const char* PNUM_CHUNK_ID = "PNUM";
static const char* PLEN_CHUNK_ID = "PLEN";
static const char* SEQU_CHUNK_ID = "SEQU";
static const char* PATT_CHUNK_ID = "PATT";
static const char* SAMP_CHUNK_ID = "SAMP";
static const char* SNAM_CHUNK_ID = "SNAM";
static const char* SVOL_CHUNK_ID = "SVOL";
static const char* SLEN_CHUNK_ID = "SLEN";
static const char* ROFS_CHUNK_ID = "ROFS";
static const char* RLEN_CHUNK_ID = "RLEN";
static const char* SDAT_CHUNK_ID = "SDAT";

static const uint8_t MUSX_ARPEGGIO_CMD = 0;           // 0
static const uint8_t MUSX_PORTAMENTO_UP_CMD = 1;      // 1
static const uint8_t MUSX_PORTAMENTO_DOWN_CMD = 2;    // 2
static const uint8_t MUSX_BREAK_CMD = 11;             // B
static const uint8_t MUSX_SET_STEREO_CMD = 14;        // E
static const uint8_t MUSX_VOLUME_SLIDE_UP_CMD = 16;   // G
static const uint8_t MUSX_VOLUME_SLIDE_DOWN_CMD = 17; // H
static const uint8_t MUSX_JUMP_CMD = 19;              // J
static const uint8_t MUSX_SET_SPEED_CMD = 28;         // S
static const uint8_t MUSX_SET_VOLUME_CMD = 31;        // V

static const uint32_t CHUNK_HEADER_SIZE = 8;
static const int PANNING[] = {1, 43, 86, 128, 170, 213, 255};
static const instrument_t UNASSIGNED_INSTRUMENT = {
    .assigned = false,
};

static bool is_tracker_format(mapped_file_t);
static bool chunk_is(const char *, const uint8_t *);
static module_t *read_tracker_module(mapped_file_t);
static bool walk_chunks(const uint8_t *, musx_file_t *);
static bool musx_sub_chunk_recognised(const uint8_t *);
static bool read_tinf_chunk(size_t, const uint8_t *, musx_file_t *);
static bool read_mvox_chunk(size_t, const uint8_t *, musx_file_t *);
static bool read_ster_chunk(size_t, const uint8_t *, musx_file_t *);
static bool read_mnam_chunk(size_t, const uint8_t *, musx_file_t *);
static bool read_anam_chunk(size_t, const uint8_t *, musx_file_t *);
static bool read_mlen_chunk(size_t, const uint8_t *, musx_file_t *);
static bool read_pnum_chunk(size_t, const uint8_t *, musx_file_t *);
static bool read_plen_chunk(size_t, const uint8_t *, musx_file_t *);
static bool read_sequ_chunk(size_t, const uint8_t *, musx_file_t *);
static bool read_patt_chunk(size_t, const uint8_t *, musx_file_t *);
static bool read_samp_chunk(size_t, const uint8_t *, musx_file_t *);
static uint8_t decode_bcd(uint8_t value);
static const uint8_t *next_chunk_address(const uint8_t *);
static module_t *create_module(musx_file_t);
static void init_tracks(module_t *module, musx_file_t);
static bool set_sequence(module_t *, musx_file_t);
static bool read_patterns(module_t *, musx_file_t);
static void read_pattern_data(const uint8_t *, module_t *, int, uint8_t);
static void decode_tracker_event(const uint8_t *, event_t *);
static effect_t effect(uint8_t, uint8_t);
static command_t tracker_command(int, uint8_t);
static bool read_samples(module_t *, musx_file_t);
static float calculate_sample_rate(int, int);
static float assumed_sample_period(int);
static bool read_sample_definition(const uint8_t *chunk_addr, size_t end, sample_definition_t *);
static bool samp_sub_chunk_recognised(const uint8_t *);
static void copy_string(char *, const char *, int);

format_t format_musx(void)
{
    return (format_t) {
        .is_this_format = is_tracker_format,
        .read_module = read_tracker_module,
        .write_module = NULL, // Writing is not supported for this format.
    };
}

static bool is_tracker_format(const mapped_file_t file)
{
    if (file.size < 12) return false;
    return chunk_is(MUSX_CHUNK_ID, file.addr);
}

static bool chunk_is(const char *chunk_id, const uint8_t *addr)
{
    return memcmp(chunk_id, addr, 4) == 0;
}

static module_t *read_tracker_module(const mapped_file_t file)
{
    const size_t file_end = (size_t) file.addr + file.size;
    musx_file_t file_info = {
        .file_end = (uint8_t *) file_end,
    };
    if (!walk_chunks(file.addr + CHUNK_HEADER_SIZE, &file_info))
    {
        return NULL;
    }
    module_t *module = create_module(file_info);
    if (module == NULL)
    {
        return NULL;
    }
    init_tracks(module, file_info);
    if (!set_sequence(module, file_info)) goto failed;
    if (!read_patterns(module, file_info)) goto failed;
    if (!read_samples(module, file_info)) goto failed;
    return module;

failed:
    module_destroy(module);
    return NULL;
}

static bool walk_chunks(const uint8_t *chunk_addr, musx_file_t *out)
{
    bool ok = true;
    while (ok && chunk_addr < out->file_end)
    {
        const size_t remaining = (size_t)(out->file_end - chunk_addr);
        if (remaining < CHUNK_HEADER_SIZE)
        {
            error("Truncated chunk header");
            return false;
        }
        if (!musx_sub_chunk_recognised(chunk_addr))
        {
            // Unrecognised chunk id, which means either the file is corrupt, or there is extraneous garbage at the end
            // of an otherwise valid file (at least such example has been seen). We do not need to worry about forwards
            // compatibility with future versions of Tracker, so we will return successfully here and let the loader
            // decide whether or not the file is valid.
            return true;
        }
        const size_t data_size = read_u32_le(chunk_addr + 4);
        if (data_size > remaining - CHUNK_HEADER_SIZE)
        {
            error(CHUNK_EXTENDS_BEYOND_EOF);
            return false;
        }
        const uint8_t *data = chunk_addr + CHUNK_HEADER_SIZE;
        if (chunk_is(TINF_CHUNK_ID, chunk_addr))
            ok = read_tinf_chunk(data_size, data, out);
        if (chunk_is(MVOX_CHUNK_ID, chunk_addr))
            ok = read_mvox_chunk(data_size, data, out);
        if (chunk_is(STER_CHUNK_ID, chunk_addr))
            ok = read_ster_chunk(data_size, data, out);
        if (chunk_is(MNAM_CHUNK_ID, chunk_addr))
            ok = read_mnam_chunk(data_size, data, out);
        if (chunk_is(ANAM_CHUNK_ID, chunk_addr))
            ok = read_anam_chunk(data_size, data, out);
        if (chunk_is(MLEN_CHUNK_ID, chunk_addr))
            ok = read_mlen_chunk(data_size, data, out);
        if (chunk_is(PNUM_CHUNK_ID, chunk_addr))
            ok = read_pnum_chunk(data_size, data, out);
        if (chunk_is(PLEN_CHUNK_ID, chunk_addr))
            ok = read_plen_chunk(data_size, data, out);
        if (chunk_is(SEQU_CHUNK_ID, chunk_addr))
            ok = read_sequ_chunk(data_size, data, out);
        if (chunk_is(PATT_CHUNK_ID, chunk_addr))
            ok = read_patt_chunk(data_size, data, out);
        if (chunk_is(SAMP_CHUNK_ID, chunk_addr))
            ok = read_samp_chunk(data_size, data, out);
        chunk_addr = next_chunk_address(chunk_addr);
    }
    return ok;
}

static bool musx_sub_chunk_recognised(const uint8_t *chunk_addr)
{
    return chunk_is(TINF_CHUNK_ID, chunk_addr)
        || chunk_is(MVOX_CHUNK_ID, chunk_addr)
        || chunk_is(STER_CHUNK_ID, chunk_addr)
        || chunk_is(MNAM_CHUNK_ID, chunk_addr)
        || chunk_is(ANAM_CHUNK_ID, chunk_addr)
        || chunk_is(MLEN_CHUNK_ID, chunk_addr)
        || chunk_is(PNUM_CHUNK_ID, chunk_addr)
        || chunk_is(PLEN_CHUNK_ID, chunk_addr)
        || chunk_is(SEQU_CHUNK_ID, chunk_addr)
        || chunk_is(PATT_CHUNK_ID, chunk_addr)
        || chunk_is(SAMP_CHUNK_ID, chunk_addr);
}

static bool read_tinf_chunk(const size_t data_size, const uint8_t *data, musx_file_t *out)
{
    if (data_size < 4)
    {
        error("Invalid TINF chunk");
        return false;
    }
    // Tracker version id is a date stored as packed BCD.
    const uint8_t year = read_u8(data);
    const uint8_t century = read_u8(data + 1);
    const uint8_t month = read_u8(data + 2);
    const uint8_t day = read_u8(data + 3);
    out->tracker_version_id[0] = decode_bcd(century) * 100 + decode_bcd(year);
    out->tracker_version_id[1] = decode_bcd(month);
    out->tracker_version_id[2] = decode_bcd(day);
    return true;
}

static bool read_mvox_chunk(const size_t data_size, const uint8_t *data, musx_file_t *out)
{
    if (data_size < 4)
    {
        error("Invalid MVOX chunk");
        return false;
    }
    const uint32_t num_tracks = read_u32_le(data);
    if (num_tracks == 0 || num_tracks > MUSX_NUM_TRACKS)
    {
        error("Invalid track count");
        return false;
    }
    out->num_tracks = num_tracks;
    return true;
}

static bool read_ster_chunk(const size_t data_size, const uint8_t *data, musx_file_t *out)
{
    if (data_size < 8)
    {
        error("Invalid STER chunk");
        return false;
    }
    for (int track = 0; track < MUSX_NUM_TRACKS; track++)
    {
        out->stereo_positions[track] = read_u8(data + track);
    }
    return true;
}

static bool read_mnam_chunk(const size_t data_size, const uint8_t *data, musx_file_t *out)
{
    if (data_size < MUSX_TUNE_NAME_LEN)
    {
        error("Invalid MNAM chunk");
        return false;
    }
    copy_string(out->tune_name, (char *) data, MUSX_TUNE_NAME_LEN);
    return true;
}

static bool read_anam_chunk(const size_t data_size, const uint8_t *data, musx_file_t *out)
{
    if (data_size < MUSX_AUTHOR_NAME_LEN)
    {
        error("Invalid ANAM chunk");
        return false;
    }
    copy_string(out->author_name, (char *) data, MUSX_AUTHOR_NAME_LEN);
    return true;
}

static bool read_mlen_chunk(const size_t data_size, const uint8_t *data, musx_file_t *out)
{
    if (data_size < 4)
    {
        error("Invalid MLEN chunk");
        return false;
    }
    const uint32_t sequence_length = read_u32_le(data);
    if (sequence_length == 0 || sequence_length > MUSX_SEQUENCE_LEN)
    {
        error("Invalid sequence length");
        return false;
    }
    out->sequence_length = sequence_length;
    return true;
}

static bool read_pnum_chunk(const size_t data_size, const uint8_t *data, musx_file_t *out)
{
    if (data_size < 4)
    {
        error("Invalid PNUM chunk");
        return false;
    }
    const uint32_t num_patterns = read_u32_le(data);
    if (num_patterns == 0 || num_patterns > MUSX_NUM_PATTERNS)
    {
        error("Invalid number of patterns");
        return false;
    }
    out->num_patterns = num_patterns;
    return true;
}

static bool read_plen_chunk(const size_t data_size, const uint8_t *data, musx_file_t *out)
{
    if (data_size < MUSX_NUM_PATTERNS)
    {
        error("Invalid PLEN chunk");
        return false;
    }
    for (int pattern_no = 0; pattern_no < MUSX_NUM_PATTERNS; pattern_no++)
    {
        const uint8_t pattern_length = read_u8(data + pattern_no);
        if (pattern_length == 0 || pattern_length > MUSX_MAX_PATTERN_LEN)
        {
            error("Invalid pattern length");
            return false;
        }
        out->pattern_lengths[pattern_no] = pattern_length;
    }
    return true;
}

static bool read_sequ_chunk(const size_t data_size, const uint8_t *data, musx_file_t *out)
{
    if (data_size < MUSX_SEQUENCE_LEN)
    {
        error("Invalid SEQU chunk");
        return false;
    }
    for (int position = 0; position < MUSX_SEQUENCE_LEN; position++)
    {
        out->sequence[position] = read_u8(data + position);
    }
    return true;
}

static bool read_patt_chunk(const size_t data_size, const uint8_t *data, musx_file_t *out)
{
    if (data_size % 4 != 0)
    {
        error("Invalid PATT chunk");
        return false;
    }
    if (out->patterns_discovered == MUSX_NUM_PATTERNS)
    {
        error("Too many PATT chunks");
        return false;
    }
    out->pattern_data[out->patterns_discovered] = data;
    out->pattern_data_sizes[out->patterns_discovered] = data_size;
    out->patterns_discovered += 1;
    return true;
}

static bool read_samp_chunk(const size_t data_size, const uint8_t *data, musx_file_t *out)
{
    if (out->samples_discovered == MUSX_NUM_SAMPLES)
    {
        error("Too many SAMP chunks");
        return false;
    }
    out->sample_chunks[out->samples_discovered] = data;
    out->sample_chunk_sizes[out->samples_discovered] = data_size;
    out->samples_discovered += 1;
    return true;
}

static uint8_t decode_bcd(const uint8_t value)
{
    return (value & 0x0f) + (value >> 4) * 10;
}

static const uint8_t *next_chunk_address(const uint8_t *chunk_address)
{
    const uint32_t chunk_length = read_u32_le(chunk_address + 4);
    return chunk_address + chunk_length + CHUNK_HEADER_SIZE;
}

static module_t *create_module(const musx_file_t musx_file)
{
    if (musx_file.num_patterns > musx_file.patterns_discovered)
    {
        error(MODFILE_INVALID_MISSING_PATTERN_DATA);
        return NULL;
    }
    module_t *module = module_create(
        musx_file.num_tracks,
        musx_file.sequence_length,
        musx_file.num_patterns,
        musx_file.samples_discovered
    );
    if (module == NULL)
    {
        return NULL;
    }
    snprintf(module->name, sizeof(module->name), "%s", musx_file.tune_name);
    snprintf(module->author, sizeof(module->author), "%s", musx_file.author_name);
    module->initial_ticks_per_event = 6;
    module->master_gain = 0.25f;
    module->default_pattern_length = 64;
    module->interpolation_type = INTERPOLATE_NONE;
    module->volume_mapping_type = VOLUME_ARCHIMEDES;
    module->relative_pitch_bend = false;
    return module;
}

static void init_tracks(module_t *module, const musx_file_t musx_file)
{
    assert(module->tracks != NULL);
    for (int track = 0; track < module->num_tracks; track++)
    {
        module->tracks[track].muted = false;
        module->tracks[track].effects_displayed = 1;
        const uint8_t initial_track_stereo = musx_file.stereo_positions[track];
        if (initial_track_stereo == 0 || initial_track_stereo > 7)
            module->tracks[track].panning = 128; // Centre.
        else
            module->tracks[track].panning = PANNING[initial_track_stereo - 1];
    }
}

static bool set_sequence(module_t *module, const musx_file_t musx_file)
{
    assert(module->sequence != NULL);
    for (uint32_t position = 0; position < musx_file.sequence_length; position++)
    {
        const int pattern_no = musx_file.sequence[position];
        if (pattern_no >= module->num_patterns)
        {
            error(INVALID_PATTERN_NUMBER);
            return false;
        }
        module->sequence[position] = pattern_no;
    }
    return true;
}

static bool read_patterns(module_t *module, const musx_file_t musx_file)
{
    for (int pattern_no = 0; pattern_no < module->num_patterns; pattern_no++)
    {
        const uint8_t pattern_length = musx_file.pattern_lengths[pattern_no];
        const int num_events = pattern_length * module->num_tracks;
        const size_t expected_size = MUSX_EVENT_SIZE_BYTES * num_events;
        const size_t reported_size = musx_file.pattern_data_sizes[pattern_no];
        if (expected_size != reported_size)
        {
            error("Pattern data size inconsistent");
            return false;
        }
        const uint8_t *data_in = musx_file.pattern_data[pattern_no];
        if (data_in > musx_file.file_end - expected_size)
        {
            error(MODFILE_INVALID_MISSING_PATTERN_DATA);
            return false;
        }
        if (!module_create_pattern(module, pattern_no, pattern_length))
        {
            return false;
        }
        read_pattern_data(data_in, module, pattern_no, pattern_length);
    }
    return true;
}

static void read_pattern_data(const uint8_t *data_in, module_t *module, const int pattern_no, const uint8_t pattern_length)
{
    for (int line = 0; line < pattern_length; line++)
    {
        for (uint32_t track = 0; track < module->track_capacity; track++)
        {
            const uint32_t event_index = line * module->track_capacity + track;
            event_t *event = &module->patterns[pattern_no].events[event_index];
            if (track >= (uint32_t) module->num_tracks)
            {
                *event = (event_t) {0};
                continue;
            }
            decode_tracker_event(data_in, event);
            data_in += MUSX_EVENT_SIZE_BYTES;
        }
    }
}

static void decode_tracker_event(const uint8_t *data_in, event_t *decoded)
{
    const uint32_t raw = read_u32_le(data_in);
    decoded->instrument_no = (int) mask_8_shift_right(raw, 16);
    const int note = (int) mask_8_shift_right(raw, 24);
    decoded->note = note == 0 ? 0 : note + 12;
    if (note_out_of_range(decoded->note))
    {
        decoded->note = 0;
    }
    decoded->effects[0] = effect(mask_8_shift_right(raw, 8), mask_8_shift_right(raw, 0));
    for (int i = 1; i <= 3; i++)
    {
        decoded->effects[i] = effect(0, 0);
    }
}

static effect_t effect(const uint8_t code, const uint8_t data)
{
    const command_t command = tracker_command(code, data);
    uint8_t effect_data = data;
    if (command == SET_STEREO)
    {
        if (data == 0 || data > 7) effect_data = 128; // Pathological value, centre it.
        else effect_data = PANNING[data - 1];
    }
    if (command == BREAK_TO_NEXT_PATTERN)
    {
        // Tracker always enters the next pattern at line 0, but Arctracker interprets the data for a pattern break
        // command as the row to enter the next pattern at, as per the ProTracker Dxy command. Some STModules have been
        // seen to contain nonzero data for these commands, which is ignored by Tracker but would cause Arctracker to
        // play the module incorrectly. To prevent that, we zero the data here.
        effect_data = 0;
    }
    if (command == VOLUME_SLIDE)
    {
        if (code == MUSX_VOLUME_SLIDE_UP_CMD) effect_data = 0x80 | data;
        else effect_data = data;
    }
    return (effect_t) {
        .data = effect_data,
        .command = command,
    };
}

static command_t tracker_command(const int code, const uint8_t data)
{
    if (code == MUSX_ARPEGGIO_CMD && data > 0) return CHIPTUNE_ARPEGGIO;
    if (code == MUSX_PORTAMENTO_UP_CMD) return PORTAMENTO_UP;
    if (code == MUSX_PORTAMENTO_DOWN_CMD) return PORTAMENTO_DOWN;
    if (code == MUSX_BREAK_CMD) return BREAK_TO_NEXT_PATTERN;
    if (code == MUSX_SET_STEREO_CMD) return SET_STEREO;
    if (code == MUSX_VOLUME_SLIDE_UP_CMD) return VOLUME_SLIDE;
    if (code == MUSX_VOLUME_SLIDE_DOWN_CMD) return VOLUME_SLIDE;
    if (code == MUSX_JUMP_CMD) return SEQUENCE_JUMP;
    if (code == MUSX_SET_SPEED_CMD) return SET_TICKS_PER_EVENT;
    if (code == MUSX_SET_VOLUME_CMD) return SET_VOLUME;
    return NO_EFFECT;
}

static bool read_samples(module_t *module, const musx_file_t musx_file)
{
    const float sample_rate = calculate_sample_rate(module->num_tracks, MUSX_BASE_NOTE);
    for (int sample_index = 0; sample_index < musx_file.samples_discovered; sample_index++)
    {
        const uint8_t *first_chunk = musx_file.sample_chunks[sample_index];
        const size_t chunk_size = musx_file.sample_chunk_sizes[sample_index];
        sample_definition_t definition = {0};
        if (!read_sample_definition(first_chunk, (size_t) first_chunk + chunk_size, &definition))
        {
            return false;
        }
        if (definition.length == 0)
        {
            // Set the instrument slot to unassigned and move on to the next sample.
            module_set_instrument(module, sample_index, UNASSIGNED_INSTRUMENT);
            continue;
        }
        if (definition.data == NULL || definition.data_size < definition.length)
        {
            error(MODFILE_CORRUPT_MISSING_SAMPLE_DATA);
            return false;
        }
        if (definition.volume > MUSX_VOLUME_MAX)
        {
            error(MODFILE_INVALID_VOLUME);
            return false;
        }
        float *sample_data = allocate_array(MODULE, definition.length, sizeof(float));
        if (sample_data == NULL)
        {
            return false;
        }
        for (uint32_t frame = 0; frame < definition.length; frame++)
        {
            sample_data[frame] = vidc_to_linear(definition.data[frame]);
        }
        if (!module_set_sample(module, sample_data, definition.length, sample_rate, MUSX_BASE_NOTE, 0, sample_index))
        {
            deallocate(MODULE, sample_data);
            return false;
        }
        uint32_t repeat_length = definition.repeat_length;
        if (repeat_length > definition.length - definition.repeat_offset || (repeat_length == 2 && definition.repeat_offset > 0))
        {
            // If the repeat length extends beyond the sample length, shorten the repeat length so that it fits within
            // the sample. That part was my decision and ought to be self explanatory. The case of ROFS>0, RLEN=2 is
            // curious, however. The play routine that was shipped with Tracker does this:
            //
            //   LDR   R1,[R3,#40+28]            ; R1 = Repeat Offset
            //   LDR   R4,[R3,#52+28]            ; R4 = Repeat Length
            //   CMP   R4,#2
            //   ADDNE R4,R4,R1
            //   LDREQ R4,[R3,#28+28]            ; R4 = Sample Length
            //   STR   R4,[R7,#12]
            //
            // That is, if RLEN != 2, it adds RLEN to ROFS and stores that as the playback endpoint. But if RLEN == 2,
            // it loads the sample length into R4 and stores that as the playback endpoint instead. In both cases, the
            // repeat offset in R1 is preserved.
            repeat_length = definition.length - definition.repeat_offset;
        }
        //
        // Strictly speaking, Tracker only considers ROFS=0, RLEN=2 to mean no repeat. But a loop of length 1 is not
        // useful and a loop of length 0 is impossible to play, so I will go with the Amiga MOD rule here instead. For
        // what it's worth, Tracker could not play sample loops accurately anyway, which became a real problem with
        // very short sample loops of the kind you find in chiptune modules. In practice, a loop of length 0 would be
        // played back as a loop of length 16 _output_ samples, which surely cannot have been the intention and
        // certainly wasn't useful. (Please God let there not be someone out there who did find a use for this).
        //
        const bool repeats = repeat_length > 2;
        instrument_t instrument = {
            .assigned = true,
            .sample_index = sample_index,
            .transpose = 0,
            .default_volume = definition.volume,
            .repeats = repeats,
            .repeat_offset = definition.repeat_offset,
            .repeat_length = repeat_length,
            .sample_slices = {0},
        };
        snprintf(instrument.name, sizeof(instrument.name), "%s", definition.name);
        module_set_instrument(module, sample_index, instrument);
    }
    return true;
}

static float calculate_sample_rate(const int channels, const int base_note)
{
    // Internally Arctracker uses this to calculate the phase increment per period:
    //   given: base period = period_for_note(sample base note, fine tuning);
    //   phase increment per period = recorded sample rate * base period / output sample rate;
    // We will calculate a recorded sample rate to plug into that formula which gives the correct playback pitch.
    const float sample_period_assumed = assumed_sample_period(channels);
    // This is the period RiscOS would actually give you:
    const float sample_period_playback = channels < 4 ? 34.0f : 32.0f;
    // The Tracker play routine uses this formula to calculate the phase increment per period:
    //   phase increment per period (calculated) = 60000 * 3575872 * sample_period_assumed / 1000000
    // However, the music is actually played back at a different rate whose increment is given by:
    //   65536 * 3575872 * sample_period_playback / 1000000
    // Therefore, for a given base period of:
    const float base_period = period_for_note(base_note, 1.0f);
    // the sample rate we need is:
    return 60000.0f * 3575872.0f * sample_period_assumed / (65536.0f * base_period * sample_period_playback);
}

static float assumed_sample_period(const int channels)
{
    // I don't know why the Tracker play routine assumes these periods in its phase increment calculation. Fortunately,
    // calculating the correct playback pitch does not depend on knowing the reason.
    switch (channels)
    {
        case 1:
            /* fallthrough */
        case 2:
            return 37.0f;
        case 3:
            /* fallthrough */
        case 4:
            return 34.0f;
        default:
            return 26.0f;
    }
}

static bool read_sample_definition(const uint8_t *chunk_addr, const size_t end, sample_definition_t *definition)
{
    while ((size_t) chunk_addr < end)
    {
        const size_t remaining = end - (size_t) chunk_addr;
        if (remaining < CHUNK_HEADER_SIZE)
        {
            error("Truncated chunk header");
            return false;
        }
        if (!samp_sub_chunk_recognised(chunk_addr))
        {
            // Unrecognised sample sub chunk id, which means either (1) the SAMP chunk is corrupt, (2) the SAMP chunk
            // is missing one or more of its sub chunks, or (3) there is extraneous garbage at the end of an otherwise
            // valid SAMP chunk (at least one such example has been seen). We do not need to worry about forwards
            // compatibility with future versions of Tracker, so we will return successfully here and let the loader
            // decide whether or not the SAMP chunk is valid.
            return true;
        }
        const size_t data_size = read_u32_le(chunk_addr + 4);
        if (data_size > remaining - CHUNK_HEADER_SIZE)
        {
            error("Sub chunk extends beyond parent SAMP chunk");
            return false;
        }
        const uint8_t *data = chunk_addr + CHUNK_HEADER_SIZE;
        if (chunk_is(SNAM_CHUNK_ID, chunk_addr))
        {
            if (data_size != MUSX_SAMPLE_NAME_LEN)
            {
                error("Invalid SNAM chunk size");
                return false;
            }
            copy_string(definition->name, (char *) data, MUSX_SAMPLE_NAME_LEN);
        }
        if (chunk_is(SVOL_CHUNK_ID, chunk_addr))
        {
            if (data_size != 4)
            {
                error("Invalid SVOL chunk size");
                return false;
            }
            definition->volume = read_u32_le(data);
        }
        if (chunk_is(SLEN_CHUNK_ID, chunk_addr))
        {
            if (data_size != 4)
            {
                error("Invalid SLEN chunk size");
                return false;
            }
            definition->length = read_u32_le(data);
        }
        if (chunk_is(ROFS_CHUNK_ID, chunk_addr))
        {
            if (data_size != 4)
            {
                error("Invalid ROFS chunk size");
                return false;
            }
            definition->repeat_offset = read_u32_le(data);
        }
        if (chunk_is(RLEN_CHUNK_ID, chunk_addr))
        {
            if (data_size != 4)
            {
                error("Invalid RLEN chunk size");
                return false;
            }
            definition->repeat_length = read_u32_le(data);
        }
        if (chunk_is(SDAT_CHUNK_ID, chunk_addr))
        {
            definition->data = data;
            definition->data_size = data_size;
        }
        chunk_addr = next_chunk_address(chunk_addr);
    }
    return true;
}

static bool samp_sub_chunk_recognised(const uint8_t *chunk_addr)
{
    return chunk_is(SNAM_CHUNK_ID, chunk_addr)
        || chunk_is(SVOL_CHUNK_ID, chunk_addr)
        || chunk_is(SLEN_CHUNK_ID, chunk_addr)
        || chunk_is(ROFS_CHUNK_ID, chunk_addr)
        || chunk_is(RLEN_CHUNK_ID, chunk_addr)
        || chunk_is(SDAT_CHUNK_ID, chunk_addr);
}

static void copy_string(char *destination, const char *source, const int length)
{
    memcpy(destination, source, length);
    destination[length] = '\0';
}
