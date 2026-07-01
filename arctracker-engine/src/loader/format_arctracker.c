#include "format_arctracker.h"
#include <string.h>
#include <unistd.h>
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
 * zero or more other chunks which contain the rest of the module data. The  *
 * chunks may be in any order, except for the format and meta chunks.        *
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
 *   - u32  number of tracks (1-256)                                         *
 *   - u32  sequence length (1-65535)                                        *
 *   - char module name (68 characters, null terminated)                     *
 *   - char author (68 characters, null terminated)                          *
 *   - u32  master gain (0-65535)                                            *
 *   - u32  ticks per event (1-255)                                          *
 *   - u32  ticks per second (1-255)                                         *
 *                                                                           *
 * Track chunk                                                               *
 * -----------                                                               *
 * Per-track metadata. There is one such chunk per track.                    *
 *                                                                           *
 *   Chunk id: TRCK                                                          *
 *   Chunk contents:                                                         *
 *   - u32 track index (0-255)                                               *
 *   - u32 initial panning (0-255: 0-centre, 1-full left, 255-full right)    *
 *   - u32 effects displayed (0-4)                                           *
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
 * Pattern data. Empty patterns may be omitted from the file.                *
 *                                                                           *
 *   Chunk id: PATT                                                          *
 *   Chunk contents:                                                         *
 *   - u32 pattern id                                                        *
 *   - u32 number of lines (aka pattern length)                              *
 *   - events                                                                *
 *                                                                           *
 * Events are stored in row-major order:                                     *
 *   line 0, track 0                                                         *
 *   line 0, track 1                                                         *
 *   ...                                                                     *
 *   line 1, track 0                                                         *
 *   line 1, track 1                                                         *
 *   ...                                                                     *
 *                                                                           *
 * Each event is 40 bytes:                                                   *
 *   u32 note (0-62: 0-no note, 1-C0, 62-B4)                                 *
 *   u32 instrument number                                                   *
 *   u32 effect 1 command                                                    *
 *   u32 effect 1 data                                                       *
 *   u32 effect 2 command                                                    *
 *   u32 effect 2 data                                                       *
 *   u32 effect 3 command                                                    *
 *   u32 effect 3 data                                                       *
 *   u32 effect 4 command                                                    *
 *   u32 effect 4 data                                                       *
 *                                                                           *
 *****************************************************************************/

#define MODULE_NAME_LEN 68
#define AUTHOR_NAME_LEN 68
#define EVENT_SIZE 40

static const uint32_t FORMAT_VERSION = 1;
static const char *FORMAT_CHUNK_ID = "ARCT";
static const char *META_CHUNK_ID = "META";
static const char *TRACK_CHUNK_ID = "TRCK";
static const char *SEQUENCE_CHUNK_ID = "SEQU";
static const char *PATTERN_CHUNK_ID = "PATT";
static const uint32_t CHUNK_HEADER_SIZE = 8;
static const uint32_t FORMAT_CHUNK_LEN = 4;
static const uint32_t META_CHUNK_LEN = MODULE_NAME_LEN + AUTHOR_NAME_LEN + 20;
static const uint32_t TRACK_CHUNK_LEN = 12;
static const uint32_t MASTER_GAIN_MAX = 65535;

