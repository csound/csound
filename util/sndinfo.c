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

static int sfSampSize(int type)
{
    switch (type & SF_FORMAT_SUBMASK) {
      case SF_FORMAT_PCM_16:    return 2;       /* Signed 16 bit data */
      case SF_FORMAT_PCM_24:    return 3;       /* Signed 24 bit data */
      case SF_FORMAT_PCM_32:                    /* Signed 32 bit data */
      case SF_FORMAT_FLOAT:     return 4;       /* 32 bit float data */
      case SF_FORMAT_DOUBLE:    return 8;       /* 64 bit float data */
    }
    return 1;
}

static char *type2string(int x)
{
    switch (x) {
      case TYP_WAV:     return "WAV";
      case TYP_AIFF:    return "AIFF";
      case TYP_AU:      return "AU";
      case TYP_RAW:     return "RAW";
      case TYP_PAF:     return "PAF";
      case TYP_SVX:     return "SVX";
      case TYP_NIST:    return "NIST";
      case TYP_VOC:     return "VOC";
      case TYP_IRCAM:   return "IRCAM";
      case TYP_W64:     return "W64";
      case TYP_MAT4:    return "MAT4";
      case TYP_MAT5:    return "MAT5";
      case TYP_PVF:     return "PVF";
      case TYP_XI:      return "XI";
      case TYP_HTK:     return "HTK";
#ifdef SF_FORMAT_SDS
      case TYP_SDS:     return "SDS";
#endif
      default:          return "(unknown)";
    }
}

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
                        (long) (sfSampSize(sf_info.format) * 8),
                        type2string(SF2TYPE(sf_info.format)),
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

