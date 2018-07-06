#ifndef ARCTRACKER_TRACKER_MODULE_H
#define ARCTRACKER_TRACKER_MODULE_H

#include "arctracker.h"
#include "read_mod.h"

#define TRACKER_FORMAT "TRACKER"
#define MUSX_CHUNK "MUSX"
#define TINF_CHUNK "TINF"
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
#define SAMPLE_INVALID NULL

format_t tracker_format();

#endif //ARCTRACKER_TRACKER_MODULE_H
