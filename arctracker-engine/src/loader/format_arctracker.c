#include "format_arctracker.h"
#include <string.h>
#include <unistd.h>
#include "messages.h"
#include "io/error.h"
#include "memory/heap.h"
#include "player/period.h"

/*****************************************************************************
 * Arctracker module file format                                             *
 * -----------------------------                                             *
 * Arctracker module files are written in chunks, inspired by the RIFF file  *
 * format. Every chunk is structured like this:                              *
 *                                                                           *
 *   Offset  Size  Description                                               *
 *   0       4     Chunk id, ASCII FourCC                                    *
 *   4       4     Chunk data length in bytes, excluding this 8-byte header  *
 *   8       N     Chunk data                                                *
 *                                                                           *
 * An Arctracker module file always begins with an ARCT format chunk, which  *
 * is always followed by a META module info chunk. That is then followed by  *
 * other chunks which contain the rest of the module data. Other than the    *
 * format and meta chunks, the chunks may be in any order.                   *
 *                                                                           *
 * All multi-byte number values are stored in little-endian order. The       *
 * meaning of the datatypes are as follows:                                  *
 *                                                                           *
 *   Type  Meaning                                                           *
 *   u8    Unsigned byte (8 bits).                                           *
 *   s8    Signed byte.                                                      *
 *   u16   Unsigned 16-bit word.                                             *
 *   u32   Unsigned 32-bit word.                                             *
 *   f32   32-bit IEEE-754 single precision float.                           *
 *   char  A string of characters, UTF-8 encoded, terminated by NUL. The     *
 *         specified length of the string is the number of bytes allocated   *
 *         in the file. The terminator is always included in this length.    *
 *         When saving, the string may be truncated as necessary to fit.     *
 *                                                                           *
 * Some parts of the file are reserved for future use. For now, these must   *
 * be written with zero, and when reading, they must be ignored.             *
 *                                                                           *
 * A description of the various chunks follows:                              *
 *                                                                           *
 * Format chunk                                                              *
 * ------------                                                              *
 * This is always the first chunk in the file. It is used to identify the    *
 * file as an Arctracker module, and it contains a format version number,    *
 * enabling the loader to decide whether it is capable of loading the file.  *
 *                                                                           *
 *   Chunk id: ARCT                                                          *
 *   Chunk contents:                                                         *
 *   - u32 format version                                                    *
 *                                                                           *
 * Current format version is 1.                                              *
 *                                                                           *
 * Metadata chunk                                                            *
 * --------------                                                            *
 * This always follows the format chunk. It is used to provide the minimum   *
 * information needed to instantiate a module structure, so that the loader  *
 * can iterate over the remaining chunks, hydrating the module as it goes.   *
 *                                                                           *
 *   Chunk id: META                                                          *
 *   Chunk contents:                                                         *
 *   - char module name (256 bytes)                                          *
 *   - char author (256 bytes)                                               *
 *   - u8 number of tracks less 1 (0-255; 0=1 track, 1=2 tracks, etc.)       *
 *   - u16 sequence length (1-65535)                                         *
 *   - u16 master gain (0-65535; 0=minimum, 65535=maximum)                   *
 *   - u8 initial ticks per event (1-255)                                    *
 *   - u8 initial tempo (beats per minute: 1-255)                            *
 *   - u8 lines per beat (0-255: 0=undefined)                                *
 *   - u16 default pattern length (1-1000)                                   *
 *   - u8 interpolation type (0=native, 1=Archimedes)                        *
 *   - u8 volume mapping (0=native/Archimedes, 1=Amiga                       *
 *                                                                           *
 * Track chunk                                                               *
 * -----------                                                               *
 * Per-track metadata. There is one such chunk per track.                    *
 *                                                                           *
 *   Chunk id: TRCK                                                          *
 *   Chunk contents:                                                         *
 *   - u8 track index (0-255)                                                *
 *   - char reserved for future use (128 characters)                         *
 *   - u8 track muted (0-not muted, 1-muted)                                 *
 *   - u8 initial panning (0-255: 0=centre, 1=full left, 255=full right)     *
 *   - u8 commands displayed (0-4)                                           *
 *   - u8 reserved for future use                                            *
 *   - u8 reserved for future use                                            *
 *   - u8 reserved for future use                                            *
 *   - u8 reserved for future use                                            *
 *                                                                           *
 * Sequence chunk                                                            *
 * --------------                                                            *
 * The sequence of patterns played in the song.                              *
 *                                                                           *
 *   Chunk id: SEQU                                                          *
 *   Chunk contents:                                                         *
 *   - u32 pattern ids (array of length specified in metadata chunk)         *
 *                                                                           *
 * Pattern chunk                                                             *
 * -------------                                                             *
 * Pattern data.                                                             *
 *                                                                           *
 *   Chunk id: PATT                                                          *
 *   Chunk contents:                                                         *
 *   - u32 pattern id                                                        *
 *   - u16 number of lines (aka pattern length)                              *
 *   - u8 reserved for future use                                            *
 *   - u8 reserved for future use                                            *
 *   - (array) events                                                        *
 *                                                                           *
 * Events are stored in row-major order:                                     *
 *   line 0, track 0                                                         *
 *   line 0, track 1                                                         *
 *   ...                                                                     *
 *   line 1, track 0                                                         *
 *   line 1, track 1                                                         *
 *   ...                                                                     *
 *                                                                           *
 * Each event is 24 bytes:                                                   *
 *   u8 instrument number                                                    *
 *   u8 note (0-62: 0-no note, 1-C0, 62-B4)                                  *
 *   u8 reserved for future use                                              *
 *   u8 reserved for future use                                              *
 *   u8 reserved for future use                                              *
 *   u8 reserved for future use                                              *
 *   u8 reserved for future use                                              *
 *   u8 reserved for future use                                              *
 *   u8 (command 1) command code                                             *
 *   u8 (command 1) reserved for future use                                  *
 *   u8 (command 1) command data                                             *
 *   u8 (command 1) reserved for future use                                  *
 *   u8 (command 2) command code                                             *
 *   u8 (command 2) reserved for future use                                  *
 *   u8 (command 2) command data                                             *
 *   u8 (command 2) reserved for future use                                  *
 *   u8 (command 3) command code                                             *
 *   u8 (command 3) reserved for future use                                  *
 *   u8 (command 3) command data                                             *
 *   u8 (command 3) reserved for future use                                  *
 *   u8 (command 4) command code                                             *
 *   u8 (command 4) reserved for future use                                  *
 *   u8 (command 4) command data                                             *
 *   u8 (command 4) reserved for future use                                  *
 *                                                                           *
 * Empty pattern chunk                                                       *
 * -------------------                                                       *
 * Represents an empty pattern, it stores no events.                         *
 *                                                                           *
 *   Chunk id: EPAT                                                          *
 *   Chunk contents:                                                         *
 *   - u32 pattern id                                                        *
 *   - u16 number of lines (aka pattern length)                              *
 *   - u8 reserved for future use                                            *
 *   - u8 reserved for future use                                            *
 *                                                                           *
 * Instrument chunk                                                          *
 * ----------------                                                          *
 * The instruments array may be sparsely populated.                          *
 *                                                                           *
 *   Chunk id: INST                                                          *
 *   Chunk contents:                                                         *
 *   - u32 instrument index                                                  *
 *   - char instrument name (32 bytes)                                       *
 *   - u8 instrument type (0-sample)                                         *
 *   - u8 instrument volume (0-255)                                          *
 *   - s8 transpose value (semitones; -11 to +11)                            *
 *   - u8 loop flag (0-does not loop, 1-instrument loops)                    *
 *   - u32 sample index (when instrument type = 0)                           *
 *   - u32 sample loop start (ignored if loop flag = 0)                      *
 *   - u32 sample loop length (ignored if loop flag = 0)                     *
 *                                                                           *
 * Sample chunk                                                              *
 * ------------                                                              *
 * Sample data. The samples array may be sparsely populated. All samples are *
 * mono, and the sample data is stored as 32-bit IEEE-754 single precision,  *
 * little-endian float.                                                      *
 *                                                                           *
 *   Chunk id: SAMP                                                          *
 *   Chunk contents:                                                         *
 *   - u32 sample index                                                      *
 *   - u32 sample length (no of frames)                                      *
 *   - f32 (array) sample data                                               *
 *                                                                           *
 * Sample slices chunk                                                       *
 * -------------------                                                       *
 * Sample slices. The file contains only the slices that are defined for a   *
 * sample, where defined means the offset is greater than 0.                 *
 *                                                                           *
 *   Chunk id: SSLC                                                          *
 *   Chunk contents:                                                         *
 *   - u32 instrument index                                                  *
 *   - u8 number of slices                                                   *
 *   - (array) sample slices                                                 *
 *                                                                           *
 * Each sample slice is 9 bytes:                                             *
 *   u8 slice index                                                          *
 *   u32 offset into sample (frames)                                         *
 *   u32 slice length (frames)                                               *
 *****************************************************************************/

