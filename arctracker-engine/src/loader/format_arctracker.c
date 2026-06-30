#include "format_arctracker.h"
#include "io/error.h"
#include "memory/heap.h"

#define MODULE_NAME_LEN 68
#define AUTHOR_NAME_LEN 68
#define EVENT_SIZE 40

static const uint32_t FORMAT_VERSION = 1;
static const char *FORMAT_CHUNK_ID = "ARCT";
static const char *META_CHUNK_ID = "META";
static const char *TRACK_CHUNK_ID = "TRCK";
static const char *SEQUENCE_CHUNK_ID = "SEQU";
static const char *PATTERN_CHUNK_ID = "PATT";
static const uint32_t FORMAT_CHUNK_LEN = 4;
static const uint32_t META_CHUNK_LEN = MODULE_NAME_LEN + AUTHOR_NAME_LEN + 16;
static const uint32_t TRACK_CHUNK_LEN = 12;
static const uint32_t MASTER_GAIN_MAX = 65535;
static bool is_arctracker_module(mapped_file_t);
static module_t *read_arctracker_module(mapped_file_t);
static bool write_arctracker_module(const module_t *, FILE *);
static bool write_format_chunk(FILE *);
static bool write_meta_chunk(const module_t *, FILE *);
static bool write_track_chunks(const module_t *, FILE *);
static bool write_track_chunk(const module_t *, int track, FILE *);
static bool write_sequence_chunk(const module_t *, FILE *);
static bool write_pattern_chunks(const module_t *, FILE *);
static bool write_pattern_chunk(const module_t *, int, FILE *);
static bool pattern_is_empty(pattern_t, const module_t*);
static bool event_is_empty(event_t);
static bool effect_is_empty(effect_t);
static bool write_pattern_events(pattern_t, const module_t *, FILE *fp);
static bool write_event(event_t, FILE *);
static uint32_t convert_gain(float);
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

static bool is_arctracker_module(mapped_file_t mapped_file)
{
    // TODO: Everything!
    return false;
}

static module_t *read_arctracker_module(mapped_file_t mapped_file)
{
    // TODO: Everything!
    return NULL;
}

static bool write_arctracker_module(const module_t *module, FILE *fp)
{
    return write_format_chunk(fp)
           && write_meta_chunk(module, fp)
           && write_track_chunks(module, fp)
           && write_sequence_chunk(module, fp)
           && write_pattern_chunks(module, fp);
}

static bool write_format_chunk(FILE *fp)
{
    return write_fourcc(fp, FORMAT_CHUNK_ID)
           && write_u32_le(fp, FORMAT_CHUNK_LEN)
           && write_u32_le(fp, FORMAT_VERSION);
}

static bool write_meta_chunk(const module_t *module, FILE *fp)
{
    char module_name[MODULE_NAME_LEN], author[AUTHOR_NAME_LEN];
    snprintf(module_name, sizeof module_name, "%s", module->name);
    snprintf(author, sizeof author, "%s", module->author);
    return write_fourcc(fp, META_CHUNK_ID)
           && write_u32_le(fp, META_CHUNK_LEN)
           && write_cc(fp, module_name, sizeof module_name)
           && write_cc(fp, author, sizeof author)
           && write_u32_le(fp, convert_gain(module->master_gain))
           && write_u32_le(fp, module->num_tracks)
           && write_u32_le(fp, module->initial_speed) // Ticks per event.
           && write_u32_le(fp, 50); // TODO: Initial ticks per second not in module yet
}

static bool write_track_chunks(const module_t *module, FILE *fp)
{
    bool ok = true;
    for (int track = 0; track < module->num_tracks; track++)
        ok &= write_track_chunk(module, track, fp);
    return ok;
}

static bool write_track_chunk(const module_t *module, const int track, FILE *fp)
{
    return write_fourcc(fp, TRACK_CHUNK_ID)
           && write_u32_le(fp, TRACK_CHUNK_LEN)
           && write_u32_le(fp, (uint32_t) track)
           && write_u32_le(fp, (uint32_t) module->initial_panning[track])
           && write_u32_le(fp, 1); // TODO: Number of effects displayed
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
    if (pattern_is_empty(pattern, module)) return true;
    return write_fourcc(fp, PATTERN_CHUNK_ID)
           && write_u32_le(fp, 8 + pattern.num_lines * module->num_tracks * EVENT_SIZE)
           && write_u32_le(fp, (uint32_t) pattern_no)
           && write_u32_le(fp, (uint32_t) pattern.num_lines)
           && write_pattern_events(pattern, module, fp);
}

static bool pattern_is_empty(const pattern_t pattern, const module_t *module)
{
    const uint32_t track_capacity = module->track_capacity;
    const uint32_t num_tracks = module->num_tracks;
    for (int line = 0; line < pattern.num_lines; line++)
        for (uint32_t track = 0; track < num_tracks; track++)
            if (!event_is_empty(pattern.events[track + line * track_capacity]))
                return false;
    return true;
}

static bool event_is_empty(const event_t event)
{
    return event.note == 0
           && event.instrument_no == 0
           && effect_is_empty(event.effects[0])
           && effect_is_empty(event.effects[1])
           && effect_is_empty(event.effects[2])
           && effect_is_empty(event.effects[3]);
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

static uint32_t convert_gain(const float gain)
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
