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

#include "cs.h"
#include "soundio.h"
#include "sndinfUG.h"
#include "oload.h" /* for strset */
#include "pvoc.h"
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

static int anal_filelen(ENVIRON *csound, SNDINFO *p, MYFLT *p_length);

static HEADATA *getsndinfo(ENVIRON *csound, SNDINFO *p)
{
    HEADATA *hdr = NULL;
    char    *sfname, *s, soundiname[512];
    SNDFILE *sf;
    SF_INFO sfinfo;

    csound->strarg2name(csound, soundiname, p->ifilno, "soundin.",
                                p->XINSTRCODE);
    sfname = soundiname;
    if (strcmp(sfname, "-i") == 0) {    /* get info on the -i    */
      if (!csound->oparms->infilename)  /* commandline inputfile */
        csound->Die(csound, Str("no infile specified in the commandline"));
      sfname = csound->oparms->infilename;
    }
    s = csoundFindInputFile(csound, sfname, "SFDIR;SSDIR");
    if (s == NULL) {                        /* open with full dir paths */
      sprintf(csound->errmsg, Str("diskinfo cannot open %s"), sfname);
      /* RWD 5:2001 better to exit in this situation ! */
      csound->Die(csound, csound->errmsg);
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
      sprintf(csound->errmsg, Str("diskinfo cannot open %s"), sfname);
      mfree(csound, sfname);
      csound->Die(csound, csound->errmsg);
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

int filelen(ENVIRON *csound, SNDINFO *p)
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

int filenchnls(ENVIRON *csound, SNDINFO *p)
{
    HEADATA *hdr;

    hdr = getsndinfo(csound, p);
    *(p->r1) = (MYFLT) hdr->nchanls;
    mfree(csound, hdr);
    return OK;
}

int filesr(ENVIRON *csound, SNDINFO *p)
{
    HEADATA *hdr;

    hdr = getsndinfo(csound, p);
    *(p->r1) = (MYFLT) hdr->sr;
    mfree(csound, hdr);
    return OK;
}

/* RWD 8:2001: now supports all relevant files, */
/* and scans overall peak properly */

int filepeak(ENVIRON *csound, SNDINFOPEAK *p)
{
    csound->Die(csound, Str("filepeak is not implemented"));
    return NOTOK;
#if 0
    int channel = (int)(*p->channel + FL(0.5));
    char    *sfname, *s, soundiname[512];
    SNDFILE *sf;
    SF_INFO sfinfo;

    csound->strarg2name(csound, soundiname, p->ifilno, "soundin.",
                                p->XINSTRCODE);
    sfname = soundiname;
    if (strcmp(sfname, "-i") == 0) {    /* get info on the -i    */
      if (!csound->oparms->infilename)  /* commandline inputfile */
        csound->Die(csound, Str("no infile specified in the commandline"));
      sfname = csound->oparms->infilename;
    }
    s = csoundFindInputFile(csound, sfname, "SFDIR;SSDIR");
    if (s == NULL) {                        /* open with full dir paths */
      sprintf(csound->errmsg, Str("diskinfo cannot open %s"), sfname);
      /* RWD 5:2001 better to exit in this situation ! */
      csound->Die(csound, csound->errmsg);
    }
    sfname = s;                             /* & record fullpath filnam */
    sndfile = sf_open(sfname, SFM_READ, &sfinfo);
    if (channel>sfinfo->channels)
      csound->Die(csound,
                  Str("Input channel for peak exceeds number "
                      "of channels in file"));
    peaks = (double*)mmalloc(sizeof(double)*sfinfo->channels);
    sf_command (sndfile, SFC_CALC_MAX_ALL_CHANNELS, peaks, sizeof(peaks));
    *p->r1 = peaks[channel];
    mfree(peaks);
    return OK;
#endif
}

/* RWD 8:2001 support analysis files in filelen opcode  */

static int anal_filelen(ENVIRON *csound, SNDINFO *p,MYFLT *p_dur)
{
    char    *sfname, soundiname[256];
    int fd;
    FILE *fp;
    PVOCDATA pvdata;
    WAVEFORMATEX fmt;
    MYFLT nframes,nchans,srate,overlap,arate,dur;

    /* leap thru std hoops to get the name */
    csound->strarg2name(csound, soundiname, p->ifilno, "soundin.",
                                p->XINSTRCODE);
    sfname = soundiname;
    /* my prerogative: try pvocex file first! */
    fd = pvoc_openfile(sfname,&pvdata,&fmt);
    if (fd >= 0) {
      nframes   = (MYFLT) pvoc_framecount(fd);
      nchans    = (MYFLT) fmt.nChannels;
      srate     = (MYFLT) fmt.nSamplesPerSec;
      overlap   = (MYFLT) pvdata.dwOverlap;
      arate     = srate /  overlap;
      dur       = (nframes / nchans) / arate;
      *p_dur    = dur;
      pvoc_closefile(fd);
      return 1;
    }
    /* then try old soon-to-die pvoc format */
    fp = fopen(sfname,"rb");
    if (fp) {
      PVSTRUCT hdr;
      int ok = PVReadHdr(fp, &hdr);
      if (ok== PVE_OK) {
        MYFLT frsiz;
        srate   = (MYFLT) hdr.samplingRate;
        nchans  = (MYFLT) hdr.channels;
        overlap = (MYFLT) hdr.frameIncr;
        frsiz   = (MYFLT) (hdr.frameSize + 2);
        /* just assume PVMYFLT format for now... */
        nframes = (MYFLT) (hdr.dataBsize / (nchans * frsiz * sizeof(float)));
        arate   = srate /  overlap;
        dur     = (nframes / nchans) / arate;
        *p_dur  = dur;
        fclose(fp);
        return 1;
      }
    }
    return 0;
}

