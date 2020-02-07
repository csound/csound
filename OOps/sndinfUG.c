/*
    sndinfUG.c:

    Copyright (C) 1999 matt ingalls, Richard Dobson
              (C) 2006 Istvan Varga

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

/* sndinfUG.c         -matt 7/25/99
             ugens to retrieve info about a sound file */

#include "csoundCore.h"
#include <sndfile.h>

#include "soundio.h"
#include "sndinfUG.h"
#include "pvfileio.h"
#include "convolve.h"

static int32_t getsndinfo(CSOUND *csound, SNDINFO *p, SF_INFO *hdr, int32_t strin)
{
    char    *sfname, *s, soundiname[1024];
    SNDFILE *sf;
    SF_INFO sfinfo;
    int32_t     csFileType;

    memset(hdr, 0, sizeof(SF_INFO));
    /* leap thru std hoops to get the name */
    if (strin)
      strNcpy(soundiname, ((STRINGDAT*)p->ifilno)->data, 1023);
    else if (csound->ISSTRCOD(*p->ifilno)){
      strNcpy(soundiname, get_arg_string(csound, *p->ifilno), 1023);
    }
    else csound->strarg2name(csound, soundiname, p->ifilno, "soundin.",0);


    sfname = soundiname;
    if (strcmp(sfname, "-i") == 0) {    /* get info on the -i    */
      if (UNLIKELY(!csound->oparms->infilename)) {  /* commandline inputfile */
        return
          csound->InitError(csound, Str("no infile specified in the commandline"));
      }
      sfname = csound->oparms->infilename;
    }
    s = csoundFindInputFile(csound, sfname, "SFDIR;SSDIR");
    if (s == NULL) {                    /* open with full dir paths */
      s = csoundFindInputFile(csound, sfname, "SADIR");
      if (UNLIKELY(s == NULL)) {
        /* RWD 5:2001 better to exit in this situation ! */
        return csound->InitError(csound, Str("diskinfo cannot open %s"), sfname);
      }
    }
    sfname = s;                         /* & record fullpath filnam */
    csFileType = CSFTYPE_UNKNOWN;
    memset(&sfinfo, 0, sizeof(SF_INFO));
    sf = sf_open(sfname, SFM_READ, &sfinfo);
    if (sf == NULL) {
      /* open failed: maybe analysis or raw file ? */
      if (*(p->irawfiles) == FL(0.0)) {
        csound->Free(csound, sfname);
        return 0;
      }
      /* check for analysis files */
      memset(hdr, 0, sizeof(SF_INFO));
      {                                 /* convolve */
        FILE      *f;
        CVSTRUCT  cvdata;

        f = fopen(sfname, "rb");
        if (f != NULL) {
          int32_t   n = (int32_t)fread(&cvdata, sizeof(CVSTRUCT), 1, f);
          fclose(f);
          if (n == 1) {
            if (cvdata.magic == (int32_t)CVMAGIC &&
                cvdata.dataFormat == (int32_t)CVMYFLT &&
                cvdata.Format == (int32_t)CVRECT) {
              hdr->frames = (sf_count_t)cvdata.Hlen;
              hdr->samplerate = (int32_t)(cvdata.samplingRate + FL(0.5));
              hdr->channels = (cvdata.channel == (int32_t)ALLCHNLS ?
                               (int32_t)cvdata.src_chnls : 1);
              csFileType = CSFTYPE_CVANAL;
            }
          }
        }
      }
      if (csFileType == CSFTYPE_UNKNOWN) {  /* PVOC */
        int32_t     fd;
        PVOCDATA pvdata;
        WAVEFORMATEX fmt;

        /* RWD: my prerogative: try pvocex file first! */
        fd = csound->PVOC_OpenFile(csound, sfname, &pvdata, &fmt);
        if (fd >= 0) {
          hdr->frames =
            (sf_count_t) (((int32_t)csound->PVOC_FrameCount(csound, fd)
                           / (int32_t)fmt.nChannels)*(sf_count_t)pvdata.dwOverlap);
          hdr->samplerate = (int32_t)fmt.nSamplesPerSec;
          hdr->channels = (int32_t)fmt.nChannels;
          csound->PVOC_CloseFile(csound, fd);
          csFileType = CSFTYPE_PVCEX;
        }
      }
      if (csFileType == CSFTYPE_UNKNOWN) {
        memset(&sfinfo, 0, sizeof(SF_INFO));
        sfinfo.samplerate = (int32_t)(csound->esr + FL(0.5));
        sfinfo.channels = 1;
        sfinfo.format = (int32_t)FORMAT2SF(csound->oparms->outformat)
                        | (int32_t)TYPE2SF(TYP_RAW);
        /* try again */
        sf = sf_open(sfname, SFM_READ, &sfinfo);
      }
    }
    if (UNLIKELY(sf == NULL && csFileType == CSFTYPE_UNKNOWN)) {
      return csound->InitError(csound, Str("diskinfo cannot open %s"), sfname);
    }
    if (sf != NULL) {
      csFileType = sftype2csfiletype(sfinfo.format);
      memcpy(hdr, &sfinfo, sizeof(SF_INFO));
      sf_close(sf);
    }
    /* FIXME: PVOC_OpenFile has already notified since it calls
       FileOpen2(), even if the file was not a PVOC file. */
    if (csFileType != CSFTYPE_PVCEX)
      csoundNotifyFileOpened(csound, sfname, csFileType, 0, 0);
    csound->Free(csound, sfname);
    return 1;
}

