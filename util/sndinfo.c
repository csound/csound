/*
    sndinfo.c:

    Copyright (C) 1991 Barry Vercoe, John ffitch, matt ingalls

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

#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif
#include "csdl.h"                                   /*  SNDINFO.C  */
#include <sndfile.h>
#include "soundio.h"

static int sndinfo(void *csound_, int argc, char **argv)
{
    ENVIRON *csound = (ENVIRON*) csound_;
    char    *infilnam, *fname;
    char    channame[32];
    int     retval = 0;
    SF_INFO sf_info;
    SNDFILE *hndl;

    while (--argc) {
      infilnam = *++argv;
      if (strncmp(infilnam, "-j", 2) == 0) {    /* Skip -j option */
        if (infilnam[2] != '\0') ++argv, --argc;
        continue;
      }
      fname = csound->FindInputFile(csound, infilnam, "SFDIR;SSDIR");
      if (fname == NULL) {
        csound->Message(csound, Str("%s:\n\tcould not find\n"), infilnam);
        retval = -1;
        continue;
      }
      hndl = sf_open(fname, SFM_READ, &sf_info);
      if (hndl == NULL) {
        csound->Message(csound, Str("%s: Not a sound file\n"), fname);
        csound->Free(csound, fname);
        retval = -1;
        continue;
      }
      else {
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
          sprintf(channame, "%d-channel", sf_info.channels);
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
        sf_close(hndl);
      }
    }
    return retval;
}

/* module interface */

PUBLIC int csoundModuleCreate(void *csound)
{
    int retval = ((ENVIRON*) csound)->AddUtility(csound, "sndinfo", sndinfo);
    if (!retval) {
      retval = ((ENVIRON*) csound)->SetUtilityDescription(csound, "sndinfo",
                    "Prints information about sound files");
    }
    return retval;
}