static bool is_arctracker_module(mapped_file_t);
static bool chunk_is(const char *, const uint8_t *);
static module_t *read_arctracker_module(mapped_file_t);
static module_t *instantiate_module(const uint8_t *, size_t);
static uint32_t read_u32_le(const uint8_t *);
static uint8_t *next_chunk_address(uint8_t *);
static bool read_sequence_chunk(const uint8_t *, size_t, module_t *);
static bool read_track_chunk(const uint8_t *data, size_t, const module_t *);
static bool read_pattern_chunk(const uint8_t *data, size_t, module_t *);
static bool write_arctracker_module(const module_t *, FILE *);
static bool write_format_chunk(FILE *);
static bool write_meta_chunk(const module_t *, FILE *);
static bool write_track_chunks(const module_t *, FILE *);
static bool write_track_chunk(const module_t *, int track, FILE *);
static bool write_sequence_chunk(const module_t *, FILE *);
static bool write_pattern_chunks(const module_t *, FILE *);
static bool write_pattern_chunk(const module_t *, int, FILE *);
static event_t read_pattern_event(const uint8_t *);
static bool pattern_is_empty(pattern_t, const module_t*);
static bool event_is_empty(event_t);
static bool effect_is_empty(effect_t);
static bool write_pattern_events(pattern_t, const module_t *, FILE *fp);
static bool write_event(event_t, FILE *);
static float read_gain(uint32_t);
static uint32_t write_gain(float);
static bool write_fourcc(FILE *, const char *);
static bool write_cc(FILE *, const char *, size_t);
static bool write_u32_le(FILE *, uint32_t);

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
    return mapped_file.size >= 12 && chunk_is(FORMAT_CHUNK_ID, mapped_file.addr);
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
    const uint32_t format_version = read_u32_le(mapped_file.addr + 8);
    if (format_version > FORMAT_VERSION)
    {
        error("File format is a later version than that supported by this program");
        return NULL;
    }
    const size_t file_end = (size_t) mapped_file.addr + mapped_file.size;
    uint8_t *chunk_addr = next_chunk_address(mapped_file.addr);
    if (!chunk_is(META_CHUNK_ID, chunk_addr))
    {
        error("File corrupt: module metadata not found");
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
            error("File corrupt: reported chunk size extends beyond EOF");
            goto read_arctracker_module_failed;
        }
        if (chunk_is(SEQUENCE_CHUNK_ID, chunk_addr))
            ok = read_sequence_chunk(data, data_size, module);
        if (chunk_is(TRACK_CHUNK_ID, chunk_addr))
            ok = read_track_chunk(data, data_size, module);
        if (chunk_is(PATTERN_CHUNK_ID, chunk_addr))
            ok = read_pattern_chunk(data, data_size, module);
        if (!ok)
            goto read_arctracker_module_failed;
        chunk_addr = next_chunk_address(chunk_addr);
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
        error("File corrupt: invalid meta chunk length");
        return NULL;
    }
    const uint32_t num_tracks = read_u32_le(meta_data);
    if (num_tracks == 0 || num_tracks > MAX_TRACKS)
    {
        error("File corrupt: invalid number of tracks");
        return NULL;
    }
    const uint32_t sequence_length = read_u32_le(meta_data + 4);
    if (sequence_length == 0)
    {
        error("File corrupt: invalid sequence length");
        return NULL;
    }
    module_t *module = module_create(num_tracks, sequence_length, 0, 0);
    if (module == NULL)
    {
        return NULL;
    }
    char module_name[MODULE_NAME_LEN];
    memcpy(module_name, meta_data + 8, MODULE_NAME_LEN);
    module_name[MODULE_NAME_LEN - 1] = '\0';
    module_set_name(module, module_name);
    char author[AUTHOR_NAME_LEN];
    memcpy(author, meta_data + 8 + MODULE_NAME_LEN, AUTHOR_NAME_LEN);
    author[AUTHOR_NAME_LEN - 1] = '\0';
    module_set_author(module, author);
    const uint32_t master_gain = read_u32_le(meta_data + 8 + MODULE_NAME_LEN + AUTHOR_NAME_LEN);
    if (master_gain > MASTER_GAIN_MAX)
    {
        error("File corrupt: invalid master gain");
        goto read_module_metadata_failed;
    }
    module->master_gain = read_gain(master_gain);
    const uint32_t initial_speed = read_u32_le(meta_data + 12 + MODULE_NAME_LEN + AUTHOR_NAME_LEN);
    if (initial_speed == 0 || initial_speed > 255)
    {
        error("File corrupt: invalid initial speed");
        goto read_module_metadata_failed;
    }
    module->initial_speed = initial_speed;
    return module;

read_module_metadata_failed:
    module_destroy(module);
    return NULL;
}

static uint32_t read_u32_le(const uint8_t *addr)
{
    return addr[0] | addr[1] << 8 | addr[2] << 16 | addr[3] << 24;
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
    if (data_size != (size_t) module->tune_length * 4)
    {
        error("File corrupt: sequence data size does not match tune length");
        return false;
    }
    // TODO: Change module->sequence to be array of uint32_t so I don't have to do this.
    int *sequence = allocate_array(MODULE, module->tune_length, sizeof(int));
    if (sequence == NULL)
        return false;
    for (uint32_t i = 0; i < (uint32_t) module->tune_length; i++)
        sequence[i] = (int) read_u32_le(data + i * 4);
    const bool ok = module_set_sequence(module, sequence, module->tune_length);
    if (!ok) error("Failed to set sequence");
    deallocate(MODULE, sequence);
    return ok;
}

static bool read_track_chunk(const uint8_t *data, const size_t data_size, const module_t *module)
{
    if (data_size < 12)
    {
        error("File corrupt: invalid track data size");
        return false;
    }
    const uint32_t track = read_u32_le(data);
    if (track >= (uint32_t) module->num_tracks)
    {
        error("File corrupt: invalid track id");
        return false;
    }
    const uint32_t initial_panning = read_u32_le(data + 4);
    if (initial_panning > 255)
    {
        error("File corrupt: invalid track panning");
        return false;
    }
    const uint32_t effects_displayed = read_u32_le(data + 8);
    if (effects_displayed > 4)
    {
        error("File corrupt: invalid number of effects displayed");
        return false;
    }
    module->initial_panning[track] = initial_panning;
    // TODO: Add number of effects displayed to module.
    return true;
}

