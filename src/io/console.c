#include <io/console.h>
#include <memory/bits.h>
#include <playroutine/sequence.h>

static bool pianola_mode;
static int pianola_tracks;
static int tune_length;

static const
char *notes[] =
        {
                "---",
                "C-1", "C#1", "D-1", "D#1", "E-1", "F-1", "F#1", "G-1", "G#1", "A-1", "A#1", "B-1",
                "C-2", "C#2", "D-2", "D#2", "E-2", "F-2", "F#2", "G-2", "G#2", "A-2", "A#2", "B-2",
                "C-3", "C#3", "D-3", "D#3", "E-3", "F-3", "F#3", "G-3", "G#3", "A-3", "A#3", "B-3"
        };

static const
char alphanum[] =
        {
                '-',
                '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
                'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J',
                'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',
                'U', 'V', 'W', 'X', 'Y', 'Z'
        };

void configure_console(const bool pianola, const module_t *module)
{
    pianola_mode = pianola;
    pianola_tracks = (int) module->num_channels;
    tune_length = (int) module->tune_length;
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
                notes[event.note],
                alphanum[event.sample],
                alphanum[first_effect.code + 1],
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