#define MODULE_NAME_LEN 256
#define AUTHOR_NAME_LEN 256
#define TRACK_NAME_LEN 128
#define INSTRUMENT_NAME_LEN 32
#define EVENT_SIZE 24
#define INSTRUMENT_TYPE_SAMPLE 0
#define MAX_PATTERN_LENGTH 1000

static const uint32_t FORMAT_VERSION = 2;
static const char *FORMAT_CHUNK_ID = "ARCT";
static const char *META_CHUNK_ID = "META";
static const char *TRACK_CHUNK_ID = "TRCK";
static const char *SEQUENCE_CHUNK_ID = "SEQU";
static const char *PATTERN_CHUNK_ID = "PATT";
static const char *EMPTY_PATTERN_CHUNK_ID = "EPAT";
static const char *SAMPLE_CHUNK_ID = "SAMP";
static const char *INSTRUMENT_CHUNK_ID = "INST";
static const char *SAMPLE_SLICES_CHUNK_ID = "SSLC";
static const uint32_t CHUNK_HEADER_SIZE = 8;
static const uint32_t FORMAT_CHUNK_LEN = 4;
static const uint32_t META_CHUNK_LEN = MODULE_NAME_LEN + AUTHOR_NAME_LEN + 12;
static const uint32_t TRACK_CHUNK_LEN = TRACK_NAME_LEN + 8;
static const uint32_t EMPTY_PATTERN_CHUNK_LEN = 8;
static const uint32_t INSTRUMENT_CHUNK_LEN = 20 + INSTRUMENT_NAME_LEN;
static const uint32_t MASTER_GAIN_MAX = 65535;

