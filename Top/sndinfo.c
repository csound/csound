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
#include "soundio.h"
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef mills_macintosh
#include "MacTransport.h"
#endif

int		sndinfo(int argc, char **argv)
{
    char    *infilnam;
    int     infd, openin(char *);
    SOUNDIN *p;         /* space allocated here */
    HEADATA *hdr, *readheader(int, char*, SOUNDIN*);
    extern  char *getstrformat(int);

    sssfinit();

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
      p = (SOUNDIN *) mcalloc((long)sizeof(SOUNDIN));
      if ((hdr = readheader(infd, infilnam, p)) != NULL && !hdr->readlong) {
        long sframes = hdr->audsize / hdr->sampsize / hdr->nchanls;
        char channame[100];
        if (hdr->filetyp == TYP_AIFF) {
          AIFFDAT *adp;
          printf(Str(X_565,"\tAIFF soundfile"));
          if ((adp = hdr->aiffdata) != NULL
              && (adp->loopmode1 || adp->loopmode2))
            printf(Str(X_84,", looping with modes %d, %d"),
                   adp->loopmode1, adp->loopmode2);
          else printf(Str(X_85,", no looping"));
          printf("\n");
        }
        else if (hdr->filetyp == TYP_WAV)
          printf(Str(X_567,"\tWAVE soundfile\n"));
        else printf("%s:\n", retfilnam);
        switch (hdr->nchanls) {
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
          sprintf(channame, "%ld-channel", hdr->nchanls);
          break;
        }
        printf(Str(X_579,"\tsrate %ld, %s, %ld bit %s, %4.2f seconds\n"),
               hdr->sr, channame,
               hdr->sampsize * 8, getstrformat(hdr->format),
               (MYFLT)sframes / hdr->sr);
        printf(Str(X_576,"\theadersiz %ld, datasiz %ld (%ld sample frames)\n"),
               hdr->hdrsize, hdr->audsize, sframes);
      }
      else printf(Str(X_74,"%s:\n\tno recognizable soundfile header\n"),
                  retfilnam);
#ifdef mills_macintosh
      nchnls = hdr->nchanls;
      O.outsampsiz = hdr->sampsize;
      esr = hdr->sr;
      transport.osfd = infd;
      O.filetyp = hdr->filetyp;
      O.informat = hdr->format;
      if (hdr->filetyp == 0) transport.eoheader = 0;
      else transport.eoheader = lseek(transport.osfd,(off_t)0L,SEEK_CUR);
/*          printf("transport.eoheader = %d\n",transport.eoheader); */
      fflush(stdout);
      transport.state &= ~kUtilPerf;
      transport.state |= kGenerating;
      transport.state = SetTransportState(transport.d,transport.state,
                                          kGenFinished,0);
      transport.state |= kFileReOpened;
      O.outbufsamps = 8192;
      O.oMaxLag = 4096;
      while (csoundYield(NULL));
#endif
      mfree((char *)p);
      close(infd);
    }
    exit(0);
    return 0;
}



