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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
    02111-1307 USA
*/

#include "std_util.h"                               /*  SNDINFO.C  */
#include <sndfile.h>
#include "soundio.h"

/* Some of the information is borrowed from libsndfile's sndfile-info code */

static int sndinfo(CSOUND *csound, int argc, char **argv)
{
    char    *infilnam, *fname;
    char    channame[32];
    int     retval = 0;
    int     instr_info = 0;
    int     bcast_info = 0;
    SF_INFO sf_info;
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
      if (fname == NULL) {
        csound->Message(csound, Str("%s:\n\tcould not find\n"), infilnam);
        retval = -1;
        continue;
      }
      memset(&sf_info, 0, sizeof(SF_INFO));
      hndl = sf_open(fname, SFM_READ, &sf_info);
      if (hndl == NULL) {
        csound->Message(csound, Str("%s: Not a sound file\n"), fname);
        csound->Free(csound, fname);
        retval = -1;
        continue;
      }
      else {
        csound->NotifyFileOpened(csound, fname,
                            csound->sftype2csfiletype(sf_info.format), 0, 0);
        csound->Message(csound, "%s:\n", fname);
        csound->Free(csound, fname);
        switch (sf_info.channels) {
        case 1:
          strcpy(channame, Str("monaural"));
          break;
        case 2:
          strcpy(channame, Str("stereo"));
          break;
        case 4:
          strcpy(channame, Str("quad"));
          break;
        case 6:
          strcpy(channame, Str("hex"));
          break;
        case 8:
          strcpy(channame, Str("oct"));
          break;
        default:
          snprintf(channame, 32, "%d-channel", sf_info.channels);
          break;
        }
        csound->Message(csound,
                        Str("\tsrate %ld, %s, %ld bit %s, %5.3f seconds\n"),
                        (long) sf_info.samplerate, channame,
                        (long) (csound->sfsampsize(sf_info.format) * 8),
                        csound->type2string(SF2TYPE(sf_info.format)),
                        (MYFLT) sf_info.frames / sf_info.samplerate);
        csound->Message(csound, Str("\t(%ld sample frames)\n"),
                                (long) sf_info.frames);
        if (instr_info) {
          SF_INSTRUMENT inst;
          int     k;

          if (sf_command(hndl, SFC_GET_INSTRUMENT, &inst, sizeof (inst)) != 0) {
            csound->Message(csound, Str("  Gain        : %d\n"),
                            inst.gain);
            csound->Message(csound, Str("  Base note   : %d\n"),
                            inst.basenote);
            csound->Message(csound, Str("  Velocity    : %d - %d\n"),
                            (int) inst.velocity_lo, (int) inst.velocity_hi);
            csound->Message(csound, Str("  Key         : %d - %d\n"),
                            (int) inst.key_lo, (int) inst.key_hi);
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
        if (bcast_info) {
          SF_BROADCAST_INFO bext;

          if (sf_command(hndl, SFC_GET_BROADCAST_INFO, &bext, sizeof (bext))
              != 0) {
            csound->Message(csound, Str("Description      : %.*s\n"),
                            (int) sizeof (bext.description), bext.description);
            csound->Message(csound, Str("Originator       : %.*s\n"),
                            (int) sizeof (bext.originator), bext.originator);
            csound->Message(csound, Str("Origination ref  : %.*s\n"),
                            (int) sizeof (bext.originator_reference),
                            bext.originator_reference);
            csound->Message(csound, Str("Origination date : %.*s\n"),
                            (int) sizeof (bext.origination_date),
                            bext.origination_date);
            csound->Message(csound, Str("Origination time : %.*s\n"),
                            (int) sizeof (bext.origination_time),
                            bext.origination_time);
            csound->Message(csound, Str("BWF version      : %d\n"),
                            bext.version);
            csound->Message(csound, Str("UMID             : %.*s\n"),
                            (int) sizeof (bext.umid), bext.umid);
            csound->Message(csound, Str("Coding history   : %.*s\n"),
                            bext.coding_history_size, bext.coding_history);
          }
        }
        sf_close(hndl);
      }
    }

    return retval;
}

/* module interface */

int sndinfo_init_(CSOUND *csound)
{
    int retval = csound->AddUtility(csound, "sndinfo", sndinfo);
    if (!retval) {
      retval =
        csound->SetUtilityDescription(csound, "sndinfo",
                                      Str("Prints information about sound files"));
    }
    return retval;
}