static bool is_arctracker_module(mapped_file_t);
static bool chunk_is(const char *, const uint8_t *);
static module_t *read_arctracker_module(mapped_file_t);
static module_t *instantiate_module(const uint8_t *, size_t);
static uint8_t read_u8(const uint8_t *);
static int8_t read_s8(const uint8_t *);
static uint16_t read_u16_le(const uint8_t *);
static uint32_t read_u32_le(const uint8_t *);
static float read_f32_le(const uint8_t *);
static uint8_t *next_chunk_address(uint8_t *);
static float read_gain(uint32_t);
static bool read_sequence_chunk(const uint8_t *, size_t, module_t *);
static bool read_track_chunk(const uint8_t *data, size_t, const module_t *);
static bool read_pattern_chunk(const uint8_t *data, size_t, module_t *);
static event_t read_pattern_event(const uint8_t *);
static bool read_empty_pattern_chunk(const uint8_t *, size_t, module_t *);
static bool read_instrument_chunk(const uint8_t *, size_t, module_t *);
static bool read_sample_chunk(const uint8_t *, size_t, module_t *);
static bool read_sample_slices_chunk(const uint8_t *, size_t, module_t *);
static bool validate_module(const module_t *);
static bool write_arctracker_module(const module_t *, FILE *);
static bool write_format_chunk(FILE *);
static bool write_meta_chunk(const module_t *, FILE *);
static bool write_track_chunks(const module_t *, FILE *);
static bool write_track_chunk(const module_t *, int track, FILE *);
static bool write_sequence_chunk(const module_t *, FILE *);
static bool write_pattern_chunks(const module_t *, FILE *);
static bool write_pattern_chunk(const module_t *, int, FILE *);
static bool write_empty_pattern_chunk(int, int, FILE *);
static bool pattern_is_empty(pattern_t, const module_t*);
static bool event_is_empty(event_t);
static bool effect_is_empty(effect_t);
static bool write_pattern_events(pattern_t, const module_t *, FILE *fp);
static bool write_event(event_t, FILE *);
static bool write_instrument_chunks(const module_t *, FILE *, bool *);
static bool write_instrument_chunk(const module_t *, uint8_t, FILE *, bool *);
static bool write_sample_chunks(const module_t *, const bool *, FILE *);
static bool write_sample_chunk(const module_t *, uint32_t, FILE *);
static bool write_sample_slices_chunks(const module_t *, FILE *);
static bool write_sample_slices_chunk(const module_t *, uint8_t, uint8_t, FILE *);
static uint32_t write_gain(float);
static bool write_fourcc(FILE *, const char *);
static bool write_cc(FILE *, const char *, size_t);
static bool write_u8(FILE *, uint8_t);
static bool write_s8(FILE *, int8_t);
static bool write_u16_le(FILE *, uint16_t);
static bool write_u32_le(FILE *, uint32_t);
static bool write_f32_le(FILE *fp, float);

format_t arctracker_format(void)
{
    return (format_t){
        .is_this_format = is_arctracker_module,
        .read_module = read_arctracker_module,
        .write_module = write_arctracker_module,
    };
}

static bool is_arctracker_module(const mapped_file_t mapped_file)
{
    if (mapped_file.size < 12) return false;
    if (!chunk_is(FORMAT_CHUNK_ID, mapped_file.addr)) return false;
    const uint32_t format_version = read_u32_le(mapped_file.addr + 8);
    return format_version == FORMAT_VERSION;
}

static bool chunk_is(const char *chunk_id, const uint8_t *addr)
{
    return memcmp(chunk_id, addr, 4) == 0;
}

/*****************************************************************************
 * Code for reading a module file.                                           *
 *****************************************************************************/

static module_t *read_arctracker_module(const mapped_file_t mapped_file)
{
    const size_t file_end = (size_t) mapped_file.addr + mapped_file.size;
    uint8_t *chunk_addr = next_chunk_address(mapped_file.addr);
    if (!chunk_is(META_CHUNK_ID, chunk_addr))
    {
        error(MODULE_METADATA_MISSING);
        return NULL;
    }
    const size_t meta_data_size = read_u32_le(chunk_addr + 4);
    const uint8_t *meta_data = chunk_addr + CHUNK_HEADER_SIZE;
    module_t *module = instantiate_module(meta_data, meta_data_size);
    if (module == NULL)
    {
        return NULL;
    }
    chunk_addr = next_chunk_address(chunk_addr);
    while ((size_t) chunk_addr < file_end)
    {
        bool ok = true;
        const uint8_t *data = chunk_addr + CHUNK_HEADER_SIZE;
        const size_t data_size = read_u32_le(chunk_addr + 4);
        if ((size_t) data + data_size > file_end)
        {
            error(CHUNK_EXTENDS_BEYOND_EOF);
            goto read_arctracker_module_failed;
        }
        if (chunk_is(SEQUENCE_CHUNK_ID, chunk_addr))
            ok = read_sequence_chunk(data, data_size, module);
        if (chunk_is(TRACK_CHUNK_ID, chunk_addr))
            ok = read_track_chunk(data, data_size, module);
        if (chunk_is(PATTERN_CHUNK_ID, chunk_addr))
            ok = read_pattern_chunk(data, data_size, module);
        if (chunk_is(EMPTY_PATTERN_CHUNK_ID, chunk_addr))
            ok = read_empty_pattern_chunk(data, data_size, module);
        if (chunk_is(INSTRUMENT_CHUNK_ID, chunk_addr))
            ok = read_instrument_chunk(data, data_size, module);
        if (chunk_is(SAMPLE_CHUNK_ID, chunk_addr))
            ok = read_sample_chunk(data, data_size, module);
        if (chunk_is(SAMPLE_SLICES_CHUNK_ID, chunk_addr))
            ok = read_sample_slices_chunk(data, data_size, module);
        if (!ok)
            goto read_arctracker_module_failed;
        chunk_addr = next_chunk_address(chunk_addr);
    }
    if (!validate_module(module))
    {
        goto read_arctracker_module_failed;
    }
    return module;
    //
    // Something went wrong while we were populating the module; destroy it and return nothing.
    //
    read_arctracker_module_failed:
        module_destroy(module);
        return NULL;
}

