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
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <sndfile.h>

static int anal_filelen(SNDINFO *p,MYFLT *p_length);

static HEADATA *getsndinfo(ENVIRON *csound, SNDINFO *p)
{
    HEADATA *hdr = NULL;
    char    *sfname, *s, soundiname[512];
    long    filno;
    SNDFILE *sf;
    SF_INFO sfinfo;

    if (*p->ifilno == SSTRCOD) { /* if char string name given */
      if (p->STRARG == NULL)
        strcpy(soundiname,unquote(currevent->strarg));
      else
        strcpy(soundiname,unquote(p->STRARG));    /* unquote it,  else use */
    }
    else if ((filno=(long)*p->ifilno) <= strsmax && strsets != NULL &&
             strsets[filno])
      strcpy(soundiname, strsets[filno]);
    else
      sprintf(soundiname,"soundin.%ld",filno);  /* soundin.filno */

    sfname = soundiname;
    if (strcmp(sfname, "-i") == 0) {    /* get info on the -i    */
      if (!O.infilename)                /* commandline inputfile */
        die(Str("no infile specified in the commandline"));
      sfname = O.infilename;
    }
    s = csoundFindInputFile(csound, sfname, "SFDIR;SSDIR");
    if (s == NULL) {                        /* open with full dir paths */
      sprintf(errmsg,Str("diskinfo cannot open %s"), sfname);
      /* RWD 5:2001 better to exit in this situation ! */
      die(errmsg);
    }
    sfname = s;                             /* & record fullpath filnam */
    hdr = (HEADATA*) mcalloc(csound, sizeof(HEADATA));
    memset(&sfinfo, 0, sizeof(SF_INFO));
    sf = sf_open(sfname, SFM_READ, &sfinfo);
    if (sf == NULL) {
      /* open failed: maybe raw file ? */
      memset(&sfinfo, 0, sizeof(SF_INFO));
      sfinfo.samplerate = (int) (esr + FL(0.5));
      sfinfo.channels = 1;
      sfinfo.format = (int) format2sf(O.outformat) | (int) type2sf(TYP_RAW);
      /* try again */
      sf = sf_open(sfname, SFM_READ, &sfinfo);
    }
    if (sf == NULL) {
      sprintf(errmsg, Str("diskinfo cannot open %s"), sfname);
      mfree(csound, sfname);
      die(errmsg);
    }
    mfree(csound, sfname);
    hdr->sr = (long) sfinfo.samplerate;
    hdr->nchanls = (long) sfinfo.channels;
    hdr->format = (long) sf2format(sfinfo.format);
    switch ((int) hdr->format) {
      case AE_SHORT: hdr->sampsize = 2L; break;
      case AE_24INT: hdr->sampsize = 3L; break;
      case AE_LONG:
      case AE_FLOAT: hdr->sampsize = 4L; break;
      default:       hdr->sampsize = 1L; break;
    }
    hdr->filetyp = (int) sf2type(sfinfo.format);
    hdr->audsize = (long) sfinfo.frames * hdr->sampsize * hdr->nchanls;
    sf_close(sf);
    return hdr;
}

int filelen(ENVIRON *csound, SNDINFO *p)
{
    HEADATA *hdr;
    MYFLT dur = FL(0.0);        /*RWD 8:2001 */

    if (anal_filelen(p, &dur)) {
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


/*RWD 8:2001: now supports all relevant files, and scans overall peak properly */

int filepeak(ENVIRON *csound, SNDINFOPEAK *p)
{
    die(Str("filepeak is not implemented"));
    return NOTOK;
#if 0
    HEADATA *hdr = NULL;
    long readlong;
    int i;
    int channel = (int)(*p->channel + FL(0.5));
    SNDINFO info;

    info.h = p->h;
    info.r1 = p->r1;
    info.ifilno = p->ifilno;
    info.audsize = p->audsize;
    if ((hdr = getsndinfo(&info)) != NULL
        && !(readlong = hdr->readlong)) {         /* & hadn't readin audio */
      if (channel > hdr->nchanls)
        die(Str("Input channel for peak exceeds number of channels in file"));

      if (
          hdr->filetyp == TYP_AIFF ||
          hdr->filetyp == TYP_WAV) { /* assume maxamps are there, (this is bad) */
        /* channel '0' is the overall maxamps */
        /* *p->r1 = hdr->aiffdata->maxamps[(int)(*(p->channel))];*/
        /*RWD*/
        if (hdr->peaksvalid) {
          if (channel==0) {
            float fmaxamp = 0.0f;
            /* get overall maxamp */
            for (i=0;i < hdr->nchanls;i++)
              if (fmaxamp < hdr->peaks[i].value) fmaxamp = hdr->peaks[i].value;
            *p->r1 = fmaxamp;
          }
          else
            *p->r1 = hdr->peaks[channel-1].value;
        }
      }
      else { /* ## should we have an option to calculate peaks? */
        die(Str("No peak information contained in the header of this file"));
      }
    }
    else {
      /* RWD: we ought to be able to recover, in this situation ?
         e.g return -1, which can be trapped in the orc. */
      die(Str("No valid header.  Cannot calculate peak values"));
    }
    return OK;
#endif
}

/* RWD 8:2001 support analysis files in filelen opcode  */

static int anal_filelen(SNDINFO *p,MYFLT *p_dur)
{
    char    *sfname, soundiname[256];
    long filno;
    int fd;
    FILE *fp;
    PVOCDATA pvdata;
    WAVEFORMATEX fmt;
    MYFLT nframes,nchans,srate,overlap,arate,dur;

    /* leap thru std hoops to get the name */
    if (*p->ifilno == SSTRCOD) { /* if char string name given */
      if (p->STRARG == NULL)
        strcpy(soundiname,unquote(currevent->strarg));
      else
        strcpy(soundiname,unquote(p->STRARG));    /* unquote it,  else use */
    }
    else if ((filno=(long)*p->ifilno) <= strsmax && strsets != NULL &&
             strsets[filno])
      strcpy(soundiname, strsets[filno]);
    else
      sprintf(soundiname,"soundin.%ld",filno);  /* soundin.filno */

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
        dur             = (nframes / nchans) / arate;
        *p_dur  = dur;
        fclose(fp);
        return 1;
      }
    }
    return 0;
}

