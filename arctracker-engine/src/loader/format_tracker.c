#include "format_tracker.h"
#include <string.h>
#include "io/error.h"
#include "vidc/vidc.h"
#include "memory/heap.h"
#include "memory/bits.h"
#include "player/period.h"

uint8_t *CHUNK_NOT_FOUND = NULL;
sample_t *get_sample_info_failed = NULL;

static const int CHUNK_ID_LENGTH = 4;
static const int CHUNK_HEADER_LENGTH = 8;
static const int MAX_LEN_TUNENAME_TRK = 32;
static const int MAX_LEN_AUTHOR_TRK = 32;
static const int MAX_LEN_SAMPLENAME_TRK = 20;
static const int NUM_SAMPLES = 36;

static const char *TRACKER_FORMAT = "TRACKER";

static const char *MUSX_CHUNK = "MUSX";
static const char *MVOX_CHUNK = "MVOX";
static const char *STER_CHUNK = "STER";
static const char *MNAM_CHUNK = "MNAM";
static const char *ANAM_CHUNK = "ANAM";
static const char *MLEN_CHUNK = "MLEN";
static const char *PNUM_CHUNK = "PNUM";
static const char *PLEN_CHUNK = "PLEN";
static const char *SEQU_CHUNK = "SEQU";
static const char *PATT_CHUNK = "PATT";
static const char *SAMP_CHUNK = "SAMP";
static const char *SNAM_CHUNK = "SNAM";
static const char *SVOL_CHUNK = "SVOL";
static const char *SLEN_CHUNK = "SLEN";
static const char *ROFS_CHUNK = "ROFS";
static const char *RLEN_CHUNK = "RLEN";
static const char *SDAT_CHUNK = "SDAT";

static const uint8_t ARPEGGIO_CMD_DSKT = 0;      // 0
static const uint8_t PORTAMENTO_UP_CMD_DSKT = 1;        // 1
static const uint8_t PORTAMENTO_DOWN_CMD_DSKT = 2;      // 2
static const uint8_t TONE_PORTAMENTO_CMD_DSKT = 3;      // 3
static const uint8_t VIBRATO_CMD_DSKT = 4;       // 4
static const uint8_t BREAK_COMMAND = 11;        // B
static const uint8_t SET_STEREO_CMD_DSKT = 14;       // E
static const uint8_t VOLSLIDEUP_COMMAND = 16;   // G
static const uint8_t VOLSLIDEDOWN_COMMAND = 17; // H
static const uint8_t JUMP_CMD_DSKT = 19;         // J
static const uint8_t SET_SPEED_CMD_DSKT = 28;        // S
static const uint8_t SET_VOLUME_CMD_DSKT = 31;       // V

static const int PANNING[] = {1, 43, 86, 128, 170, 213, 255};

static bool is_tracker_format(mapped_file_t);
static module_t *read_tracker_module(mapped_file_t);
static uint8_t *search_tff(uint8_t *, long, const char *);
static bool decode_patterns(uint8_t *, long, module_t *, const int *);
static size_t decode_tracker_event(const uint8_t *, event_t *);
static effect_t effect(uint8_t code, uint8_t);
static int get_samples(void *, long, sample_t *, instrument_t *, int);
static bool get_sample_info(void *, long, sample_t *, instrument_t *, int);
static float calculate_sample_rate(int, int);
static float assumed_sample_period(int);
static void copy_int_array(uint8_t *, int *, int);

format_t tracker_format(void)
{
    format_t format_reader = {
            .is_this_format = is_tracker_format,
            .read_module = read_tracker_module,
            .write_module = NULL,
    };
    return format_reader;
}

static bool is_tracker_format(mapped_file_t file)
{
    long array_end = (long) file.addr + file.size;
    return (search_tff(file.addr, array_end, MUSX_CHUNK) != CHUNK_NOT_FOUND);
}