static module_t *instantiate_module(const uint8_t *meta_data, const size_t data_size)
{
    if (data_size < META_CHUNK_LEN)
    {
        error(INVALID_META_CHUNK_LENGTH);
        return NULL;
    }
    char module_name[MODULE_NAME_LEN + 1];
    memcpy(module_name, meta_data, MODULE_NAME_LEN);
    module_name[MODULE_NAME_LEN] = '\0';
    char author[AUTHOR_NAME_LEN + 1];
    memcpy(author, meta_data + MODULE_NAME_LEN, AUTHOR_NAME_LEN);
    author[AUTHOR_NAME_LEN] = '\0';
    const uint8_t num_tracks = read_u8(meta_data + MODULE_NAME_LEN + AUTHOR_NAME_LEN);
    const uint16_t sequence_length = read_u16_le(meta_data + 1 + MODULE_NAME_LEN + AUTHOR_NAME_LEN);
    if (sequence_length == 0)
    {
        error(MODFILE_INVALID_SEQUENCE_LENGTH);
        return NULL;
    }
    module_t *module = module_create(num_tracks + 1, sequence_length, 0, 0);
    if (module == NULL)
    {
        return NULL;
    }
    module_set_name(module, module_name);
    module_set_author(module, author);
    const uint16_t master_gain = read_u16_le(meta_data + 3 + MODULE_NAME_LEN + AUTHOR_NAME_LEN);
    if (master_gain > MASTER_GAIN_MAX)
    {
        error(MODFILE_INVALID_MASTER_GAIN);
        goto read_module_metadata_failed;
    }
    module->master_gain = read_gain(master_gain);
    const uint8_t initial_speed = read_u8(meta_data + 5 + MODULE_NAME_LEN + AUTHOR_NAME_LEN);
    if (initial_speed == 0)
    {
        error(MODFILE_INVALID_INITIAL_SPEED);
        goto read_module_metadata_failed;
    }
    module->initial_ticks_per_event = initial_speed;
    const uint8_t initial_bpm = read_u8(meta_data + 6 + MODULE_NAME_LEN + AUTHOR_NAME_LEN);
    const uint8_t lines_per_beat = read_u8(meta_data + 7 + MODULE_NAME_LEN + AUTHOR_NAME_LEN);
    module_set_lines_per_beat(module, lines_per_beat);
    module_set_initial_bpm(module, initial_bpm);
    const uint16_t default_pattern_length = read_u16_le(meta_data + 8 + MODULE_NAME_LEN + AUTHOR_NAME_LEN);
    if (default_pattern_length == 0 || default_pattern_length > MAX_PATTERN_LENGTH)
    {
        error(MODFILE_INVALID_DEFAULT_PATTERN_LENGTH);
        goto read_module_metadata_failed;
    }
    const uint8_t interpolation_type = read_u8(meta_data + 10 + MODULE_NAME_LEN + AUTHOR_NAME_LEN);
    if (interpolation_type != 0 && interpolation_type != 1)
    {
        error(MODFILE_INVALID_INTERPOLATION_TYPE);
        goto read_module_metadata_failed;
    }
    const uint8_t volume_mapping = read_u8(meta_data + 11 + MODULE_NAME_LEN + AUTHOR_NAME_LEN);
    if (volume_mapping != 0 && volume_mapping != 1)
    {
        error(MODFILE_INVALID_VOLUME_MAPPING);
        goto read_module_metadata_failed;
    }
    module->default_pattern_length = default_pattern_length;
    module->interpolation_type = interpolation_type == 0 ? LINEAR : NONE;
    module->volume_mapping_type = volume_mapping == 0 ? VOLUME_ARCHIMEDES : VOLUME_AMIGA;
    return module;

read_module_metadata_failed:
    module_destroy(module);
    return NULL;
}

static uint8_t read_u8(const uint8_t *addr)
{
    return *addr;
}

static int8_t read_s8(const uint8_t *addr)
{
    return (int8_t) *addr;
}

static uint16_t read_u16_le(const uint8_t *addr)
{
    return addr[0] | addr[1] << 8;
}

static uint32_t read_u32_le(const uint8_t *addr)
{
    return addr[0] | addr[1] << 8 | addr[2] << 16 | addr[3] << 24;
}

static float read_f32_le(const uint8_t *addr)
{
    const uint32_t bits = read_u32_le(addr);
    float value;
    memcpy(&value, &bits, sizeof value);
    return value;
}

static uint8_t *next_chunk_address(uint8_t *chunk_address)
{
    const uint32_t chunk_length = read_u32_le(chunk_address + 4);
    return chunk_address + chunk_length + CHUNK_HEADER_SIZE;
}

static float read_gain(const uint32_t gain)
{
    return (float) gain / (float) MASTER_GAIN_MAX;
}

static bool read_sequence_chunk(const uint8_t *data, const size_t data_size, module_t *module)
{
    if (data_size != (size_t) module->sequence_length * 4)
    {
        error(INVALID_SEQUENCE_CHUNK_LENGTH);
        return false;
    }
    // TODO: Change module->sequence to be array of uint32_t so I don't have to do this.
    int *sequence = allocate_array(MODULE, module->sequence_length, sizeof(int));
    if (sequence == NULL)
        return false;
    for (uint32_t i = 0; i < (uint32_t) module->sequence_length; i++)
        sequence[i] = (int) read_u32_le(data + i * 4);
    const bool ok = module_set_sequence(module, sequence, module->sequence_length);
    if (!ok) error(FAILED_TO_SET_SEQUENCE);
    deallocate(MODULE, sequence);
    return ok;
}

