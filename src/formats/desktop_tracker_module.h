#ifndef ARCTRACKER_DESKTOP_TRACKER_MODULE_H
#define ARCTRACKER_DESKTOP_TRACKER_MODULE_H

#include <arctracker.h>
#include <io/read_mod.h>

#define DESKTOP_TRACKER_FORMAT "DESKTOP TRACKER"
#define DTT_FILE_IDENTIFIER "DskT"

#define MAX_LEN_TUNENAME_DSKT 64
#define MAX_LEN_AUTHOR_DSKT 64
#define MAX_LEN_SAMPLENAME_DSKT 32

#define ARPEGGIO_COMMAND_DSKT 0x0
#define PORTUP_COMMAND_DSKT 0x1
#define PORTDOWN_COMMAND_DSKT 0x2
#define TONEPORT_COMMAND_DSKT 0x3
#define VIBRATO_COMMAND_DSKT 0x4             // not implemented
#define DELAYEDNOTE_COMMAND_DSKT 0x5         // not implemented
#define RELEASESAMP_COMMAND_DSKT 0x6         // not implemented
#define TREMOLO_COMMAND_DSKT 0x7             // not implemented
#define PHASOR_COMMAND1_DSKT 0x8             // not implemented
#define PHASOR_COMMAND2_DSKT 0x9             // not implemented
#define VOLSLIDE_COMMAND_DSKT 0xa
#define JUMP_COMMAND_DSKT 0xb
#define VOLUME_COMMAND_DSKT 0xc
#define STEREO_COMMAND_DSKT 0xd
#define STEREOSLIDE_COMMAND_DSKT 0xe         // not implemented
#define SPEED_COMMAND_DSKT 0xf
#define ARPEGGIOSPEED_COMMAND_DSKT 0x10      // not implemented
#define FINEPORTAMENTO_COMMAND_DSKT 0x11
#define CLEAREPEAT_COMMAND_DSKT 0x12         // not implemented
#define SETVIBRATOWAVEFORM_COMMAND_DSKT 0x14 // not implemented
#define LOOP_COMMAND_DSKT 0x16               // not implemented
#define SETTREMOLOWAVEFORM_COMMAND_DSKT 0x17 // not implemented
#define SETFINETEMPO_COMMAND_DSKT 0x18
#define RETRIGGERSAMPLE_COMMAND_DSKT 0x19    // not implemented
#define FINEVOLSLIDE_COMMAND_DSKT 0x1a
#define HOLD_COMMAND_DSKT 0x1b               // not implemented
#define NOTECUT_COMMAND_DSKT 0x1c            // not implemented
#define NOTEDELAY_COMMAND_DSKT 0x1d          // not implemented
#define PATTERNDELAY_COMMAND_DSKT 0x1e       // not implemented

#define IS_MULTIPLE_EFFECT(raw_event) ((raw_event) & (0x1f << 17)) > 0

format_t desktop_tracker_format();

#endif //ARCTRACKER_DESKTOP_TRACKER_MODULE_H
