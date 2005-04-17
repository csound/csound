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

#include "cs.h"                                       /*  SNDINFO.C  */
#include <sndfile.h>
#include "soundio.h"

extern char* sf2string(int);
extern char* type2string(int);

int sndinfo(int argc, char **argv)
{
    ENVIRON *csound = &cenviron;
    char    *infilnam;
    char    channame[32];
    int     infd;
    SF_INFO sf_info;
    SNDFILE *hndl;

    while (--argc) {
      infilnam = *++argv;
      if (strncmp(infilnam, "-j", 2)==0) { /* Skip -j option */
        if (infilnam[2]!='\0') ++argv, --argc;
        continue;
      }
      if ((infd = openin(infilnam)) < 0) {
        csound->Message(csound, Str("%s:\n\tcould not find\n"),
                                csound->retfilnam);
        continue;
      }
      if ((hndl = sf_open_fd(infd, SFM_READ, &sf_info, 1))==NULL) {
        csound->Message(csound, Str("%s: Not a sound file\n"),
                                csound->retfilnam);
        close(infd);
      }
      else {
        csound->Message(csound, "%s:\n", csound->retfilnam);
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
          csound->Message(csound, channame, "%d-channel", sf_info.channels);
          break;
        }
        csound->Message(csound,
                        Str("\tsrate %ld, %s, %ld bit %s, %4.2f seconds\n"),
                        (long) sf_info.samplerate, channame,
                        (long) (sfsampsize(sf_info.format) * 8),
                        type2string(SF2TYPE(sf_info.format)),
                        (MYFLT)sf_info.frames / sf_info.samplerate);
        csound->Message(csound, Str("\t(%ld sample frames)\n"),
                                (long) sf_info.frames);
        sf_close(hndl);
      }
    }
    return 0;
}