int32_t filelen(CSOUND *csound, SNDINFO *p)
{
    SF_INFO hdr;

    if (getsndinfo(csound, p, &hdr, 0))
      *(p->r1) = (MYFLT)((int32_t)hdr.frames) / (MYFLT)hdr.samplerate;
    else
      *(p->r1) = FL(0.0);

    return OK;
}

int32_t filelen_S(CSOUND *csound, SNDINFO *p)
{
    SF_INFO hdr;

    if (getsndinfo(csound, p, &hdr, 1))
      *(p->r1) = (MYFLT)((int32_t)hdr.frames) / (MYFLT)hdr.samplerate;
    else
      *(p->r1) = FL(0.0);

    return OK;
}

int32_t filenchnls_S(CSOUND *csound, SNDINFO *p)
{
    SF_INFO hdr;

    getsndinfo(csound, p, &hdr, 1);
    *(p->r1) = (MYFLT)hdr.channels;

    return OK;
}

int32_t filesr_S(CSOUND *csound, SNDINFO *p)
{
    SF_INFO hdr;

    getsndinfo(csound, p, &hdr, 1);
    *(p->r1) = (MYFLT)hdr.samplerate;

    return OK;
}

int32_t filebit_S(CSOUND *csound, SNDINFO *p)
{
    SF_INFO hdr;
    int32_t bits, format;

    getsndinfo(csound, p, &hdr, 1);
    format = hdr.format &  SF_FORMAT_SUBMASK;
    if (format < 5)
      bits = format*8 ;
    else if (format == 5) bits = 8;
    else if (format == 6) bits = -1;
    else if (format == 7) bits = -2;
    else bits = -format; /* non-PCM data */

    *(p->r1) = (MYFLT) bits;

    return OK;
}


int32_t filenchnls(CSOUND *csound, SNDINFO *p)
{
    SF_INFO hdr;

    getsndinfo(csound, p, &hdr, 0);
    *(p->r1) = (MYFLT)hdr.channels;

    return OK;
}

int32_t filesr(CSOUND *csound, SNDINFO *p)
{
    SF_INFO hdr;

    getsndinfo(csound, p, &hdr, 0);
    *(p->r1) = (MYFLT)hdr.samplerate;

    return OK;
}

int32_t filebit(CSOUND *csound, SNDINFO *p)
{
    SF_INFO hdr;
    int32_t bits, format;

    getsndinfo(csound, p, &hdr, 0);
    format = hdr.format &  SF_FORMAT_SUBMASK;
    if (format < 5)
      bits = format*8 ;
    else if (format == 5) bits = 8;
    else if (format == 6) bits = -1;
    else if (format == 7) bits = -2;
    else bits = -format; /* non-PCM data */

    *(p->r1) = (MYFLT) bits;

    return OK;
}


/* RWD 8:2001: now supports all relevant files, */
/* and scans overall peak properly */



