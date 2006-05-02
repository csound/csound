/*
    sndinfUG.c:

    Copyright (C) 1999 matt ingalls, Richard Dobson

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

/* sndinfUG.c         -matt 7/25/99
             ugens to retrieve info about a sound file */

#include "csoundCore.h"
#include "soundio.h"
#include "sndinfUG.h"
#include "oload.h" /* for strset */
#include "pvfileio.h"
#include <sndfile.h>

typedef struct {         /* struct for passing data to/from sfheader routines */
    long    sr;
    long    nchanls;
    long    sampsize;
    long    format;
    long    hdrsize;
    int     filetyp;
    AIFFDAT *aiffdata;
    long    audsize;
    long    readlong;
    long    firstlong;
} HEADATA;

static int anal_filelen(CSOUND *csound, SNDINFO *p, MYFLT *p_length);

static HEADATA *getsndinfo(CSOUND *csound, SNDINFO *p)
{
    HEADATA *hdr = NULL;
    char    *sfname, *s, soundiname[512];
    SNDFILE *sf;
    SF_INFO sfinfo;

    csound->strarg2name(csound, soundiname, p->ifilno, "soundin.",
                                p->XSTRCODE);
    sfname = soundiname;
    if (strcmp(sfname, "-i") == 0) {    /* get info on the -i    */
      if (!csound->oparms->infilename)  /* commandline inputfile */
        csound->Die(csound, Str("no infile specified in the commandline"));
      sfname = csound->oparms->infilename;
    }
    s = csoundFindInputFile(csound, sfname, "SFDIR;SSDIR");
    if (s == NULL) {                        /* open with full dir paths */
      /* RWD 5:2001 better to exit in this situation ! */
      csound->Die(csound, Str("diskinfo cannot open %s"), sfname);
    }
    sfname = s;                             /* & record fullpath filnam */
    hdr = (HEADATA*) mcalloc(csound, sizeof(HEADATA));
    memset(&sfinfo, 0, sizeof(SF_INFO));
    sf = sf_open(sfname, SFM_READ, &sfinfo);
    if (sf == NULL) {
      /* open failed: maybe raw file ? */
      memset(&sfinfo, 0, sizeof(SF_INFO));
      sfinfo.samplerate = (int) (csound->esr + FL(0.5));
      sfinfo.channels = 1;
      sfinfo.format = (int) FORMAT2SF(csound->oparms->outformat)
                      | (int) TYPE2SF(TYP_RAW);
      /* try again */
      sf = sf_open(sfname, SFM_READ, &sfinfo);
    }
    if (sf == NULL) {
      csound->Die(csound, Str("diskinfo cannot open %s"), sfname);
    }
    mfree(csound, sfname);
    hdr->sr = (long) sfinfo.samplerate;
    hdr->nchanls = (long) sfinfo.channels;
    hdr->format = (long) SF2FORMAT(sfinfo.format);
    switch ((int) hdr->format) {
      case AE_SHORT: hdr->sampsize = 2L; break;
      case AE_24INT: hdr->sampsize = 3L; break;
      case AE_LONG:
      case AE_FLOAT: hdr->sampsize = 4L; break;
      default:       hdr->sampsize = 1L; break;
    }
    hdr->filetyp = (int) SF2TYPE(sfinfo.format);
    hdr->audsize = (long) sfinfo.frames * hdr->sampsize * hdr->nchanls;
    sf_close(sf);
    return hdr;
}

int filelen(CSOUND *csound, SNDINFO *p)
{
    HEADATA *hdr;
    MYFLT dur = FL(0.0);        /*RWD 8:2001 */

    if (anal_filelen(csound, p, &dur)) {
      *(p->r1) = dur;
    }
    /* RWD 8:2001 now set to quit on failure, else we have bad hdr */
    else {
      hdr = getsndinfo(csound, p);
      *(p->r1) = (MYFLT) hdr->audsize
                 / ((MYFLT) hdr->sampsize * (MYFLT) hdr->nchanls
                    * (MYFLT) hdr->sr);
      mfree(csound, hdr);
    }
    return OK;
}

int filenchnls(CSOUND *csound, SNDINFO *p)
{
    HEADATA *hdr;

    hdr = getsndinfo(csound, p);
    *(p->r1) = (MYFLT) hdr->nchanls;
    mfree(csound, hdr);
    return OK;
}

int filesr(CSOUND *csound, SNDINFO *p)
{
    HEADATA *hdr;

    hdr = getsndinfo(csound, p);
    *(p->r1) = (MYFLT) hdr->sr;
    mfree(csound, hdr);
    return OK;
}

/* RWD 8:2001: now supports all relevant files, */
/* and scans overall peak properly */

