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
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

extern char* sf2string(int);
extern char* type2string(int);
extern short sf2type(int);

int sndinfo(int argc, char **argv)
{
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
        printf(Str(X_73,"%s:\n\tcould not find\n"), retfilnam);
        continue;
      }
      if ((hndl = sf_open_fd(infd, SFM_READ, &sf_info, 1))==NULL) {
        printf(Str(X_223,"%s: Not a sound file\n"), retfilnam);
        close(infd);
      }
      else {
        printf("%s:\n", retfilnam);
        switch (sf_info.channels) {
        case 1:
          strcpy(channame, Str(X_1005,"monaural"));
          break;
        case 2:
          strcpy(channame, Str(X_1246,"stereo"));
          break;
        case 4:
          strcpy(channame, Str(X_1148,"quad"));
          break;
        case 6:
          strcpy(channame, Str(X_830,"hex"));
          break;
        case 8:
          strcpy(channame, Str(X_1088,"oct"));
          break;
        default:
          sprintf(channame, "%d-channel", sf_info.channels);
          break;
        }
        printf(Str(X_579,"\tsrate %ld, %s, %ld bit %s, %4.2f seconds\n"),
               sf_info.samplerate, channame,
               sfsampsize(sf_info.format) * 8,
               type2string(sf2type(sf_info.format)),
               (MYFLT)sf_info.frames / sf_info.samplerate);
        printf(Str(X_225,"\t(%ld sample frames)\n"),
               (long)sf_info.frames);
        sf_close(hndl);
      }
    }
    return 0;
}



