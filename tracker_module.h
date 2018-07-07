#ifndef ARCTRACKER_TRACKER_MODULE_H
#define ARCTRACKER_TRACKER_MODULE_H

#include "arctracker.h"
#include "read_mod.h"

#define TRACKER_FORMAT "TRACKER"

#define MAX_CHANNELS_TRK 8
#define MAX_LEN_TUNENAME_TRK 32
#define MAX_LEN_AUTHOR_TRK 32
#define MAX_LEN_SAMPLENAME_TRK 20
#define NUM_SAMPLES 256

#define MUSX_CHUNK "MUSX"
#define MVOX_CHUNK "MVOX"
#define STER_CHUNK "STER"
#define MNAM_CHUNK "MNAM"
#define ANAM_CHUNK "ANAM"
#define MLEN_CHUNK "MLEN"
#define PNUM_CHUNK "PNUM"
#define PLEN_CHUNK "PLEN"
#define SEQU_CHUNK "SEQU"
#define PATT_CHUNK "PATT"
#define SAMP_CHUNK "SAMP"
#define SNAM_CHUNK "SNAM"
#define SVOL_CHUNK "SVOL"
#define SLEN_CHUNK "SLEN"
#define ROFS_CHUNK "ROFS"
#define RLEN_CHUNK "RLEN"
#define SDAT_CHUNK "SDAT"

#define ARPEGGIO_COMMAND_TRK 0      // 0
#define PORTUP_COMMAND_TRK 1        // 1
#define PORTDOWN_COMMAND_TRK 2      // 2
#define TONEPORT_COMMAND_TRK 3      // 3
#define BREAK_COMMAND_TRK 11        // B
#define STEREO_COMMAND_TRK 14       // E
#define VOLSLIDEUP_COMMAND_TRK 16   // G
#define VOLSLIDEDOWN_COMMAND_TRK 17 // H
#define JUMP_COMMAND_TRK 19         // J
#define SPEED_COMMAND_TRK 28        // S
#define VOLUME_COMMAND_TRK 31       // V

#define SAMPLE_INVALID NULL

format_t tracker_format();

#endif //ARCTRACKER_TRACKER_MODULE_H