static bool read_pattern_chunk(const uint8_t *data, const size_t data_size, module_t *module)
{
    const uint32_t pattern_no = read_u32_le(data);
    if (pattern_no > UINT16_MAX)
    {
        error("File corrupt: invalid pattern number");
        return false;
    }
    const uint32_t num_lines = read_u32_le(data + 4);
    if (num_lines == 0 || num_lines > 1000)
    {
        error("File corrupt: invalid pattern length");
        return false;
    }
    if (data_size != 8 + module->num_tracks * num_lines * EVENT_SIZE)
    {
        error("File corrupt: pattern data size does not match expected");
        return false;
    }
    if (!module_create_pattern(module, pattern_no, num_lines))
    {
        error("Failed to create pattern");
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
    const uint32_t note = read_u32_le(data);
    const uint32_t instrument_no = read_u32_le(data + 4);
    if (!NOTE_OUT_OF_RANGE(note))
        event.note = note;
    if (instrument_no < 256)
        event.instrument_no = instrument_no;
    for (uint32_t effect = 0; effect < 4; effect++)
    {
        const uint32_t effect_command = read_u32_le(data + 8 + effect * 8);
        const uint32_t effect_data = read_u32_le(data + 12 + effect * 8);
        event.effects[effect] = (effect_t) {
            .command = (char) effect_command,
            .data = effect_data & 0xff,
        };
    }
    return event;
}

/*****************************************************************************
 * Code for writing a module file.                                           *
 *****************************************************************************/

static bool write_arctracker_module(const module_t *module, FILE *fp)
{
    if (!write_format_chunk(fp)) return false;
    if (!write_meta_chunk(module, fp)) return false;
    if (!write_track_chunks(module, fp)) return false;
    if (!write_sequence_chunk(module, fp)) return false;
    if (!write_pattern_chunks(module, fp)) return false;
    return true;
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
    char module_name[MODULE_NAME_LEN], author[AUTHOR_NAME_LEN];
    snprintf(module_name, sizeof module_name, "%s", module->name);
    snprintf(author, sizeof author, "%s", module->author);
    if (!write_fourcc(fp, META_CHUNK_ID)) return false;
    if (!write_u32_le(fp, META_CHUNK_LEN)) return false;
    if (!write_u32_le(fp, module->num_tracks)) return false;
    if (!write_u32_le(fp, module->tune_length)) return false;   // TODO: Rename sequence_length.
    if (!write_cc(fp, module_name, sizeof module_name)) return false;
    if (!write_cc(fp, author, sizeof author)) return false;
    if (!write_u32_le(fp, write_gain(module->master_gain))) return false;
    if (!write_u32_le(fp, module->initial_speed)) return false; // TODO: Rename ticks_per_event.
    if (!write_u32_le(fp, 50)) return false;                    // TODO: Initial ticks per second not in module yet
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
    if (!write_fourcc(fp, TRACK_CHUNK_ID)) return false;
    if (!write_u32_le(fp, TRACK_CHUNK_LEN)) return false;
    if (!write_u32_le(fp, (uint32_t) track)) return false;
    if (!write_u32_le(fp, (uint32_t) module->initial_panning[track])) return false;
    if (!write_u32_le(fp, 1)) return false;                     // TODO: Number of effects displayed
    return true;
}

static bool write_sequence_chunk(const module_t *module, FILE *fp)
{
    if (!write_fourcc(fp, SEQUENCE_CHUNK_ID)) return false;
    if (!write_u32_le(fp, (uint32_t) module->tune_length * 4)) return false;
    for (int p = 0; p < module->tune_length; p++)
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
        return true;
    }
    if (!write_fourcc(fp, PATTERN_CHUNK_ID)) return false;
    if (!write_u32_le(fp, 8 + pattern.num_lines * module->num_tracks * EVENT_SIZE)) return false;
    if (!write_u32_le(fp, (uint32_t) pattern_no)) return false;
    if (!write_u32_le(fp, (uint32_t) pattern.num_lines)) return false;
    if (!write_pattern_events(pattern, module, fp)) return false;
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
    if (!write_u32_le(fp, (uint32_t) event.note)) return false;
    if (!write_u32_le(fp, (uint32_t) event.instrument_no)) return false;
    for (int effect_no = 0; effect_no < 4; effect_no++)
    {
        if (!write_u32_le(fp, (uint32_t) event.effects[effect_no].command)) return false;
        if (!write_u32_le(fp, event.effects[effect_no].data)) return false;
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