static bool read_track_chunk(const uint8_t *data, const size_t data_size, const module_t *module)
{
    if (data_size < TRACK_CHUNK_LEN)
    {
        error(INVALID_TRACK_CHUNK_LENGTH);
        return false;
    }
    const uint8_t track = read_u8(data);
    if (track >= (uint8_t) module->num_tracks)
    {
        error(MODFILE_INVALID_TRACK_ID);
        return false;
    }
    const uint8_t muted = read_u8(data + TRACK_NAME_LEN + 1);
    if (muted != 0 && muted != 1)
    {
        error(MODFILE_INVALID_TRACK_MUTE_STATE);
        return false;
    }
    const uint8_t initial_panning = read_u8(data + TRACK_NAME_LEN + 2);
    const uint8_t commands_displayed = read_u8(data + TRACK_NAME_LEN + 3);
    if (commands_displayed > 4)
    {
        error(MODFILE_INVALID_EFFECTS_DISPLAYED);
        return false;
    }
    module->tracks[track].panning = initial_panning;
    module->tracks[track].muted = muted == 1;
    module->tracks[track].effects_displayed = commands_displayed;
    return true;
}

static bool read_pattern_chunk(const uint8_t *data, const size_t data_size, module_t *module)
{
    const uint32_t pattern_no = read_u32_le(data);
    if (pattern_no > UINT16_MAX)
    {
        error(MODFILE_INVALID_PATTERN_NUMBER);
        return false;
    }
    const uint16_t num_lines = read_u16_le(data + 4);
    if (num_lines == 0 || num_lines > 1000)
    {
        error(MODFILE_INVALID_PATTERN_LENGTH);
        return false;
    }
    if (data_size != 8 + module->num_tracks * (size_t) num_lines * EVENT_SIZE)
    {
        error(INVALID_PATTERN_CHUNK_LENGTH);
        return false;
    }
    if (!module_create_pattern(module, pattern_no, num_lines))
    {
        error(FAILED_TO_CREATE_PATTERN);
        return false;
    }
    const pattern_t pattern = module->patterns[pattern_no];
    for (uint32_t line = 0; line < num_lines; line++)
    {
        for (int track = 0; track < module->num_tracks; track++)
        {
            const size_t data_index = 8 + EVENT_SIZE * (track + line * module->num_tracks);
            const uint32_t event_index = track + line * module->track_capacity;
            const event_t event = read_pattern_event(data + data_index);
            pattern.events[event_index] = event;
        }
    }
    return true;
}

static event_t read_pattern_event(const uint8_t *data)
{
    event_t event = {0};
    const uint8_t instrument_no = read_u8(data);
    const uint8_t note = read_u8(data + 1);
    if (!note_out_of_range(note)) event.note = note;
    event.instrument_no = instrument_no;
    for (int effect = 0; effect < 4; effect++)
    {
        const size_t effect_offset = 8 + effect * 4;
        const uint8_t effect_command = read_u8(data + effect_offset);
        const uint8_t effect_data = read_u8(data + effect_offset + 2);
        event.effects[effect] = (effect_t) {
            .command = (char) effect_command,
            .data = effect_data,
        };
    }
    return event;
}

static bool read_empty_pattern_chunk(const uint8_t *data, const size_t data_size, module_t *module)
{
    const uint32_t pattern_no = read_u32_le(data);
    if (pattern_no > UINT16_MAX)
    {
        error(MODFILE_INVALID_PATTERN_NUMBER);
        return false;
    }
    const uint16_t num_lines = read_u16_le(data + 4);
    if (num_lines == 0 || num_lines > 1000)
    {
        error(MODFILE_INVALID_PATTERN_LENGTH);
        return false;
    }
    if (data_size < EMPTY_PATTERN_CHUNK_LEN)
    {
        error(INVALID_EPAT_CHUNK_LENGTH);
        return false;
    }
    if (!module_create_pattern(module, pattern_no, num_lines))
    {
        error(FAILED_TO_CREATE_PATTERN);
        return false;
    }
    return true;
}

static bool read_instrument_chunk(const uint8_t *data, const size_t data_size, module_t *module)
{
    if (data_size < INSTRUMENT_CHUNK_LEN)
    {
        error(INVALID_INSTRUMENT_CHUNK_LENGTH);
        return false;
    }
    const uint32_t instrument_index = read_u32_le(data);
    char instrument_name[INSTRUMENT_NAME_LEN];
    memcpy(instrument_name, data + 4, INSTRUMENT_NAME_LEN);
    instrument_name[INSTRUMENT_NAME_LEN - 1] = '\0';
    const uint8_t instrument_type = read_u8(data + 4 + INSTRUMENT_NAME_LEN);
    if (instrument_type != INSTRUMENT_TYPE_SAMPLE)
    {
        error(MODFILE_INVALID_INSTRUMENT_TYPE);
        return false;
    }
    const uint8_t instrument_volume = read_u8(data + 5 + INSTRUMENT_NAME_LEN);
    const int8_t transpose = read_s8(data + 6 + INSTRUMENT_NAME_LEN);
    if (transpose < -12 || transpose > 12)
    {
        error(MODFILE_INVALID_TRANSPOSE);
        return false;
    }
    const uint8_t sample_loop_flag = read_u8(data + 7 + INSTRUMENT_NAME_LEN);
    if (sample_loop_flag != 0 && sample_loop_flag != 1)
    {
        error(MODFILE_INVALID_SAMPLE_LOOP_FLAG);
        return false;
    }
    const uint32_t sample_index = read_u32_le(data + 8 + INSTRUMENT_NAME_LEN);
    const uint32_t sample_loop_start = read_u32_le(data + 12 + INSTRUMENT_NAME_LEN);
    const uint32_t sample_loop_length = read_u32_le(data + 16 + INSTRUMENT_NAME_LEN);
    instrument_t *instrument = &module->instruments[instrument_index];
    instrument->assigned = true;
    snprintf(instrument->name, sizeof instrument->name, "%s", instrument_name);
    instrument->default_volume = instrument_volume;
    instrument->transpose = transpose + 13;
    instrument->repeats = sample_loop_flag == 1;
    instrument->repeat_offset = (int) sample_loop_start;
    instrument->repeat_length = (int) sample_loop_length;
    instrument->sample_index = (int) sample_index;
    return true;
}

