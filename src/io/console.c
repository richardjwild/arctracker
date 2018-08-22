#include "console.h"
#include "configuration.h"
#include <memory/bits.h>
#include <playroutine/sequence.h>
#include <arctracker.h>

static bool pianola_mode;
static int pianola_tracks;
static int tune_length;

static const
char *NOTES[] =
        {
                "---",
                "C-1", "C#1", "D-1", "D#1", "E-1", "F-1", "F#1", "G-1", "G#1", "A-1", "A#1", "B-1",
                "C-2", "C#2", "D-2", "D#2", "E-2", "F-2", "F#2", "G-2", "G#2", "A-2", "A#2", "B-2",
                "C-3", "C#3", "D-3", "D#3", "E-3", "F-3", "F#3", "G-3", "G#3", "A-3", "A#3", "B-3"
        };

static const
char ALPHANUM[] =
        {
                '-',
                '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
                'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J',
                'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',
                'U', 'V', 'W', 'X', 'Y', 'Z'
        };

static const
char *PANNING[] =
        {
                "LLL", "LL", "L", "C", "R", "RR", "RRR"
        };

void configure_console(const bool pianola, const module_t *module)
{
    pianola_mode = pianola;
    pianola_tracks = module->num_channels;
    tune_length = module->tune_length;
    if (!pianola)
    {
        printf("Playing position 1 of %d", tune_length);
        fflush(stdout);
    }
}

void output_new_position()
{
    printf("\rPlaying position %d of %d ", song_position() + 1, tune_length);
    fflush(stdout);
}

void pianola_roll(const channel_event_t *line)
{
    printf("%2d %2d | ", song_position(), pattern_position());
    for (int track = 0; track < pianola_tracks; track++)
    {
        channel_event_t event = line[track];
        effect_t first_effect = event.effects[0];
        printf(
                "%s %c%c%X%X | ",
                NOTES[event.note],
                ALPHANUM[event.sample],
                ALPHANUM[first_effect.code + 1],
                HIGH_NYBBLE(first_effect.data),
                LOW_NYBBLE(first_effect.data));
    }
    printf("\n");
}

void output_to_console(const channel_event_t *line)
{
    if (pianola_mode)
        pianola_roll(line);
    else
        output_new_position();
}

const char *get_panning(int pan)
{
    if (pan >= 0 && pan <= 6)
        return PANNING[pan];
    else
        return "invalid";
}

void write_info(const module_t module)
{
    printf("File is %s format.\n", module.format);
    printf("Module name: %s\nAuthor: %s\n", module.name, module.author);
    if (configuration().info)
    {
        printf("\n--------------------------------- Song Details ---------------------------------\n\n");
        printf("Channels        : %d\n"
               "Initial speed   : %d\n"
               "Length          : %d\n"
               "Patterns        : %d\n"
               "Samples         : %d\n",
               module.num_channels,
               module.initial_speed,
               module.tune_length,
               module.num_patterns,
               module.num_samples);
        printf("Initial panning : ");
        for (int channel = 0; channel < module.num_channels; channel++)
        {
            printf("[%d:%s] ",
                   module.initial_panning[channel],
                   get_panning(module.initial_panning[channel] - 1));
        }
        printf("\nSequence        :");
        for (int group = 0; group < module.tune_length; group += 10)
        {
            printf("\n");
            for (int position = group; position < (group + 10) && position < module.tune_length; position++)
                printf("[%02d:%d] ",
                       module.sequence[position],
                       module.pattern_lengths[module.sequence[position]]);
        }
        printf("\n\n----------------------------- Filled Sample Slots ------------------------------\n\n");
        printf("No Name                              Vol  Length Loop?  Offset   R-len Transpose\n");
        printf("== ================================= === ======= ===== ======= ======= =========\n");
        for (int sno = 0; sno < module.num_samples; sno++)
        {
            sample_t sample = module.samples[sno];
            if (sample.sample_length > 0)
                printf("%2d %-33s %3d %7d %-5s %7d %7d %9d\n",
                       (sno + 1),
                       sample.name,
                       sample.default_gain,
                       sample.sample_length,
                       (sample.repeats ? "yes" : "no"),
                       sample.repeat_offset,
                       sample.repeat_length,
                       sample.transpose);
        }
    }
}

inline
void warn_clip()
{
    if (configuration().clip_warning)
        printf("!");
}

void finish_console() {
    printf("\n");
}