int filepeak(CSOUND *csound, SNDINFOPEAK *p)
{
    int     channel = (int) (*p->channel + FL(0.5));
    char    *sfname, soundiname[512];
    void    *fd;
    SNDFILE *sf;
    double  peakVal = -1.0;
    int     fmt, typ;
    SF_INFO sfinfo;

    csound->strarg2name(csound, soundiname, p->ifilno, "soundin.", p->XSTRCODE);
    sfname = soundiname;
    if (strcmp(sfname, "-i") == 0) {        /* get info on the -i    */
      sfname = csound->oparms->infilename;  /* commandline inputfile */
      if (sfname == NULL)
        csound->Die(csound, Str("no infile specified in the commandline"));
    }
    memset(&sfinfo, 0, sizeof(SF_INFO));    /* open with full dir paths */
    fd = csound->FileOpen(csound, &sf, CSFILE_SND_R, sfname, &sfinfo,
                                  "SFDIR;SSDIR");
    if (fd == NULL) {
      /* RWD 5:2001 better to exit in this situation ! */
      csound->Die(csound, Str("diskinfo cannot open %s"), sfname);
    }
    if (channel <= 0) {
#if defined(HAVE_LIBSNDFILE) && HAVE_LIBSNDFILE >= 1016
      if (sf_command(sf, SFC_GET_SIGNAL_MAX, &peakVal, sizeof(double))
          == SF_FALSE) {
        csound->Warning(csound, Str("%s: no PEAK chunk was found, scanning "
                                    "file for maximum amplitude"), sfname);
#endif
        if (sf_command(sf, SFC_CALC_NORM_SIGNAL_MAX, &peakVal, sizeof(double))
            != 0)
          peakVal = -1.0;
#if defined(HAVE_LIBSNDFILE) && HAVE_LIBSNDFILE >= 1016
      }
#endif
    }
    else {
      double  *peaks;
      size_t  nBytes;
      if (channel > sfinfo.channels)
        csound->Die(csound, Str("Input channel for peak exceeds number "
                                "of channels in file"));
      nBytes = sizeof(double) * sfinfo.channels;
      peaks = (double*) csound->Malloc(csound, nBytes);
#if defined(HAVE_LIBSNDFILE) && HAVE_LIBSNDFILE >= 1016
      if (sf_command(sf, SFC_GET_MAX_ALL_CHANNELS, peaks, nBytes) == SF_FALSE) {
        csound->Warning(csound, Str("%s: no PEAK chunk was found, scanning "
                                    "file for maximum amplitude"), sfname);
#endif
        if (sf_command(sf, SFC_CALC_NORM_MAX_ALL_CHANNELS, peaks, nBytes) == 0)
          peakVal = peaks[channel - 1];
#if defined(HAVE_LIBSNDFILE) && HAVE_LIBSNDFILE >= 1016
      }
#endif
      csound->Free(csound, peaks);
    }
    if (peakVal < 0.0)
      csound->Die(csound, Str("filepeak: error getting peak value"));
    /* scale output consistently with soundin opcode (see diskin2.c) */
    fmt = sfinfo.format & SF_FORMAT_SUBMASK;
    typ = sfinfo.format & SF_FORMAT_TYPEMASK;
    if ((fmt != SF_FORMAT_FLOAT && fmt != SF_FORMAT_DOUBLE) ||
        (typ == SF_FORMAT_WAV || typ == SF_FORMAT_W64 || typ == SF_FORMAT_AIFF))
      *p->r1 = (MYFLT) (peakVal * (double) csound->e0dbfs);
    else
      *p->r1 = (MYFLT) peakVal;
    csound->FileClose(csound, fd);

    return OK;
}

/* RWD 8:2001 support analysis files in filelen opcode  */

static int anal_filelen(CSOUND *csound, SNDINFO *p,MYFLT *p_dur)
{
    char    *sfname, soundiname[256];
    int     fd;
    PVOCDATA pvdata;
    WAVEFORMATEX fmt;
    MYFLT   nframes, nchans, srate, overlap, arate, dur;

    /* leap thru std hoops to get the name */
    csound->strarg2name(csound, soundiname, p->ifilno, "soundin.",
                                p->XSTRCODE);
    sfname = soundiname;
    /* my prerogative: try pvocex file first! */
    fd = csound->PVOC_OpenFile(csound, sfname, &pvdata, &fmt);
    if (fd >= 0) {
      nframes   = (MYFLT) csound->PVOC_FrameCount(csound, fd);
      nchans    = (MYFLT) fmt.nChannels;
      srate     = (MYFLT) fmt.nSamplesPerSec;
      overlap   = (MYFLT) pvdata.dwOverlap;
      arate     = srate / overlap;
      dur       = (nframes / nchans) / arate;
      *p_dur    = dur;
      csound->PVOC_CloseFile(csound, fd);
      return 1;
    }
    return 0;
}