static bool read_sample_chunk(const uint8_t *data, const size_t data_size, module_t *module)
{
    const uint32_t sample_index = read_u32_le(data);
    const uint32_t sample_length = read_u32_le(data + 4);
    if (data_size != 8 + sample_length * 4)
    {
        error(MODFILE_INVALID_SAMPLE_DATA_LENGTH);
        return false;
    }
    float *copied_sample_data = allocate_array(MODULE, sample_length + 2, sizeof(float));
    if (copied_sample_data == NULL) return false;
    for (uint32_t sample = 0; sample < sample_length; sample++)
        copied_sample_data[sample] = read_f32_le(data + 8 + sample * 4);
    const bool ok = module_set_sample(module, copied_sample_data, sample_length, 8287.14f, 24, 0, sample_index);
    if (!ok) deallocate(MODULE, copied_sample_data);
    return ok;
}

static bool read_sample_slices_chunk(const uint8_t *data, const size_t data_size, module_t *module)
{
    const uint32_t instrument_index = read_u32_le(data);
    const uint8_t slice_count = read_u8(data + 4);
    if (data_size != 5 + slice_count * 9)
    {
        error(MODFILE_INVALID_INVALID_SAMPLE_SLICE_LENGTH);
        return false;
    }
    instrument_t *instrument = &module->instruments[instrument_index];
    for (int slice = 0; slice < slice_count; slice++)
    {
        const uint8_t *slice_addr = data + 5 + slice * 9;
        const uint8_t slice_index = read_u8(slice_addr);
        instrument->sample_slices[slice_index].offset = read_u32_le(slice_addr + 1);
        instrument->sample_slices[slice_index].length = read_u32_le(slice_addr + 5);
    }
    return true;
}

static bool validate_module(const module_t *module)
{
    for (int sequence_index = 0; sequence_index < module->sequence_length; sequence_index++)
    {
        const int pattern_no = module->sequence[sequence_index];
        if (pattern_no >= module->num_patterns)
        {
            error(INVALID_SEQUENCE_PATTERN_NO);
            return false;
        }
    }
    for (int instrument_index = 0; instrument_index < 256; instrument_index++)
    {
        const instrument_t instrument = module->instruments[instrument_index];
        if (!instrument.assigned) continue;
        const int sample_index = instrument.sample_index;
        if (sample_index >= module->sample_slots || module->samples[sample_index].sample_length == 0)
        {
            error(MODFILE_INVALID_SAMPLE_INDEX);
            return false;
        }
    }
    return true;
}

/*****************************************************************************
 * Code for writing a module file.                                           *
 *****************************************************************************/

static bool write_arctracker_module(const module_t *module, FILE *fp)
{
    bool *samples_used = allocate_array(MODULE, module->sample_slots, sizeof(bool));
    if (samples_used == NULL) return false;
    for (int i = 0; i < module->sample_slots; i++) samples_used[i] = false;
    if (!write_format_chunk(fp)) goto write_failed;
    if (!write_meta_chunk(module, fp)) goto write_failed;
    if (!write_track_chunks(module, fp)) goto write_failed;
    if (!write_sequence_chunk(module, fp)) goto write_failed;
    if (!write_pattern_chunks(module, fp)) goto write_failed;
    if (!write_instrument_chunks(module, fp, samples_used)) goto write_failed;
    if (!write_sample_slices_chunks(module, fp)) goto write_failed;
    if (!write_sample_chunks(module, samples_used, fp)) goto write_failed;
    deallocate(MODULE, samples_used);
    return true;

write_failed:
    deallocate(MODULE, samples_used);
    return false;
}

static bool write_format_chunk(FILE *fp)
{
    if (!write_fourcc(fp, FORMAT_CHUNK_ID)) return false;
    if (!write_u32_le(fp, FORMAT_CHUNK_LEN)) return false;
    if (!write_u32_le(fp, FORMAT_VERSION)) return false;
    return true;
}

static bool write_meta_chunk(const module_t *module, FILE *fp)
{
    char module_name[256], author[256];
    snprintf(module_name, sizeof module_name, "%s", module->name);
    snprintf(author, sizeof author, "%s", module->author);
    if (!write_fourcc(fp, META_CHUNK_ID)) return false;
    if (!write_u32_le(fp, META_CHUNK_LEN)) return false;
    if (!write_cc(fp, module_name, sizeof module_name)) return false;
    if (!write_cc(fp, author, sizeof author)) return false;
    if (!write_u8(fp, module->num_tracks - 1)) return false;
    if (!write_u16_le(fp, module->sequence_length)) return false;
    if (!write_u16_le(fp, write_gain(module->master_gain))) return false;
    if (!write_u8(fp, module->initial_ticks_per_event)) return false;
    if (!write_u8(fp, module->initial_bpm)) return false;
    if (!write_u8(fp, module->lines_per_beat)) return false;
    if (!write_u16_le(fp, module->default_pattern_length)) return false;
    if (!write_u8(fp, module->interpolation_type == LINEAR ? 0 : 1)) return false;
    if (!write_u8(fp, module->volume_mapping_type == VOLUME_ARCHIMEDES ? 0 : 1)) return false;
    return true;
}

