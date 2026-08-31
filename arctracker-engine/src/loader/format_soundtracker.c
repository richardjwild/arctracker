#include "format_soundtracker.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "memory/heap.h"
#include "pcm/mu_law.h"

/*
 * Documentation lifted from Andrew Scott's website: https://www.aes.id.au/modformat.html
 * --------------------------------------------------------------------------------------
 *
 * Module Format:
 * # Bytes   Description
 * -------   -----------
 * 20        The module's title, padded with null (\0) bytes. Original
 *           Protracker wrote letters only in uppercase.
 *
 * (Data repeated for each sample 1-15 or 1-31)
 * 22        Sample's name, padded with null bytes. If a name begins with a
 *           '#', it is assumed not to be an instrument name, and is
 *           probably a message.
 * 2         Sample length in words (1 word = 2 bytes). The first word of
 *           the sample is overwritten by the tracker, so a length of 1
 *           still means an empty sample. See below for sample format.
 * 1         Lowest four bits represent a signed nibble (-8..7) which is
 *           the finetune value for the sample. Each finetune step changes
 *           the note 1/8th of a semitone. Implemented by switching to a
 *           different table of period-values for each finetune value.
 * 1         Volume of sample. Legal values are 0..64. Volume is the linear
 *           difference between sound intensities. 64 is full volume, and
 *           the change in decibels can be calculated with 20*log10(Vol/64)
 * 2         Start of sample repeat offset in words. Once the sample has
 *           been played all of the way through, it will loop if the repeat
 *           length is greater than one. It repeats by jumping to this
 *           position in the sample and playing for the repeat length, then
 *           jumping back to this position, and playing for the repeat
 *           length, etc.
 * 2         Length of sample repeat in words. Only loop if greater than 1.
 * (End of this sample's data.. each sample uses the same format and they
 *  are stored sequentially)
 * N.B. All 2 byte lengths are stored with the Hi-byte first, as is usual
 *      on the Amiga (big-endian format).
 *
 * 1         Number of song positions (ie. number of patterns played
 *           throughout the song). Legal values are 1..128.
 * 1         Historically set to 127, but can be safely ignored.
 *           Noisetracker uses this byte to indicate restart position -
 *           this has been made redundant by the 'Position Jump' effect.
 * 128       Pattern table: patterns to play in each song position (0..127)
 *           Each byte has a legal value of 0..63 (note the Protracker
 *           exception below). The highest value in this table is the
 *           highest pattern stored, no patterns above this value are
 *           stored.
 * (4)       The four letters "M.K." These are the initials of
 *           Unknown/D.O.C. who changed the format so it could handle 31
 *           samples (sorry.. they were not inserted by Mahoney & Kaktus).
 *           Startrekker puts "FLT4" or "FLT8" here to indicate the # of
 *           channels. If there are more than 64 patterns, Protracker will
 *           put "M!K!" here. You might also find: "4CHN", "6CHN" or "8CHN"
 *           which indicates 4, 6 or 8 channels respectively. If no letters
 *           are here, then this is the start of the pattern data, and only
 *           15 samples were present.
 *
 * (Data repeated for each pattern:)
 * 1024      Pattern data for each pattern (starting at 0).
 * (Each pattern has same format and is stored in numerical order.
 *  See below for pattern format)
 *
 * (Data repeated for each sample:)
 * xxxxxx    The maximum size of a sample is 65535 words. Each sample is
 *           stored as a collection of bytes (length of a sample was given
 *           previously in the module). Each byte is a signed value (-128
 *           ..127) which is the channel data. When a sample is played at a
 *           pitch of C2 (see below for pitches), about 8287 bytes of
 *           sample data are sent to the channel per second. Multiply the
 *           rate by the twelfth root of 2 (=1.0595) for each semitone
 *           increase in pitch eg. moving the pitch up 1 octave doubles the
 *           rate. The data is stored in the order it is played (eg. first
 *           byte is first byte played). The first word of the sample data
 *           is used to hold repeat information, and will overwrite any
 *           sample data that is there (but it is probably safer to set it
 *           to 0).
 *           The rate given above (8287) conveys an inaccurate picture of
 *           the module-format - in reality it is different for different
 *           Amigas. As the routines for playing were written to run off
 *           certain interrupts, for different Amiga computers the rate to
 *           send data to the channel will be different. For PAL machines
 *           the clock rate is 7093789.2 Hz and for NTSC machines it is
 *           7159090.5 Hz. When the clock rate is divided by twice the
 *           period number for the pitch it will give the rate to send the
 *           data to the channel, eg. for a PAL machine sending a note at
 *           C2 (period 428), the rate is 7093789.2/856 ~= 8287.1369
 * (Each sample is stored sequentially)
 *
 * Pattern Format:
 * Each pattern is divided into 64 divisions. By allocating different
 * tempos for each pattern and spacing the notes across different amounts
 * of divisions, different bar sizes can be accommodated.
 *
 * Each division contains the data for each channel (1..4) stored after
 * each other. Channels 1 and 4 are on the left, and channels 2 and 3 are
 * on the right. In the case of more channels: channels 5 and 8 are on the
 * left, and channels 6 and 7 are on the right, etc. Each channel's data in
 * the division has an identical format which consists of 2 words (4
 * bytes). Divisions are numbered 0..63. Each division may be divided into
 * a number of ticks (see 'set speed' effect below).
 *
 * Channel Data:
 *                   (the four bytes of channel data in a pattern division)
 * 7654-3210 7654-3210 7654-3210 7654-3210
 * wwww xxxxxxxxxxxxxx yyyy zzzzzzzzzzzzzz
 *
 *     wwwwyyyy (8 bits) is the sample for this channel/division
 * xxxxxxxxxxxx (12 bits) is the sample's period (or effect parameter)
 * zzzzzzzzzzzz (12 bits) is the effect for this channel/division
 *
 * If there is to be no new sample to be played at this division on this
 * channel, then the old sample on this channel will continue, or at least
 * be "remembered" for any effects. If the sample is 0, then the previous
 * sample on that channel is used. Only one sample may play on a channel at
 * a time, so playing a new sample will cancel an old one - even if there
 * has been no data supplied for the new sample. Though, if you are using a
 * "silence" sample (ie. no data, only used to turn off other samples) it
 * is polite to set its default volume to 0.
 *
 * To determine what pitch the sample is to be played on, look up the
 * period in a table, such as the one below (for finetune 0). If the period
 * is 0, then the previous period on that channel is used. Unfortunately,
 * some modules do not use these exact values. It is best to do a binary-
 * search (unless you use the period as the offset of an array of notes..
 * expensive), especially if you plan to use periods outside the "standard"
 * range. Periods are the internal representation of the pitch, so effects
 * that alter pitch (eg. sliding) alter the period value (see "effects"
 * below).
 *
 *           C    C#   D    D#   E    F    F#   G    G#   A    A#   B
 * Octave 1: 856, 808, 762, 720, 678, 640, 604, 570, 538, 508, 480, 453
 * Octave 2: 428, 404, 381, 360, 339, 320, 302, 285, 269, 254, 240, 226
 * Octave 3: 214, 202, 190, 180, 170, 160, 151, 143, 135, 127, 120, 113
 *
 * Octave 0:1712,1616,1525,1440,1357,1281,1209,1141,1077,1017, 961, 907
 * Octave 4: 107, 101,  95,  90,  85,  80,  76,  71,  67,  64,  60,  57
 *
 * Octaves 0 and 4 are NOT standard, so don't rely on every tracker being
 * able to play them, or even not crashing if being given them - it's just
 * nice that if you can code it, to allow them to be read.
 *
 * Effects:
 * Effects are written as groups of 4 bits, eg. 1871 = 7 * 256 + 4 * 16 +
 * 15 = [7][4][15]. The high nibble (4 bits) usually determines the effect,
 * but if it is [14], then the second nibble is used as well.
 *
 * [0]: Arpeggio
 *      Where [0][x][y] means "play note, note+x semitones, note+y
 *      semitones, then return to original note". The fluctuations are
 *      carried out evenly spaced in one pattern division. They are usually
 *      used to simulate chords, but this doesn't work too well. They are
 *      also used to produce heavy vibrato. A major chord is when x=4, y=7.
 *      A minor chord is when x=3, y=7.
 *
 * [1]: Slide up
 *      Where [1][x][y] means "smoothly decrease the period of current
 *      sample by x*16+y after each tick in the division". The
 *      ticks/division are set with the 'set speed' effect (see below). If
 *      the period of the note being played is z, then the final period
 *      will be z - (x*16 + y)*(ticks - 1). As the slide rate depends on
 *      the speed, changing the speed will change the slide. You cannot
 *      slide beyond the note B3 (period 113).
 *
 * [2]: Slide down
 *      Where [2][x][y] means "smoothly increase the period of current
 *      sample by x*16+y after each tick in the division". Similar to [1],
 *      but lowers the pitch. You cannot slide beyond the note C1 (period
 *      856).
 *
 * [3]: Slide to note
 *      Where [3][x][y] means "smoothly change the period of current sample
 *      by x*16+y after each tick in the division, never sliding beyond
 *      current period". Any note in this channel's division is not played,
 *      but changes the "remembered" note - it can be thought of as a
 *      parameter to this effect. Sliding to a note is similar to effects
 *      [1] and [2], but the slide will not go beyond the given period, and
 *      the direction is implied by that period. If x and y are both 0,
 *      then the old slide will continue.
 *
 * [4]: Vibrato
 *      Where [4][x][y] means "oscillate the sample pitch using a
 *      particular waveform with amplitude y/16 semitones, such that (x *
 *      ticks)/64 cycles occur in the division". The waveform is set using
 *      effect [14][4]. By placing vibrato effects on consecutive
 *      divisions, the vibrato effect can be maintained. If either x or y
 *      are 0, then the old vibrato values will be used.
 *
 * [5]: Continue 'Slide to note', but also do Volume slide
 *      Where [5][x][y] means "either slide the volume up x*(ticks - 1) or
 *      slide the volume down y*(ticks - 1), at the same time as continuing
 *      the last 'Slide to note'". It is illegal for both x and y to be
 *      non-zero. You cannot slide outside the volume range 0..64. The
 *      period-length in this channel's division is a parameter to this
 *      effect, and hence is not played.
 *
 * [6]: Continue 'Vibrato', but also do Volume slide
 *      Where [6][x][y] means "either slide the volume up x*(ticks - 1) or
 *      slide the volume down y*(ticks - 1), at the same time as continuing
 *      the last 'Vibrato'". It is illegal for both x and y to be non-zero.
 *      You cannot slide outside the volume range 0..64.
 *
 * [7]: Tremolo
 *      Where [7][x][y] means "oscillate the sample volume using a
 *      particular waveform with amplitude y*(ticks - 1), such that (x *
 *      ticks)/64 cycles occur in the division". If either x or y are 0,
 *      then the old tremolo values will be used. The waveform is set using
 *      effect [14][7]. Similar to [4].
 *
 * [8]: (Set panning position)
 *      This command is unused by the vast majority of trackers, but one
 *      tracker for the PC (called DMP) uses this for setting the panning
 *      state of the channel. As this is very useful, I am documenting it
 *      here. The effect [8][x][y] means "set channel to panning position
 *      x*16 + y". Position 0 is left, 64 is centre, 128 is right.
 *      Interestingly, position 164 is defined as "surround".
 *
 * [9]: Set sample offset
 *      Where [9][x][y] means "play the sample from offset x*4096 + y*256".
 *      The offset is measured in words. If no sample is given, yet one is
 *      still playing on this channel, it should be retriggered to the new
 *      offset using the current volume.
 *
 * [10]: Volume slide
 *      Where [10][x][y] means "either slide the volume up x*(ticks - 1) or
 *      slide the volume down y*(ticks - 1)". If both x and y are non-zero,
 *      then the y value is ignored (assumed to be 0). You cannot slide
 *      outside the volume range 0..64.
 *
 * [11]: Position Jump
 *      Where [11][x][y] means "stop the pattern after this division, and
 *      continue the song at song-position x*16+y". This shifts the
 *      'pattern-cursor' in the pattern table (see above). Legal values for
 *      x*16+y are from 0 to 127.
 *
 * [12]: Set volume
 *      Where [12][x][y] means "set current sample's volume to x*16+y".
 *      Legal volumes are 0..64.
 *
 * [13]: Pattern Break
 *      Where [13][x][y] means "stop the pattern after this division, and
 *      continue the song at the next pattern at division x*10+y" (the 10
 *      is not a typo). Legal divisions are from 0 to 63.
 *
 * [14][0]: Set filter on/off
 *      Where [14][0][x] means "set sound filter ON if x is 0, and OFF is x
 *      is 1". This is a hardware command for some Amigas, so if you don't
 *      understand it, it is better not to use it.
 *
 * [14][1]: Fineslide up
 *      Where [14][1][x] means "decrement the period of the current sample
 *      by x". The incrementing takes place at the beginning of the
 *      division, and hence there is no actual sliding. This type of
 *      sliding cannot be continued with effect [5]. You cannot slide
 *      beyond the note B3 (period 113).
 *
 * [14][2]: Fineslide down
 *      Where [14][2][x] means "increment the period of the current sample
 *      by x". Similar to [14][1] but shifts the pitch down. You cannot
 *      slide beyond the note C1 (period 856).
 *
 * [14][3]: Set glissando on/off
 *      Where [14][3][x] means "set glissando ON if x is 1, OFF if x is 0".
 *      Used in conjunction with [3] ('Slide to note'). If glissando is on,
 *      then 'Slide to note' will slide in semitones, otherwise will
 *      perform the default smooth slide.
 *
 * [14][4]: Set vibrato waveform
 *      Where [14][4][x] means "set the waveform of succeeding 'vibrato'
 *      effects to wave #x". [4] is the 'vibrato' effect.  Possible values
 *      for x are:
 *           0 - sine (default)      /\    /\     (2 cycles shown)
 *           4  (without retrigger)     \/    \/
 *
 *           1 - ramp down          | \   | \
 *           5  (without retrigger)     \ |   \ |
 *
 *           2 - square             ,--,  ,--,
 *           6  (without retrigger)    '--'  '--'
 *
 *           3 - random: a random choice of one of the above.
 *           7  (without retrigger)
 *      If the waveform is selected "without retrigger", then it will not
 *      be retriggered from the beginning at the start of each new note.
 *
 * [14][5]: Set finetune value
 *      Where [14][5][x] means "sets the finetune value of the current
 *      sample to the signed nibble x". x has legal values of 0..15,
 *      corresponding to signed nibbles 0..7,-8..-1 (see start of text for
 *      more info on finetune values).
 *
 * [14][6]: Loop pattern
 *      Where [14][6][x] means "set the start of a loop to this division if
 *      x is 0, otherwise after this division, jump back to the start of a
 *      loop and play it another x times before continuing". If the start
 *      of the loop was not set, it will default to the start of the
 *      current pattern. Hence 'loop pattern' cannot be performed across
 *      multiple patterns. Note that loops do not support nesting, and you
 *      may generate an infinite loop if you try to nest 'loop pattern's.
 *
 * [14][7]: Set tremolo waveform
 *      Where [14][7][x] means "set the waveform of succeeding 'tremolo'
 *      effects to wave #x". Similar to [14][4], but alters effect [7] -
 *      the 'tremolo' effect.
 *
 * [14][8]: -- Unused --
 *
 * [14][9]: Retrigger sample
 *      Where [14][9][x] means "trigger current sample every x ticks in
 *      this division". If x is 0, then no retriggering is done (acts as if
 *      no effect was chosen), otherwise the retriggering begins on the
 *      first tick and then x ticks after that, etc.
 *
 * [14][10]: Fine volume slide up
 *      Where [14][10][x] means "increment the volume of the current sample
 *      by x". The incrementing takes place at the beginning of the
 *      division, and hence there is no sliding. You cannot slide beyond
 *      volume 64.
 *
 * [14][11]: Fine volume slide down
 *      Where [14][11][x] means "decrement the volume of the current sample
 *      by x". Similar to [14][10] but lowers volume. You cannot slide
 *      beyond volume 0.
 *
 * [14][12]: Cut sample
 *      Where [14][12][x] means "after the current sample has been played
 *      for x ticks in this division, its volume will be set to 0". This
 *      implies that if x is 0, then you will not hear any of the sample.
 *      If you wish to insert "silence" in a pattern, it is better to use a
 *      "silence"-sample (see above) due to the lack of proper support for
 *      this effect.
 *
 * [14][13]: Delay sample
 *      Where [14][13][x] means "do not start this division's sample for
 *      the first x ticks in this division, play the sample after this".
 *      This implies that if x is 0, then you will hear no delay, but
 *      actually there will be a VERY small delay. Note that this effect
 *      only influences a sample if it was started in this division.
 *
 * [14][14]: Delay pattern
 *      Where [14][14][x] means "after this division there will be a delay
 *      equivalent to the time taken to play x divisions after which the
 *      pattern will be resumed". The delay only relates to the
 *      interpreting of new divisions, and all effects and previous notes
 *      continue during delay.
 *
 * [14][15]: Invert loop
 *      Where [14][15][x] means "if x is greater than 0, then play the
 *      current sample's loop upside down at speed x". Each byte in the
 *      sample's loop will have its sign changed (negated). It will only
 *      work if the sample's loop (defined previously) is not too big. The
 *      speed is based on an internal table.
 *
 * [15]: Set speed
 *      Where [15][x][y] means "set speed to x*16+y". Though it is nowhere
 *      near that simple. Let z = x*16+y. Depending on what values z takes,
 *      different units of speed are set, there being two: ticks/division
 *      and beats/minute (though this one is only a label and not strictly
 *      true). If z=0, then what should technically happen is that the
 *      module stops, but in practice it is treated as if z=1, because
 *      there is already a method for stopping the module (running out of
 *      patterns). If z<=32, then it means "set ticks/division to z"
 *      otherwise it means "set beats/minute to z" (convention says that
 *      this should read "If z<32.." but there are some composers out there
 *      that defy conventions). Default values are 6 ticks/division, and
 *      125 beats/minute (4 divisions = 1 beat). The beats/minute tag is
 *      only meaningful for 6 ticks/division. To get a more accurate view
 *      of how things work, use the following formula:
 *                              24 * beats/minute
 *           divisions/minute = -----------------
 *                               ticks/division
 *      Hence divisions/minute range from 24.75 to 6120, eg. to get a value
 *      of 2000 divisions/minute use 3 ticks/division and 250 beats/minute.
 *      If multiple "set speed" effects are performed in a single division,
 *      the ones on higher-numbered channels take precedence over the ones
 *      on lower-numbered channels. This effect has a large number of
 *      different implementations, but the one described here has the
 *      widest usage.
 */

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

    // return volume_mapping[volume];
    if (volume == 0) return 0;
    return volume * 4 - 1;
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

        case 0x5:
            effect->command = PORTAMENTO_PLUS_VOLUME_SIDE;
            effect->data = mod_data * 4;
            break;

        case 0x8:
            effect->command = SET_PANNING;
            // Arctracker uses both 0x00 and 0x80 to mean pan centre; hard left is 0x01 and hard right is 0xff.
            // FastTracker panning ranges from 0x00 for hard left to 0xff for hard right, so we need to correct
            // value 0 in order to not inadvertently pan hard left as centre by mistake.
            effect->data = mod_data == 0 ? 1 : mod_data;
            break;

        case 0x9:
            effect->command = USE_SAMPLE_SLICE;
            effect->data = mod_data;
            break;

        case 0xA:
            if ((mod_data & 0xf0) != 0) {
                effect->command = CRESCENDO;
                effect->data = 4 * (mod_data >> 4);
            }
            else if ((mod_data & 0x0f) != 0) {
                effect->command = DECRESCENDO;
                effect->data = 4 * (mod_data & 0xf);
            }
            break;

        case 0xB:
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
            {
                const uint8_t e_cmd = mod_data >> 4;
                const uint8_t e_cmd_data = mod_data & 0xf;
                // TODO: E1x and E2x map to FINE_PORTAMENTO but that needs figuring out first.
                if (e_cmd == 10)
                {
                    effect->command = FINE_CRESCENDO;
                    effect->data = e_cmd_data * 4;
                }
                if (e_cmd == 11)
                {
                    effect->command = FINE_DECRESCENDO;
                    effect->data = e_cmd_data * 4;
                }
                if (e_cmd == 12)
                {
                    effect->command = SILENCE_SAMPLE_AFTER_DELAY;
                    effect->data = e_cmd_data;
                }
                if (e_cmd == 14)
                {
                    effect->command = DELAY_NEXT_EVENT;
                    effect->data = e_cmd_data;
                }
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
                // Unfortunately we cannot accurately represent the calculated ticks_per_second in an effect data field.
                // This is the best we can do without redesigning Arctracker's timing system just for Protracker.
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
        instrument->sample_index = sample_no;

        /*
         * Set up the slice offsets to match what the 0x9 command requires.
         */
        for (int slice = 0; slice < 256; slice++)
        {
            uint32_t slice_offset = 256 * slice;
            uint32_t slice_length;
            if (slice_offset < (uint32_t) sample_length)
                slice_length = sample_length - slice_offset;
            else
            {
                slice_offset = 0;
                slice_length = 0;
            }
            instrument->sample_slices[slice] = (sample_slice_t) {
                .offset = slice_offset,
                .length = slice_length,
            };
        }
        printf("\n");

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
        sample->sample_rate = 8287.14f;
        sample->base_note = 24;
        sample->finetune = 16 * (finetune < 8 ? finetune : finetune - 16);

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
    module->volume_mapping_type = VOLUME_AMIGA;

    for (int track = 0; track < module->num_tracks; track++)
    {
        module->tracks[track].effects_displayed = 1;
        module->tracks[track].muted = false;
        switch (track % 4)
        {
            case 0: /* Fall through */
            case 3:
                module->tracks[track].panning = 1; // Full left.
                break;

            case 1: /* Fall through */
            case 2:
                module->tracks[track].panning = 255; // Full right.
                break;

            default:
                module->tracks[track].panning = 0; // Centre.
        }
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