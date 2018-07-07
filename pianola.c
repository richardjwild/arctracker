#include "arctracker.h"
#include "pianola.h"

static bool enabled = false;
static int pianola_tracks;

static const
char *notes[] = {"---",
                 "C-1", "C#1", "D-1", "D#1", "E-1", "F-1", "F#1", "G-1", "G#1", "A-1", "A#1", "B-1",
                 "C-2", "C#2", "D-2", "D#2", "E-2", "F-2", "F#2", "G-2", "G#2", "A-2", "A#2", "B-2",
                 "C-3", "C#3", "D-3", "D#3", "E-3", "F-3", "F#3", "G-3", "G#3", "A-3", "A#3", "B-3"};

static const
char alphanum[] = {'-',
                   '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
                   'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J',
                   'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',
                   'U', 'V', 'W', 'X', 'Y', 'Z'};

void enable_pianola(int channels)
{
    enabled = true;
    pianola_tracks = channels;
}

void pianola_roll(positions_t *positions, channel_event_t *line)
{
    if (enabled)
    {
        printf("%2d %2d | ", positions->position_in_sequence, positions->position_in_pattern);
        for (int track = 0; track < pianola_tracks; track++)
        {
            channel_event_t event = line[track];
            printf(
                    "%s %c%c%X%X | ",
                    notes[event.note],
                    alphanum[event.sample],
                    alphanum[event.effects[0].code + 1],
                    (event.effects[0].data >> 4) & 0xf,
                    event.effects[0].data & 0xf);
        }
        printf("\n");
    }
}