static bool write_track_chunks(const module_t *module, FILE *fp)
{
    for (int track = 0; track < module->num_tracks; track++)
        if (!write_track_chunk(module, track, fp)) return false;
    return true;
}

static bool write_track_chunk(const module_t *module, const int track, FILE *fp)
{
    const char reserved_for_track_name[TRACK_NAME_LEN] = {0};
    if (!write_fourcc(fp, TRACK_CHUNK_ID)) return false;
    if (!write_u32_le(fp, TRACK_CHUNK_LEN)) return false;
    if (!write_u8(fp, (uint8_t) track)) return false;
    if (!write_cc(fp, reserved_for_track_name, sizeof reserved_for_track_name)) return false;
    if (!write_u8(fp, (uint8_t) module->tracks[track].muted ? 1 : 0)) return false;
    if (!write_u8(fp, (uint8_t) module->tracks[track].panning)) return false;
    if (!write_u8(fp, (uint8_t) module->tracks[track].effects_displayed)) return false;
    if (!write_u8(fp, 0)) return false;
    if (!write_u8(fp, 0)) return false;
    if (!write_u8(fp, 0)) return false;
    if (!write_u8(fp, 0)) return false;
    return true;
}

static bool write_sequence_chunk(const module_t *module, FILE *fp)
{
    if (!write_fourcc(fp, SEQUENCE_CHUNK_ID)) return false;
    if (!write_u32_le(fp, (uint32_t) module->sequence_length * 4)) return false;
    for (int p = 0; p < module->sequence_length; p++)
        if (!write_u32_le(fp, (uint32_t) module->sequence[p])) return false;
    return true;
}

static bool write_pattern_chunks(const module_t *module, FILE *fp)
{
    for (int pno = 0; pno < module->num_patterns; pno++)
        if (!write_pattern_chunk(module, pno, fp)) return false;
    return true;
}

static bool write_pattern_chunk(const module_t *module, const int pattern_no, FILE *fp)
{
    const pattern_t pattern = module->patterns[pattern_no];
    if (pattern_is_empty(pattern, module))
    {
        return write_empty_pattern_chunk(pattern_no, pattern.num_lines, fp);
    }
    if (!write_fourcc(fp, PATTERN_CHUNK_ID)) return false;
    if (!write_u32_le(fp, 8 + pattern.num_lines * module->num_tracks * EVENT_SIZE)) return false;
    if (!write_u32_le(fp, (uint32_t) pattern_no)) return false;
    if (!write_u16_le(fp, (uint16_t) pattern.num_lines)) return false;
    if (!write_u8(fp, 0)) return false;
    if (!write_u8(fp, 0)) return false;
    if (!write_pattern_events(pattern, module, fp)) return false;
    return true;
}

static bool write_empty_pattern_chunk(const int pattern_no, const int num_lines, FILE *fp)
{
    if (!write_fourcc(fp, EMPTY_PATTERN_CHUNK_ID)) return false;
    if (!write_u32_le(fp, EMPTY_PATTERN_CHUNK_LEN)) return false;
    if (!write_u32_le(fp, (uint32_t) pattern_no)) return false;
    if (!write_u16_le(fp, (uint16_t) num_lines)) return false;
    if (!write_u8(fp, 0)) return false;
    if (!write_u8(fp, 0)) return false;
    return true;
}

static bool pattern_is_empty(const pattern_t pattern, const module_t *module)
{
    const uint32_t track_capacity = module->track_capacity;
    const uint32_t num_tracks = module->num_tracks;
    for (int line = 0; line < pattern.num_lines; line++)
        for (uint32_t track = 0; track < num_tracks; track++)
            if (!event_is_empty(pattern.events[track + line * track_capacity])) return false;
    return true;
}

static bool event_is_empty(const event_t event)
{
    if (event.note != 0) return false;
    if (event.instrument_no != 0) return false;
    for (uint32_t effect = 0; effect < 4; effect++)
        if (!effect_is_empty(event.effects[effect])) return false;
    return true;
}

static bool effect_is_empty(const effect_t effect)
{
    return effect.command == 0 && effect.data == 0;
}

static bool write_pattern_events(const pattern_t pattern, const module_t *module, FILE *fp)
{
    const uint32_t track_capacity = module->track_capacity;
    const uint32_t num_tracks = module->num_tracks;
    for (int line = 0; line < pattern.num_lines; line++)
        for (uint32_t track = 0; track < num_tracks; track++)
            if (!write_event(pattern.events[track + line * track_capacity], fp))
                return false;
    return true;
}

static bool write_event(const event_t event, FILE *fp)
{
    if (!write_u8(fp, (uint8_t) event.instrument_no)) return false;
    if (!write_u8(fp, (uint8_t) event.note)) return false;
    if (!write_u8(fp, 0)) return false;
    if (!write_u8(fp, 0)) return false;
    if (!write_u8(fp, 0)) return false;
    if (!write_u8(fp, 0)) return false;
    if (!write_u8(fp, 0)) return false;
    if (!write_u8(fp, 0)) return false;
    for (int effect_no = 0; effect_no < 4; effect_no++)
    {
        if (!write_u8(fp, (uint8_t) event.effects[effect_no].command)) return false;
        if (!write_u8(fp, 0)) return false;
        if (!write_u8(fp, event.effects[effect_no].data)) return false;
        if (!write_u8(fp, 0)) return false;
    }
    return true;
}

