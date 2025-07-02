/*
    sndinfo.c:

    Copyright (C) 1991, 2006 Barry Vercoe, John ffitch, Matt Ingalls,
                             Erik de Castro Lopo

    This file is part of Csound.

    The Csound Library is free software; you can redistribute it
    and/or modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    Csound is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with Csound; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
    02110-1301 USA
*/

#include "std_util.h"                               /*  SNDINFO.C  */
#include "soundio.h"

/* Some of the information is borrowed from libsndfile's sndfile-info code */

static int32_t sndinfo(CSOUND *csound, int32_t argc, char **argv)
{
    char    *infilnam, *fname;
    char    channame[32];
    int32_t     retval = 0;
    int32_t     instr_info = 0;
    int32_t     bcast_info = 0;
    SFLIB_INFO sflib_info;
    SNDFILE *hndl;

    while (--argc) {
      infilnam = *++argv;
      if (strncmp(infilnam, "-j", 2) == 0) {    /* Skip -j option */
        if (infilnam[2] == '\0' && argc > 1)
          ++argv, --argc;
        continue;
      }
      if (strcmp(infilnam, "-i") == 0) {
        instr_info = 1;
        continue;
      }
      if (strncmp(infilnam, "-i", 2) == 0) {
        instr_info = atoi(infilnam + 2);
        continue;
      }
      if (strcmp(infilnam, "-b") == 0) {
        bcast_info = 1;
        continue;
      }
      if (strncmp(infilnam, "-b", 2) == 0) {
        bcast_info = atoi(infilnam + 2);
        continue;
      }
      fname = csound->FindInputFile(csound, infilnam, "SFDIR;SSDIR");
      if (UNLIKELY(fname == NULL)) {
        csound->Message(csound, Str("%s:\n\tcould not find\n"), infilnam);
        retval = -1;
        continue;
      }
      memset(&sflib_info, 0, sizeof(SFLIB_INFO));
      hndl = csound->SndfileOpen(csound,fname, SFM_READ, &sflib_info);
      if (UNLIKELY(hndl == NULL)) {
        csound->Message(csound, Str("%s: Not a sound file\n"), fname);
        csound->Free(csound, fname);
        retval = -1;
        continue;
      }
      else {
        csound->NotifyFileOpened(csound, fname,
                            csound->SndfileType2CsfileType(sflib_info.format), 0, 0);
        csound->Message(csound, "%s:\n", fname);
        csound->Free(csound, fname);
        switch (sflib_info.channels) {
        case 1:
          strncpy(channame, Str("monaural"), 30);
          break;
        case 2:
          strncpy(channame, Str("stereo"), 30);
          break;
        case 4:
          strncpy(channame, Str("quad"), 30);
          break;
        case 6:
          strncpy(channame, Str("hex"),30);
          break;
        case 8:
          strncpy(channame, Str("oct"),30);
          break;
        default:
          snprintf(channame, 30, "%d-channel", sflib_info.channels);
          break;
        }
        channame[31] = '\0';
        csound->Message(csound,
                        Str("\tsrate %ld, %s, %ld bit %s, %5.3f seconds\n"),
                        (long) sflib_info.samplerate, channame,
                        (long) (csound->SndfileSampleSize(sflib_info.format) * 8),
                       csound->Type2String(SF2TYPE(sflib_info.format)),
                        (MYFLT) sflib_info.frames / sflib_info.samplerate);
        csound->Message(csound, Str("\t(%ld sample frames)\n"),
                                (long) sflib_info.frames);
        if (instr_info) {
          SFLIB_INSTRUMENT inst;
          int32_t     k;

          if (csound->SndfileCommand(csound,hndl, SFC_GET_INSTRUMENT, &inst, sizeof (inst)) != 0) {
            csound->Message(csound, Str("  Gain        : %d\n"),
                            inst.gain);
            csound->Message(csound, Str("  Base note   : %d\n"),
                            inst.basenote);
            csound->Message(csound, Str("  Velocity    : %d - %d\n"),
                            (int32_t) inst.velocity_lo, (int32_t) inst.velocity_hi);
            csound->Message(csound, Str("  Key         : %d - %d\n"),
                            (int32_t) inst.key_lo, (int32_t) inst.key_hi);
            csound->Message(csound, Str("  Loop points : %d\n"),
                            inst.loop_count);

            for (k = 0; k < inst.loop_count; k++)
              csound->Message(csound, Str("  %-2d    Mode : %s    "
                                          "Start : %6d   End : %6d   "
                                          "Count : %6d\n"),
                              k,
                              (inst.loops[k].mode == SF_LOOP_NONE ? "none" :
                               inst.loops[k].mode == SF_LOOP_FORWARD ? "fwrd" :
                               inst.loops[k].mode == SF_LOOP_BACKWARD ? "bwrd" :
                               inst.loops[k].mode == SF_LOOP_ALTERNATING ?
                               "alt " : ""),
                              inst.loops[k].start, inst.loops[k].end,
                              inst.loops[k].count);
            csound->Message(csound, "\n");
          }
        }
#ifdef USE_LIBSNDFILE        
        if (bcast_info) {
          
          SF_BROADCAST_INFO bext;

          if (csound->SndfileCommand(csound,hndl, SFC_GET_BROADCAST_INFO, &bext, sizeof (bext))
              != 0) {
            csound->Message(csound, Str("Description      : %.*s\n"),
                            (int32_t) sizeof (bext.description), bext.description);
            csound->Message(csound, Str("Originator       : %.*s\n"),
                            (int32_t) sizeof (bext.originator), bext.originator);
            csound->Message(csound, Str("Origination ref  : %.*s\n"),
                            (int32_t) sizeof (bext.originator_reference),
                            bext.originator_reference);
            csound->Message(csound, Str("Origination date : %.*s\n"),
                            (int32_t) sizeof (bext.origination_date),
                            bext.origination_date);
            csound->Message(csound, Str("Origination time : %.*s\n"),
                            (int32_t) sizeof (bext.origination_time),
                            bext.origination_time);
            csound->Message(csound, Str("BWF version      : %d\n"),
                            bext.version);
            csound->Message(csound, Str("UMID             : %.*s\n"),
                            (int32_t) sizeof (bext.umid), bext.umid);
            csound->Message(csound, Str("Coding history   : %.*s\n"),
                            bext.coding_history_size, bext.coding_history);
          }
        }
#endif
        csound->SndfileClose(csound,hndl);
      }
    }

    return retval;
}

/* module interface */

int32_t sndinfo_init_(CSOUND *csound)
{
    int32_t retval = (csound->GetUtility(csound))->AddUtility(csound, "sndinfo", sndinfo);
    if (!retval) {
      retval =
        (csound->GetUtility(csound))->SetUtilityDescription(csound, "sndinfo",
                                      Str("Prints information about sound files"));
    }
    return retval;
}