int32_t filepeak_(CSOUND *csound, SNDINFOPEAK *p, char *soundiname)
{
    int32_t     channel = (int32_t)(*p->channel + FL(0.5));
    char    *sfname;
    void    *fd;
    SNDFILE *sf;
    double  peakVal = -1.0;
    int32_t     fmt, typ;
    SF_INFO sfinfo;

    sfname = soundiname;
    if (strcmp(sfname, "-i") == 0) {        /* get info on the -i    */
      sfname = csound->oparms->infilename;  /* commandline inputfile */
      if (UNLIKELY(sfname == NULL))
        return csound->InitError(csound,
                    Str("no infile specified in the commandline"));
    }
    memset(&sfinfo, 0, sizeof(SF_INFO));    /* open with full dir paths */
    fd = csound->FileOpen2(csound, &sf, CSFILE_SND_R, sfname, &sfinfo,
                             "SFDIR;SSDIR", CSFTYPE_UNKNOWN_AUDIO, 0);
    if (UNLIKELY(fd == NULL)) {
      /* RWD 5:2001 better to exit in this situation ! */
      return csound->InitError(csound, Str("diskinfo cannot open %s: %s"),
                               sfname, Str(sf_strerror(NULL)));
    }
    if (channel <= 0) {
      if (sf_command(sf, SFC_GET_SIGNAL_MAX, &peakVal, sizeof(double))
          == SF_FALSE) {
        csound->Warning(csound, Str("%s: no PEAK chunk was found, scanning "
                                    "file for maximum amplitude"), sfname);
        if (sf_command(sf, SFC_CALC_NORM_SIGNAL_MAX,
                       &peakVal, sizeof(double)) != 0)
          peakVal = -1.0;
      }
    }
    else {
      double  *peaks;
      size_t  nBytes;
      if (UNLIKELY(channel > sfinfo.channels))
        return csound->InitError(csound,
                                 Str("Input channel for peak exceeds number "
                                "of channels in file"));
      nBytes = sizeof(double)* sfinfo.channels;
      peaks = (double*)csound->Malloc(csound, nBytes);
      if (sf_command(sf, SFC_GET_MAX_ALL_CHANNELS, peaks, nBytes) == SF_FALSE) {
        csound->Warning(csound, Str("%s: no PEAK chunk was found, scanning "
                                    "file for maximum amplitude"), sfname);
        if (sf_command(sf, SFC_CALC_NORM_MAX_ALL_CHANNELS, peaks, nBytes) == 0)
          peakVal = peaks[channel - 1];
      }
      csound->Free(csound, peaks);
    }
    if (UNLIKELY(peakVal < 0.0))
      return csound->InitError(csound, Str("filepeak: error getting peak value"));
    /* scale output consistently with soundin opcode (see diskin2.c) */
    fmt = sfinfo.format & SF_FORMAT_SUBMASK;
    typ = sfinfo.format & SF_FORMAT_TYPEMASK;
    if ((fmt != SF_FORMAT_FLOAT && fmt != SF_FORMAT_DOUBLE) ||
        (typ == SF_FORMAT_WAV || typ == SF_FORMAT_W64 || typ == SF_FORMAT_AIFF))
      *p->r1 = (MYFLT)(peakVal * (double)csound->e0dbfs);
    else
      *p->r1 = (MYFLT)peakVal;
    csound->FileClose(csound, fd);

    return OK;
}


int32_t filepeak(CSOUND *csound, SNDINFOPEAK *p){

 char soundiname[1024];
 if (csound->ISSTRCOD(*p->ifilno)){
      strNcpy(soundiname, get_arg_string(csound, *p->ifilno), 1023);
    }
  else csound->strarg2name(csound, soundiname, p->ifilno,
                        "soundin.", 0);

 return filepeak_(csound, p, soundiname);
}

int32_t filepeak_S(CSOUND *csound, SNDINFOPEAK *p){

 char soundiname[1024];
 strNcpy(soundiname, ((STRINGDAT*)p->ifilno)->data, 1023);

 return filepeak_(csound, p, soundiname);
}

/* From matt ingalls */
int32_t filevalid(CSOUND *csound, FILEVALID *p)
{
    char soundiname[1024];       /* There is no check on this length */
    *p->r1 = 0;
    if (csound->ISSTRCOD(*p->ifilno)){
      strNcpy(soundiname, get_arg_string(csound, *p->ifilno), 1023);
    }
    else csound->strarg2name(csound, soundiname, p->ifilno,
                        "soundin.", 0);

    if (UNLIKELY(strcmp(soundiname, "-i") == 0)) {    /* get info on the -i    */
      if (csound->oparms->infilename)  /* commandline inputfile */
        *p->r1 = 1;
      return OK;
    }
    if (LIKELY(csound->FindInputFile(csound, soundiname, "SFDIR;SSDIR")))
      *p->r1 = 1;
    return OK;
}


int32_t filevalid_S(CSOUND *csound, FILEVALID *p)
{
    char soundiname[1024];       /* There is no check on this length */
    *p->r1 = 0;
    strNcpy(soundiname, ((STRINGDAT*)p->ifilno)->data, 1023);
    if (UNLIKELY(strcmp(soundiname, "-i") == 0)) {    /* get info on the -i    */
      if (csound->oparms->infilename)  /* commandline inputfile */
        *p->r1 = 1;
      return OK;
    }
    if (LIKELY(csound->FindInputFile(csound, soundiname, "SFDIR;SSDIR")))
      *p->r1 = 1;
    return OK;
}