static module_t *read_tracker_module(mapped_file_t file)
{
    module_t *module = NULL;
    int *pattern_lengths = NULL;
    uint8_t *chunk_address;
    long array_end = (long) file.addr + file.size;
    if ((chunk_address = search_tff(file.addr, array_end, MVOX_CHUNK)) == CHUNK_NOT_FOUND)
    {
        error("Modfile corrupt - MVOX chunk not found");
        goto fail;
    }
    uint32_t num_tracks = *(uint32_t *) (chunk_address + 8);
    if ((chunk_address = search_tff(file.addr, array_end, MLEN_CHUNK)) == CHUNK_NOT_FOUND)
    {
        error("Modfile corrupt - MLEN chunk not found");
        goto fail;
    }
    uint32_t sequence_len = *(uint32_t *) (chunk_address + 8);
    if ((chunk_address = search_tff(file.addr, array_end, PNUM_CHUNK)) == CHUNK_NOT_FOUND)
    {
        error("Modfile corrupt - PNUM chunk not found");
        goto fail;
    }
    uint32_t num_patterns = *(uint32_t *) (chunk_address + 8);
    module = module_create(num_tracks, sequence_len, num_patterns, 36);
    if (module == NULL)
    {
        goto fail;
    }
    module->format = TRACKER_FORMAT;
    module->initial_ticks_per_event = 6;
    module->master_gain = 0.25f;
    module->default_pattern_length = 64;
    module->interpolation_type = NONE;
    module->volume_mapping_type = VOLUME_ARCHIMEDES;
    if ((chunk_address = search_tff(file.addr, array_end, STER_CHUNK)) == CHUNK_NOT_FOUND)
    {
        error("Modfile corrupt - STER chunk not found");
        goto fail;
    }
    const uint8_t *initial_stereo = chunk_address + 8;
    for (int track = 0; track < module->num_tracks; track++)
    {
        const uint8_t track_panning = initial_stereo[track];
        if (track_panning == 0 || track_panning > 7)
        {
            module->tracks[track].panning = 128;
            continue;
        }
        module->tracks[track].panning = PANNING[track_panning - 1];
        module->tracks[track].muted = false;
        module->tracks[track].effects_displayed = 1;
    }
    if ((chunk_address = search_tff(file.addr, array_end, MNAM_CHUNK)) == CHUNK_NOT_FOUND)
    {
        error("Modfile corrupt - MNAM chunk not found");
        goto fail;
    }
    strncpy(module->name, (char *) chunk_address + 8, MAX_LEN_TUNENAME_TRK);
    if ((chunk_address = search_tff(file.addr, array_end, ANAM_CHUNK)) == CHUNK_NOT_FOUND)
    {
        error("Modfile corrupt - ANAM chunk not found");
        goto fail;
    }
    strncpy(module->author, (char *) chunk_address + 8, MAX_LEN_AUTHOR_TRK);
    if ((chunk_address = search_tff(file.addr, array_end, PLEN_CHUNK)) == CHUNK_NOT_FOUND)
    {
        error("Modfile corrupt - PLEN chunk not found");
        goto fail;
    }
    pattern_lengths = allocate_array(MODULE, NUM_PATTERNS, sizeof(int));
    if (pattern_lengths == NULL)
    {
        goto fail;
    }
    copy_int_array(chunk_address + 8, pattern_lengths, NUM_PATTERNS);
    if ((chunk_address = search_tff(file.addr, array_end, SEQU_CHUNK)) == CHUNK_NOT_FOUND)
    {
        error("Modfile corrupt - SEQU chunk not found");
        goto fail;
    }
    copy_int_array(chunk_address + 8, module->sequence, module->sequence_length);
    if (!decode_patterns(file.addr, array_end, module, pattern_lengths))
        goto fail;
    module->sample_slots = get_samples(file.addr, array_end, module->samples, module->instruments, module->num_tracks);
    if (module->sample_slots == 0)
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

static uint8_t *search_tff(uint8_t *array_start, const long array_end, const char *to_find)
{
    while ((long) array_start <= (array_end - CHUNK_ID_LENGTH))
    {
        if (memcmp(to_find, array_start, CHUNK_ID_LENGTH) == 0)
            return array_start;
        array_start++;
    }
    return CHUNK_NOT_FOUND;
}

static command_t tracker_command(const int code, const uint8_t data)
{
    if (code == SET_VOLUME_CMD_DSKT) return SET_VOLUME;
    if (code == SET_SPEED_CMD_DSKT) return SET_TEMPO;
    if (code == SET_STEREO_CMD_DSKT) return SET_PANNING;
    if (code == VOLSLIDEUP_COMMAND) return VOLUME_SLIDE;
    if (code == VOLSLIDEDOWN_COMMAND) return VOLUME_SLIDE;
    if (code == PORTAMENTO_UP_CMD_DSKT) return PITCH_SLIDE_UP;
    if (code == PORTAMENTO_DOWN_CMD_DSKT) return PITCH_SLIDE_DOWN;
    if (code == TONE_PORTAMENTO_CMD_DSKT) return PORTAMENTO;
    if (code == VIBRATO_CMD_DSKT) return VIBRATO;
    if (code == BREAK_COMMAND) return PATTERN_BREAK;
    if (code == JUMP_CMD_DSKT) return SEQUENCE_JUMP;
    if (code == ARPEGGIO_CMD_DSKT && data > 0) return ARPEGGIO;
    return NO_EFFECT;
}

static bool decode_patterns(uint8_t *array_start, const long array_end, module_t *module, const int *pattern_lengths)
{
    int patterns_found = 0;
    uint8_t *chunk_address = search_tff(array_start, array_end, PATT_CHUNK);
    while (chunk_address != CHUNK_NOT_FOUND)
    {
        const int pno = patterns_found++;
        const int pattern_length = pattern_lengths[pno];
        if (!module_create_pattern(module, pno, pattern_length))
        {
            return false;
        }
        const uint8_t *raw_pattern_data = chunk_address + CHUNK_HEADER_LENGTH;
        for (int line = 0; line < pattern_length; line++)
        {
            for (uint32_t track = 0; track < module->track_capacity; track++)
            {
                const uint32_t event_index = (line * module->track_capacity) + track;
                event_t *event = module->patterns[pno].events + event_index;
                if (track < (uint32_t) module->num_tracks)
                    raw_pattern_data += decode_tracker_event(raw_pattern_data, event);
                else
                    *event = (event_t) {0};
            }
        }
        chunk_address = search_tff(chunk_address + CHUNK_ID_LENGTH, array_end, PATT_CHUNK);
    }
    if (patterns_found == 0)
    {
        error("Modfile corrupt - no patterns in module");
        return false;
    }
    return true;
}

static size_t decode_tracker_event(const uint8_t *event_p, event_t *decoded)
{
    const uint32_t *raw = (uint32_t *) event_p;
    decoded->instrument_no = (int) mask_8_shift_right(*raw, 16);
    const int note = (int) mask_8_shift_right(*raw, 24);
    decoded->note = note == 0 ? 0 : note + 12;
    decoded->effects[0] = effect(mask_8_shift_right(*raw, 8), mask_8_shift_right(*raw, 0));
    for (int i = 1; i <= 3; i++)
    {
        decoded->effects[i] = effect(0, 0);
    }
    return EVENT_SIZE_SINGLE_EFFECT;
}

static effect_t effect(const uint8_t code, const uint8_t data)
{
    const command_t command = tracker_command(code, data);
    uint8_t effect_data = data;
    if (command == SET_PANNING)
    {
        if (data == 0 || data > 7) effect_data = 128; // Pathological value, centre it.
        else effect_data = PANNING[data - 1];
    }
    if (command == PATTERN_BREAK)
    {
        // Tracker always breaks to the next pattern at line 0, but Arctracker interprets the data as the line to begin
        // the next pattern at, so zero it here.
        effect_data = 0;
    }
    if (command == VOLUME_SLIDE)
    {
        if (code == VOLSLIDEUP_COMMAND) effect_data = 0x80 | data;
        else effect_data = data;
    }
    return (effect_t) {
        .data = effect_data,
        .command = command,
    };
}

static int get_samples(void *array_start, long array_end, sample_t *samples, instrument_t *instrument_slots, const int num_tracks)
{
    int sample_index = 0;
    int slot = 0;
    char error_message[256];
    uint8_t *chunk_address = search_tff(array_start, array_end, SAMP_CHUNK);
    while (chunk_address != CHUNK_NOT_FOUND && sample_index < NUM_SAMPLES)
    {
        sample_t *sample = &samples[sample_index];
        instrument_t *instrument = &instrument_slots[slot];
        if (!get_sample_info(chunk_address, array_end, sample, instrument, num_tracks))
        {
            snprintf(error_message, 256, "Modfile corrupt - sample %d invalid", sample_index);
            error(error_message);
            return 0;
        }
        if (sample->sample_length > 0)
        {
            instrument->assigned = true;
            instrument->sample_index = sample_index;
        }
        else
        {
            instrument->assigned = false;
        }
        slot++;
        sample_index++;
        chunk_address = search_tff(chunk_address + CHUNK_ID_LENGTH, array_end, SAMP_CHUNK);
    }
    return sample_index;
}

static bool get_sample_info(void *array_start, const long array_end, sample_t *sample, instrument_t *instrument, const int num_tracks)
{
    uint8_t *chunk_address;

    // Sample name.
    if ((chunk_address = search_tff(array_start, array_end, SNAM_CHUNK)) == CHUNK_NOT_FOUND)
    {
        fprintf(stderr, "Failed to find SNAM\n");
        goto get_sample_info_failed;
    }
    strncpy(instrument->name, (char *) chunk_address + CHUNK_HEADER_LENGTH, MAX_LEN_SAMPLENAME_TRK);

    // Sample volume.
    if ((chunk_address = search_tff(array_start, array_end, SVOL_CHUNK)) == CHUNK_NOT_FOUND)
    {
        fprintf(stderr, "Failed to find SVOL\n");
        goto get_sample_info_failed;
    }
    instrument->default_volume = *(int32_t *) (chunk_address + CHUNK_HEADER_LENGTH);

    // Sample length.
    if ((chunk_address = search_tff(array_start, array_end, SLEN_CHUNK)) == CHUNK_NOT_FOUND)
    {
        fprintf(stderr, "Failed to find SLEN\n");
        goto get_sample_info_failed;
    }
    sample->sample_length = *(int32_t *) (chunk_address + CHUNK_HEADER_LENGTH);

    // Repeat offset.
    if ((chunk_address = search_tff(array_start, array_end, ROFS_CHUNK)) == CHUNK_NOT_FOUND)
    {
        fprintf(stderr, "Failed to find ROFS\n");
        goto get_sample_info_failed;
    }
    instrument->repeat_offset = *(int32_t *) (chunk_address + CHUNK_HEADER_LENGTH);

    // Repeat length.
    if ((chunk_address = search_tff(array_start, array_end, RLEN_CHUNK)) == CHUNK_NOT_FOUND)
    {
        fprintf(stderr, "Failed to find RLEN\n");
        goto get_sample_info_failed;
    }
    const int repeat_length = *(int32_t *) (chunk_address + CHUNK_HEADER_LENGTH);
    if ((repeat_length == 2 && instrument->repeat_offset != 0)
    || repeat_length + instrument->repeat_offset > sample->sample_length)
        instrument->repeat_length = sample->sample_length - instrument->repeat_offset;
    else
        instrument->repeat_length = repeat_length;

    // Sample data.
    if ((chunk_address = search_tff(array_start, array_end, SDAT_CHUNK)) == CHUNK_NOT_FOUND)
    {
        sample->sample_length = 0;
    }
    const uint8_t *sample_data_mu_law = chunk_address + CHUNK_HEADER_LENGTH;
    if (sample->sample_length == 0)
    {
        sample->sample_data = NULL;
        instrument->repeats = false;
    }
    else
    {
        float *sample_data = allocate_array(MODULE, sample->sample_length + 2, sizeof(float));
        if (sample_data == NULL) goto get_sample_info_failed;
        for (int i = 0; i < sample->sample_length; i++)
        {
            sample_data[i] = vidc_to_linear(sample_data_mu_law[i]);
        }
        instrument->repeats = instrument->repeat_offset != 0 || instrument->repeat_length != 2;
        if (instrument->repeats) {
            sample_data[sample->sample_length] = sample_data[instrument->repeat_offset];
            sample_data[sample->sample_length + 1] = sample_data[instrument->repeat_offset + 1];
        }
        sample->sample_data = sample_data;
        //
        // The sample base note is chosen as C2 for convenience, but it could be any playable note.
        //
        sample->base_note = 24;
        sample->sample_rate = calculate_sample_rate(num_tracks, sample->base_note);
        sample->finetune = 0;
    }
    instrument->transpose = 0;

    return true;

get_sample_info_failed:
    return false;
}

static float calculate_sample_rate(const int channels, const int base_note)
{
    // Internally Arctracker uses this to calculate the phase increment per period:
    //   given: base period = period_for_note(sample base note, fine tuning);
    //   phase increment per period = recorded sample rate * base period / output sample rate;
    // We will calculate a recorded sample rate to plug into that formula which gives the correct playback pitch.
    const float sample_period_assumed = assumed_sample_period(channels);
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
    // I don't know why the Tracker play routine assumes these periods in its phase increment calculation.
    // Fortunately, we don't need to know the reason for calculating the correct playback pitch. It merely is.
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

static void copy_int_array(uint8_t *dest, int *source, int num_elements)
{
    for (int i = 0; i < num_elements; i++)
        source[i] = dest[i];
}