static bool write_instrument_chunks(const module_t *module, FILE *fp, bool *samples_used)
{
    for (uint32_t instrument_index = 0; instrument_index < 256; instrument_index++)
        if (!write_instrument_chunk(module, instrument_index, fp, samples_used)) return false;
    return true;
}

static bool write_instrument_chunk(const module_t *module, const uint8_t instrument_index, FILE *fp, bool *samples_used)
{
    const instrument_t instrument = module->instruments[instrument_index];
    if (!instrument.assigned) return true;
    samples_used[instrument.sample_index] = true;
    char instrument_name[INSTRUMENT_NAME_LEN];
    snprintf(instrument_name, sizeof instrument_name, "%s", instrument.name);
    if (!write_fourcc(fp, INSTRUMENT_CHUNK_ID)) return false;
    if (!write_u32_le(fp, INSTRUMENT_CHUNK_LEN)) return false;
    if (!write_u32_le(fp, instrument_index)) return false;
    if (!write_cc(fp, instrument_name, INSTRUMENT_NAME_LEN)) return false;
    if (!write_u8(fp, INSTRUMENT_TYPE_SAMPLE)) return false;
    if (!write_u8(fp, instrument.default_volume)) return false;
    if (!write_s8(fp, instrument.transpose - 13)) return false;
    if (!write_u8(fp, instrument.repeats ? 1 : 0)) return false;
    if (!write_u32_le(fp, instrument.sample_index)) return false;
    if (!write_u32_le(fp, instrument.repeat_offset)) return false;
    if (!write_u32_le(fp, instrument.repeat_length)) return false;
    return true;
}

static bool write_sample_chunks(const module_t *module, const bool *samples_used, FILE *fp)
{
    for (int sample_index = 0; sample_index < module->sample_slots; sample_index++)
    {
        if (!samples_used[sample_index]) continue;
        if (!write_sample_chunk(module, sample_index, fp)) return false;
    }
    return true;
}

static bool write_sample_chunk(const module_t *module, const uint32_t sample_index, FILE *fp)
{
    _Static_assert(sizeof(float) == 4, "float must be 32-bit");
    const sample_t sample = module->samples[sample_index];
    if (!write_fourcc(fp, SAMPLE_CHUNK_ID)) return false;
    if (!write_u32_le(fp, 8 + sample.sample_length * 4)) return false;
    if (!write_u32_le(fp, sample_index)) return false;
    if (!write_u32_le(fp, sample.sample_length)) return false;
    for (int i = 0; i < sample.sample_length; i++)
        if (!write_f32_le(fp, sample.sample_data[i])) return false;
    return true;
}

static bool write_sample_slices_chunks(const module_t *module, FILE *fp)
{
    for (int instrument_index = 0; instrument_index <= 255; instrument_index++)
    {
        const instrument_t instrument = module->instruments[instrument_index];
        if (!instrument.assigned) continue;
        uint8_t slices_defined = 0;
        for (int slice_index = 0; slice_index <= 255; slice_index++)
            if (instrument.sample_slices[slice_index].length > 0) slices_defined++;
        if (slices_defined == 0)
            continue;
        if (!write_sample_slices_chunk(module, instrument_index, slices_defined, fp)) return false;
    }
    return true;
}

static bool write_sample_slices_chunk(const module_t *module, const uint8_t instrument_index, const uint8_t slice_count, FILE *fp)
{
    const instrument_t instrument = module->instruments[instrument_index];
    if (!write_fourcc(fp, SAMPLE_SLICES_CHUNK_ID)) return false;
    if (!write_u32_le(fp, 5 + slice_count * 9)) return false;
    if (!write_u32_le(fp, instrument_index)) return false;
    if (!write_u8(fp, slice_count)) return false;
    for (int slice_index = 0; slice_index < 256; slice_index++)
    {
        const sample_slice_t slice = instrument.sample_slices[slice_index];
        if (slice.length == 0) continue;
        if (!write_u8(fp, (uint8_t) slice_index)) return false;
        if (!write_u32_le(fp, slice.offset)) return false;
        if (!write_u32_le(fp, slice.length)) return false;
    }
    return true;
}

static uint32_t write_gain(const float gain)
{
    if (gain < 0.0f) return 0;
    if (gain > 1.0f) return MASTER_GAIN_MAX;
    return (uint32_t) (gain * MASTER_GAIN_MAX);
}

static bool write_fourcc(FILE *fp, const char *ch)
{
    return write_cc(fp, ch, 4);
}

static bool write_cc(FILE *fp, const char *ch, const size_t len)
{
    return fwrite(ch, sizeof(char), len, fp) == len;
}

static bool write_u8(FILE *fp, const uint8_t value)
{
    return fwrite(&value, sizeof value, 1, fp) == 1;
}

static bool write_s8(FILE *fp, const int8_t value)
{
    return fwrite(&value, sizeof value, 1, fp) == 1;
}

static bool write_u16_le(FILE *fp, const uint16_t value)
{
    const uint8_t bytes[2] = {
        (uint8_t) (value & 0xFF),
        (uint8_t) (value >> 8 & 0xFF)
    };
    return fwrite(bytes, sizeof bytes, 1, fp) == 1;
}

static bool write_u32_le(FILE *fp, const uint32_t value)
{
    const uint8_t bytes[4] = {
        (uint8_t) (value & 0xFF),
        (uint8_t) (value >> 8 & 0xFF),
        (uint8_t) (value >> 16 & 0xFF),
        (uint8_t) (value >> 24 & 0xFF)
    };
    return fwrite(bytes, sizeof bytes, 1, fp) == 1;
}

static bool write_f32_le(FILE *fp, const float value)
{
    uint32_t bits;
    memcpy(&bits, &value, sizeof bits);
    return write_u32_le(fp, bits);
}
