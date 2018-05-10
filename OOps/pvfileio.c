/*
    pvfileio.c:

    Copyright (C) 2000 Richard Dobson
              (C) 2005 Istvan Varga

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

/* pvfileio.c
 * pvocex format test routines
 *
 * Initial version RWD May 2000.
 * All rights reserved: work in progress!
 *
 * Manifestly not a complete API yet!
 * In particular, error returns are kept very simplistic at the moment.
 * (and are not even very consistent yet...)
 * In time, a full set of error values and messages will be developed.
 *
 * NB: the RIFF<WAVE> functions only look for, and accept, a PVOCEX format file.
 * NB also: if windows.h is included anywhere (should be no need in this file,
 *          or in pvfileio.h),
 *          the WAVE_FORMAT~ symbols may need to be #ifndef-ed.
 */

/*   very simple CUSTOM window chunk:
 *
 *  <PVXW><size><data>
 *
 *      where size as usual gives the size of the data in bytes.
 *      the size in samples much match dwWinlen (which may not be the same
 *      as N (fft length)
 *      the sample type must be the same as that of the pvoc data itself
 *      (only floatsams supported so far)
 *      values must be normalised to peak of 1.0
 */

/* CSOUND NB: floats must be kept as 'float', not MYFLT,
   as only 32bit floats supported at present.
 */

#include "csoundCore.h"
#include "pvfileio.h"

#if !defined(WAVE_FORMAT_EXTENSIBLE)
#define WAVE_FORMAT_EXTENSIBLE  (0xFFFE)
#endif
#ifndef WAVE_FORMAT_PCM
#define WAVE_FORMAT_PCM         (0x0001)
#endif
#define WAVE_FORMAT_IEEE_FLOAT  (0x0003)

#define PVFILETABLE ((PVOCFILE**) ((CSOUND*) csound)->pvFileTable)

const GUID KSDATAFORMAT_SUBTYPE_PVOC = {
    0x8312b9c2, 0x2e6e, 0x11d4,
    { 0xa8, 0x24, 0xde, 0x5b, 0x96, 0xc3, 0xab, 0x21 }
};

typedef struct pvoc_file {
    WAVEFORMATEX fmtdata;
    PVOCDATA    pvdata;
    int32_t       datachunkoffset;
    int32_t       nFrames;        /* no of frames in file */
    int32_t       FramePos;       /* where we are in file */
    FILE        *fp;
    void        *fd;
    int32_t       curpos;
    int32_t         to_delete;
    int32_t         readonly;
    char        *name;
    float       *customWindow;
} PVOCFILE;

static const char *pvErrorStrings[] = {
    Str_noop("\npvsys: (no error)"),                                /*   0 */
    Str_noop("\npvsys: unknown error"),                             /*  -1 */
    Str_noop("\npvsys: already initialised"),                       /*  -2 */
    Str_noop("\npvsys: bad arguments"),                             /*  -3 */
    Str_noop("\npvsys: bad format parameter"),                      /*  -4 */
    Str_noop("\npvsys: bad window type"),                           /*  -5 */
    Str_noop("\npvsys: too many files open"),                       /*  -6 */
    Str_noop("\npvsys: unable to create file"),                     /*  -7 */
    Str_noop("\npvsys: Internal error: NULL data arrays"),          /*  -8 */
    Str_noop("\npvsys: unable to open file"),                       /*  -9 */
    Str_noop("\npvsys: error reading Source format data"),          /* -10 */
    Str_noop("\npvsys: not a WAVE_EX file"),                        /* -11 */
    Str_noop("\npvsys: bad size for fmt chunk"),                    /* -12 */
    Str_noop("\npvsys: error reading Extended format data"),        /* -13 */
    Str_noop("\npvsys: not a PVOC-EX file"),                        /* -14 */
    Str_noop("\npvsys: error reading Extended pvoc format data"),   /* -15 */
    Str_noop("\npvsys: unknown pvocex Version"),                    /* -16 */
    Str_noop("\npvsys: error reading header"),                      /* -17 */
    Str_noop("\npvsys: not a RIFF file"),                           /* -18 */
    Str_noop("\npvsys: file too small"),                            /* -19 */
    Str_noop("\npvsys: not a WAVE file"),                           /* -20 */
    Str_noop("\npvsys: error reading format chunk"),                /* -21 */
    Str_noop("\npvsys: PVXW chunk found before fmt chunk."),        /* -22 */
    Str_noop("\npvsys: PVXW chunk found but custom window not specified"),/* -23 */
    Str_noop("\npvsys: error reading window data."),                /* -24 */
    Str_noop("\npvsys: bad RIFF file"),                             /* -25 */
    Str_noop("\npvsys: bad format, data chunk before fmt chunk"),   /* -26 */
    Str_noop("\npvsys: custom window chunk PVXW not found"),        /* -27 */
    Str_noop("\npvsys: error skipping unknown WAVE chunk"),         /* -28 */
    Str_noop("\npvsys: bad format in RIFF file"),                   /* -29 */
    Str_noop("\npvsys: error writing header"),                      /* -30 */
    Str_noop("\npvsys: error writing fmt chunk"),                   /* -31 */
    Str_noop("\npvsys: error writing window data."),                /* -32 */
    Str_noop("\npvsys: error updating data chunk"),                 /* -33 */
    Str_noop("\npvsys: error updating riff chunk"),                 /* -34 */
    Str_noop("\npvsys: error seeking to end of file"),              /* -35 */
    Str_noop("\npvsys: file does not exist"),                       /* -36 */
    Str_noop("\npvsys: file not open"),                             /* -37 */
    Str_noop("\npvsys: bad file descriptor"),                       /* -38 */
    Str_noop("\npvsys: error writing data"),                        /* -39 */
    Str_noop("\npvsys: error reading data"),                        /* -40 */
    Str_noop("\npvsys: error rewinding file"),                      /* -41 */
    Str_noop("\npvsys: unable to close file on termination"),       /* -42 */
    NULL                                                            /* -43 */
};

static  int32_t     pvoc_writeheader(CSOUND *csound, PVOCFILE *p);
static  int32_t     pvoc_readheader(CSOUND *csound, PVOCFILE *p,
                                                 WAVEFORMATPVOCEX *pWfpx);

/* thanks to the SNDAN programmers for this! */
/* return 1 for big-endian machine, 0 for little-endian machine */

static inline int32_t byte_order(void)
{
    const int32_t one = 1;
    return (!*((char*) &one));
}

/* low level file I/O */

static inline int32_t pvfile_read_tag(PVOCFILE *p, char *s)
{
    if (UNLIKELY((int32_t) fread(s, 1, 4, p->fp) != 4)) {
      s[0] = '\0';
      return -1;
    }
    s[4] = '\0';
    return 0;
}

static inline int32_t pvfile_write_tag(PVOCFILE *p, const char *s)
{
    if (UNLIKELY((int32_t) fwrite((void*) s, 1, 4, p->fp) != 4))
      return -1;
    return 0;
}

static inline int32_t pvfile_read_16(PVOCFILE *p, void *data, int32_t cnt)
{
    int32_t n = (int32_t) fread(data, sizeof(uint16_t), (size_t) cnt, p->fp);
    if (byte_order()) {
      int32_t     i;
      uint16_t  tmp;
      for (i = 0; i < n; i++) {
        tmp = ((uint16_t*) data)[i];
        tmp = ((tmp & (uint16_t) 0xFF) << 8) | ((tmp & (uint16_t) 0xFF00) >> 8);
        ((uint16_t*) data)[i] = tmp;
      }
    }
    return n;
}

static inline int32_t pvfile_write_16(PVOCFILE *p, void *data, int32_t cnt)
{
    int32_t  n;

    if (byte_order()) {
      uint16_t  tmp;
      for (n = 0; n < cnt; n++) {
        tmp = ((uint16_t*) data)[n];
        tmp = ((tmp & (uint16_t) 0xFF) << 8) | ((tmp & (uint16_t) 0xFF00) >> 8);
        if (fwrite(&tmp, sizeof(uint16_t), 1, p->fp) != (size_t) 1)
          break;
      }
    }
    else
      n = fwrite(data, sizeof(uint16_t), (size_t) cnt, p->fp);
    return (n != cnt);
}

static inline int32_t pvfile_read_32(PVOCFILE *p, void *data, int32_t cnt)
{
    int32_t  n = (int32_t) fread(data, sizeof(uint32_t), (size_t) cnt, p->fp);
    if (byte_order()) {
      int32_t     i;
      uint32_t  tmp;
      for (i = 0; i < n; i++) {
        tmp = ((uint32_t*) data)[i];
        tmp =   ((tmp & (uint32_t) 0x000000FFU) << 24)
              | ((tmp & (uint32_t) 0x0000FF00U) << 8)
              | ((tmp & (uint32_t) 0x00FF0000U) >> 8)
              | ((tmp & (uint32_t) 0xFF000000U) >> 24);
        ((uint32_t*) data)[i] = tmp;
      }
    }
    return n;
}

static inline int32_t pvfile_write_32(PVOCFILE *p, void *data, int32_t cnt)
{
    int32_t  n;

    if (byte_order()) {
      uint32_t  tmp;
      for (n = 0; n < cnt; n++) {
        tmp = ((uint32_t*) data)[n];
        tmp =   ((tmp & (uint32_t) 0x000000FFU) << 24)
              | ((tmp & (uint32_t) 0x0000FF00U) << 8)
              | ((tmp & (uint32_t) 0x00FF0000U) >> 8)
              | ((tmp & (uint32_t) 0xFF000000U) >> 24);
        if (fwrite(&tmp, sizeof(uint32_t), 1, p->fp) != (size_t) 1)
          break;
      }
    }
    else
      n = fwrite(data, sizeof(uint32_t), (size_t) cnt, p->fp);
    return (n != cnt);
}

static int32_t write_guid(PVOCFILE *p, const GUID *pGuid)
{
    int32_t err = 0;
    err |= pvfile_write_32(p, (void*) &(pGuid->Data1), 1L);
    err |= pvfile_write_16(p, (void*) &(pGuid->Data2), 1L);
    err |= pvfile_write_16(p, (void*) &(pGuid->Data3), 1L);
    err |= ((int32_t) fwrite(&(pGuid->Data4[0]), 1, 8, p->fp) != 8);
    return err;
}

static int32_t compare_guids(const GUID *gleft, const GUID *gright)
{
    const char *left = (const char *) gleft, *right = (const char *) gright;
    return !memcmp(left, right, sizeof(GUID));
}

static int32_t write_pvocdata(PVOCFILE *p)
{
    int32_t err = 0;
    err |= pvfile_write_16(p, &(p->pvdata.wWordFormat), 1L);
    err |= pvfile_write_16(p, &(p->pvdata.wAnalFormat), 1L);
    err |= pvfile_write_16(p, &(p->pvdata.wSourceFormat), 1L);
    err |= pvfile_write_16(p, &(p->pvdata.wWindowType), 1L);
    err |= pvfile_write_32(p, &(p->pvdata.nAnalysisBins), 1L);
    err |= pvfile_write_32(p, &(p->pvdata.dwWinlen), 1L);
    err |= pvfile_write_32(p, &(p->pvdata.dwOverlap), 1L);
    err |= pvfile_write_32(p, &(p->pvdata.dwFrameAlign), 1L);
    err |= pvfile_write_32(p, &(p->pvdata.fAnalysisRate), 1L);
    err |= pvfile_write_32(p, &(p->pvdata.fWindowParam), 1L);
    return err;
}

static int32_t write_fmt(PVOCFILE *p)
{
    int32_t err = 0;
    err |= pvfile_write_16(p, &(p->fmtdata.wFormatTag), 1L);
    err |= pvfile_write_16(p, &(p->fmtdata.nChannels), 1L);
    err |= pvfile_write_32(p, &(p->fmtdata.nSamplesPerSec), 1L);
    err |= pvfile_write_32(p, &(p->fmtdata.nAvgBytesPerSec), 1L);
    err |= pvfile_write_16(p, &(p->fmtdata.nBlockAlign), 1L);
    err |= pvfile_write_16(p, &(p->fmtdata.wBitsPerSample), 1L);
    err |= pvfile_write_16(p, &(p->fmtdata.cbSize), 1L);
    return err;
}

static int32_t pvoc_writeWindow(PVOCFILE *p, float *window, uint32_t length)
{
    return (pvfile_write_32(p, window, (int32_t) length));
}

static int32_t pvoc_readWindow(PVOCFILE *p, float *window, uint32_t length)
{
    return (pvfile_read_32(p, window, (int32_t) length) != (int32_t) length);
}

const char *pvoc_errorstr(CSOUND *csound)
{
    int32_t i = -(csound->pvErrorCode);

    if (UNLIKELY(i < 0 || i > 42)) i = 1;
    return (const char *) Str(pvErrorStrings[i]);
}

/***** loosely modelled on CDP sfsys ******
 *      This is a static array, but could be made dynamic in an OOP sort of way.
 *      The idea is that all low-level operations and data
 *      are completely hidden from the user, so that internal
 *      format changes can be made with little or no disruption
 *      to the public functions.
 * But avoiding the full monty of a C++ implementation.
 *******************************************/

int32_t init_pvsys(CSOUND *csound)
{
    if (UNLIKELY(csound->pvNumFiles)) {
      csound->pvErrorCode = -2;
      return 0;
    }
    csound->pvErrorCode = 0;
    return 1;
}

static inline PVOCFILE *pvsys_getFileHandle(CSOUND *csound, int fd)
{
    if (UNLIKELY(fd < 0 || fd >= csound->pvNumFiles))
      return (PVOCFILE*) NULL;
    return (PVFILETABLE[fd]);
}

static int pvsys_createFileHandle(CSOUND *csound)
{
    int32_t i;
    for (i = 0; i < csound->pvNumFiles; i++) {
      if (PVFILETABLE[i] == NULL)
        break;
    }
    if (i >= csound->pvNumFiles) {
      PVOCFILE  **tmp;
      int32_t       j = i;
      /* extend table */
      if (!csound->pvNumFiles) {
        csound->pvNumFiles = 8;
        tmp = (PVOCFILE**) csound->Malloc(csound,
                                          sizeof(PVOCFILE*) * csound->pvNumFiles);
      }
      else {
        csound->pvNumFiles <<= 1;
        tmp = (PVOCFILE**) csound->ReAlloc(csound, csound->pvFileTable,
                                           sizeof(PVOCFILE*) * csound->pvNumFiles);
      }
      if (tmp == NULL)
        return -1;
      csound->pvFileTable = (void*) tmp;
      for ( ; j < csound->pvNumFiles; j++)
        PVFILETABLE[j] = (PVOCFILE*) NULL;
    }
    /* allocate new handle */
    PVFILETABLE[i] = (PVOCFILE*) csound->Malloc(csound, sizeof(PVOCFILE));
    if (PVFILETABLE[i] == NULL)
      return -1;
    memset(PVFILETABLE[i], 0, sizeof(PVOCFILE));
    return i;
}

static void prepare_pvfmt(WAVEFORMATEX *pfmt, uint32 chans,
                          uint32 srate, pv_stype stype)
{
    pfmt->wFormatTag       = WAVE_FORMAT_EXTENSIBLE;
    pfmt->nChannels        = (uint16_t) chans;
    pfmt->nSamplesPerSec   = srate;
    switch (stype) {
    default:
    case (STYPE_16):
      pfmt->wBitsPerSample = (uint16_t) 16;
      pfmt->nBlockAlign    = (uint16_t) (chans * 2 * sizeof(char));
      break;
    case (STYPE_24):
      pfmt->wBitsPerSample = (uint16_t) 24;
      pfmt->nBlockAlign    = (uint16_t) (chans * 3 * sizeof(char));
      break;
    case (STYPE_32):
    case (STYPE_IEEE_FLOAT):
      pfmt->wBitsPerSample = (uint16_t) 32;
      pfmt->nBlockAlign    = (uint16_t) (chans * 4 * sizeof(char));
      break;
    }
    pfmt->nAvgBytesPerSec  = pfmt->nBlockAlign * srate;
    /* if we have extended WindowParam fields, or something,
       will need to adjust this */
    pfmt->cbSize           = 62;
}

/* lots of different ways of doing this!
 * we will need one in the form:
 * int pvoc_fmtcreate(const char *fname, PVOCDATA *p_pvfmt,
 *                    WAVEFORMATEX *p_wvfmt);
 */

/* a simple minimalist function to begin with!*/
/* set D to 0, and/or dwWinlen to 0, to use internal default */
/* fWindow points to userdef window, or is NULL */
/* NB currently this does not enforce a soundfile extension; */
/* probably it should... */

int32_t  pvoc_createfile(CSOUND *csound, const char *filename,
                     uint32 fftlen, uint32 overlap,
                     uint32 chans, uint32 format,
                     int32_t srate, int32_t stype, int32_t wtype,
                     float wparam, float *fWindow, uint32 dwWinlen)
{
    int32_t       fd;
    int32_t     N, D;
    char      *pname;
    PVOCFILE  *p = NULL;
    float     winparam = 0.0f;

    N = fftlen;                         /* keep the CARL varnames for now */
    D = overlap;
    csound->pvErrorCode = -1;

    if (UNLIKELY(N == 0 || (int32_t) chans <= 0 || filename == NULL || D > N)) {
      csound->pvErrorCode = -3;
      return -1;
    }
    if (UNLIKELY(/*format < PVOC_AMP_FREQ ||*/ format > PVOC_COMPLEX)) {
      csound->pvErrorCode = -4;
      return -1;
    }
    if (UNLIKELY(!(wtype >= PVOC_DEFAULT && wtype <= PVOC_CUSTOM))) {
      csound->pvErrorCode = -5;
      return -1;
    }

    /* load it, but can not write until we have a PVXW chunk definition... */
    if (wtype == PVOC_CUSTOM) {

    }

    else if (wtype == PVOC_DEFAULT)
      wtype = PVOC_HAMMING;

    else if (wtype == PVOC_KAISER)
      if (wparam != 0.0f)
        winparam = wparam;
    /* will need an internal default for window parameters... */

    fd = pvsys_createFileHandle(csound);
    if (UNLIKELY(fd < 0)) {
      csound->pvErrorCode = -6;
      return -1;
    }
    p = pvsys_getFileHandle(csound, fd);
    pname = (char *) csound->Malloc(csound, strlen(filename) + 1);
    strcpy(pname, filename);
    p->customWindow = NULL;

    /* setup rendering inforamtion */
    prepare_pvfmt(&p->fmtdata, chans, srate, stype);
    p->pvdata.wWordFormat     = PVOC_IEEE_FLOAT;
    p->pvdata.wAnalFormat     = (uint16_t) format;
    if (stype == STYPE_IEEE_FLOAT)
      p->pvdata.wSourceFormat = WAVE_FORMAT_IEEE_FLOAT;
    else
      p->pvdata.wSourceFormat = WAVE_FORMAT_PCM;
    p->pvdata.wWindowType     = wtype;
    p->pvdata.nAnalysisBins   = (N >> 1) + 1;
    if (dwWinlen == 0)
      p->pvdata.dwWinlen      = N;
    else
      p->pvdata.dwWinlen      = dwWinlen;
    if (D == 0)
      p->pvdata.dwOverlap     = N / 8;
    else
      p->pvdata.dwOverlap     = D;
    p->pvdata.dwFrameAlign    = p->pvdata.nAnalysisBins * 2 * sizeof(float);
    p->pvdata.fAnalysisRate   = (float) srate / (float) p->pvdata.dwOverlap;
    p->pvdata.fWindowParam    = winparam;
    if (fWindow != NULL) {
      p->customWindow = csound->Malloc(csound, dwWinlen * sizeof(float));
      memcpy(p->customWindow, fWindow, dwWinlen * sizeof(float));
    }

    p->fd = csound->FileOpen2(csound, &(p->fp), CSFILE_STD, filename, "wb",
                               "", CSFTYPE_PVCEX, 0);
    if (UNLIKELY(p->fd == NULL)) {
      csound->Free(csound, pname);
      if (p->customWindow)
        csound->Free(csound, p->customWindow);
      csound->Free(csound, p);
      PVFILETABLE[fd] = NULL;
      csound->pvErrorCode = -7;
      return -1;
    }
    p->name = pname;

    if (pvoc_writeheader(csound, p) != 0) {
      csound->FileClose(csound, p->fd);
      (void)remove(p->name);
      csound->Free(csound, p->name);
      if (p->customWindow)
        csound->Free(csound, p->customWindow);
      csound->Free(csound, p);
      PVFILETABLE[fd] = NULL;
      return -1;
    }

    csound->pvErrorCode = 0;
    return fd;
}

int32_t pvoc_openfile(CSOUND *csound,
                  const char *filename, void *data_, void *fmt_)
{
    WAVEFORMATPVOCEX  wfpx;
    char              *pname;
    PVOCFILE          *p = NULL;
    int32_t               fd;
    PVOCDATA         *data = (PVOCDATA *) data_;
    WAVEFORMATEX      *fmt = (WAVEFORMATEX *) fmt_;

    csound->pvErrorCode = -1;
    if (UNLIKELY(data == NULL || fmt == NULL)) {
      csound->pvErrorCode = -8;
      return -1;
    }
    fd = pvsys_createFileHandle(csound);
    if (UNLIKELY(fd < 0)) {
      csound->pvErrorCode = -6;
      return -1;
    }
    p = pvsys_getFileHandle(csound, fd);

    p->customWindow = NULL;
    p->fd = csound->FileOpen2(csound, &(p->fp), CSFILE_STD, filename,
                                   "rb", "SADIR", CSFTYPE_PVCEX, 0);
    if (UNLIKELY(p->fd == NULL)) {
      csound->pvErrorCode = -9;
      csound->Free(csound, p);
      PVFILETABLE[fd] = NULL;
      return -1;
    }
    pname = (char*) csound->Malloc(csound, strlen(filename) + 1);
    strcpy(pname, filename);
    p->name = pname;
    p->readonly = 1;

    if (UNLIKELY(pvoc_readheader(csound, p, &wfpx) != 0)) {
      csound->FileClose(csound, p->fd);
      csound->Free(csound, p->name);
      if (p->customWindow)
        csound->Free(csound, p->customWindow);
      csound->Free(csound, p);
      PVFILETABLE[fd] = NULL;
      return -1;
    }
    memcpy(data, &(wfpx.data), sizeof(PVOCDATA));
    memcpy(fmt, &(wfpx.wxFormat.Format), SIZEOF_WFMTEX);

    csound->pvErrorCode = 0;
    return fd;
}

static int32_t pvoc_readfmt(CSOUND *csound, PVOCFILE *p, WAVEFORMATPVOCEX *pWfpx)
{
    WAVEFORMATEXTENSIBLE  *wxfmt = &(pWfpx->wxFormat);
    WAVEFORMATEX          *fmt = &(wxfmt->Format);
    int32_t                   err = 0;

    memset(pWfpx, 0, sizeof(WAVEFORMATPVOCEX));
    err |= (pvfile_read_16(p, &(fmt->wFormatTag), 1L) != 1L);
    err |= (pvfile_read_16(p, &(fmt->nChannels), 1L) != 1L);
    err |= (pvfile_read_32(p, &(fmt->nSamplesPerSec), 1L) != 1L);
    err |= (pvfile_read_32(p, &(fmt->nAvgBytesPerSec), 1L) != 1L);
    err |= (pvfile_read_16(p, &(fmt->nBlockAlign), 1L) != 1L);
    err |= (pvfile_read_16(p, &(fmt->wBitsPerSample), 1L) != 1L);
    err |= (pvfile_read_16(p, &(fmt->cbSize), 1L) != 1L);
    if (UNLIKELY(err)) {
      csound->pvErrorCode = -10;
      return err;
    }
    /* the first clues this is pvx format...*/
    if (UNLIKELY(fmt->wFormatTag != WAVE_FORMAT_EXTENSIBLE)) {
      csound->pvErrorCode = -11;
      return -1;
    }
    if (UNLIKELY(fmt->cbSize != 62)) {
      csound->pvErrorCode = -12;
      return -1;
    }
    err |= (pvfile_read_16(p, &(wxfmt->Samples.wValidBitsPerSample), 1L) != 1L);
    err |= (pvfile_read_32(p, &(wxfmt->dwChannelMask), 1L) != 1L);
    err |= (pvfile_read_32(p, &(wxfmt->SubFormat.Data1), 1L) != 1L);
    err |= (pvfile_read_16(p, &(wxfmt->SubFormat.Data2), 1L) != 1L);
    err |= (pvfile_read_16(p, &(wxfmt->SubFormat.Data3), 1L) != 1L);
    err |= ((int32_t) fread(&(wxfmt->SubFormat.Data4[0]), 1, 8, p->fp) != 8);
    if (UNLIKELY(err)) {
      csound->pvErrorCode = -13;
      return -1;
    }
    /* ... but this is the clincher */
    if (UNLIKELY(!compare_guids(&(pWfpx->wxFormat.SubFormat),
                                &KSDATAFORMAT_SUBTYPE_PVOC))) {
      csound->pvErrorCode = -14;
      return -1;
    }
    err |= (pvfile_read_32(p, &(pWfpx->dwVersion), 1L) != 1L);
    err |= (pvfile_read_32(p, &(pWfpx->dwDataSize), 1L) != 1L);
    err |= (pvfile_read_16(p, &(pWfpx->data.wWordFormat), 1L) != 1L);
    err |= (pvfile_read_16(p, &(pWfpx->data.wAnalFormat), 1L) != 1L);
    err |= (pvfile_read_16(p, &(pWfpx->data.wSourceFormat), 1L) != 1L);
    err |= (pvfile_read_16(p, &(pWfpx->data.wWindowType), 1L) != 1L);
    err |= (pvfile_read_32(p, &(pWfpx->data.nAnalysisBins), 1L) != 1L);
    err |= (pvfile_read_32(p, &(pWfpx->data.dwWinlen), 1L) != 1L);
    err |= (pvfile_read_32(p, &(pWfpx->data.dwOverlap), 1L) != 1L);
    err |= (pvfile_read_32(p, &(pWfpx->data.dwFrameAlign), 1L) != 1L);
    err |= (pvfile_read_32(p, &(pWfpx->data.fAnalysisRate), 1L) != 1L);
    err |= (pvfile_read_32(p, &(pWfpx->data.fWindowParam), 1L) != 1L);
    if (UNLIKELY(err)) {
      csound->pvErrorCode = -15;
      return -1;
    }
    if (UNLIKELY(pWfpx->dwVersion != PVX_VERSION)) {
      csound->pvErrorCode = -16;
      return -1;
    }


    return 0;
}

static int32_t pvoc_readheader(CSOUND *csound, PVOCFILE *p,
                                            WAVEFORMATPVOCEX *pWfpx)
{
    char      tag[5];
    uint32_t  size;
    uint32_t  riffsize;
    int32_t       fmtseen = 0, windowseen = 0;

    if (UNLIKELY(pvfile_read_tag(p, &(tag[0])) != 0 ||
        strcmp(tag, "RIFF") != 0 ||
                 pvfile_read_32(p, &size, 1L) != 1L)) {
      csound->pvErrorCode = -17;
      return -1;
    }
    if (UNLIKELY(size < 24 * sizeof(uint32_t) + SIZEOF_FMTPVOCEX)) {
      csound->pvErrorCode = -19;
      return -1;
    }
    riffsize = size;
    if (UNLIKELY(pvfile_read_tag(p, &(tag[0])) != 0 || strcmp(tag, "WAVE") != 0)) {
      csound->pvErrorCode = -20;
      return -1;
    }
    riffsize -= sizeof(uint32_t);
    /* loop for chunks */
    while (riffsize > (uint32_t) 0) {
      if (UNLIKELY(pvfile_read_tag(p, &(tag[0])) != 0 ||
                   pvfile_read_32(p, &size, 1L) != 1L)) {
        csound->pvErrorCode = -17;
        return -1;
      }
      riffsize -= 2 * sizeof(uint32_t);
      if (strcmp(tag, "fmt ") == 0) {
        /* bail out if not a pvoc file: not trying to read all WAVE formats!*/
        if (UNLIKELY((int32_t) size < (int32_t) SIZEOF_FMTPVOCEX)) {
          csound->pvErrorCode = -14;
          return -1;
        }
        if (UNLIKELY(pvoc_readfmt(csound, p, pWfpx) != 0)) {
          csound->pvErrorCode = -21;
          return -1;
        }
        riffsize -= SIZEOF_FMTPVOCEX;
        fmtseen = 1;
        memcpy(&(p->fmtdata), &(pWfpx->wxFormat), SIZEOF_WFMTEX);
        memcpy(&(p->pvdata), &(pWfpx->data), sizeof(PVOCDATA));
      }
      else if (strcmp(tag, "PVXW") == 0) {
        if (UNLIKELY(!fmtseen)) {
          csound->pvErrorCode = -22;
          return -1;
        }
        if (UNLIKELY(p->pvdata.wWindowType != PVOC_CUSTOM)) {
          /* whaddayado? can you warn the user and continue? */
          csound->pvErrorCode = -23;
          return -1;
        }
        p->customWindow = csound->Malloc(csound, p->pvdata.dwWinlen * sizeof(float));
        if (UNLIKELY(pvoc_readWindow(p,
                                     p->customWindow, p->pvdata.dwWinlen) != 0)) {
          csound->pvErrorCode = -24;
          return -1;
        }
        windowseen = 1;
      }
      else if (strcmp(tag, "data") == 0) {
        if (UNLIKELY((uint32_t) riffsize != size)) {
          csound->pvErrorCode = -25;
          return -1;
        }
        if (UNLIKELY(!fmtseen)) {
          csound->pvErrorCode = -26;
          return -1;
        }
        if (p->pvdata.wWindowType == PVOC_CUSTOM) {
          if (UNLIKELY(!windowseen)) {
            csound->pvErrorCode = -27;
            return -1;
          }
        }
        p->datachunkoffset = (int32_t) ftell(p->fp);
        p->curpos = p->datachunkoffset;
        /* not m/c frames, for now */
        p->nFrames = size / p->pvdata.dwFrameAlign;
        return 0;
      }
      else {
        /* skip any unknown chunks */
        riffsize -= 2 * sizeof(uint32_t);
        if (UNLIKELY(fseek(p->fp, (int32_t) size, SEEK_CUR) != 0)) {
          csound->pvErrorCode = -28;
          return -1;
        }
        riffsize -= size;
      }
    }
    /* if here, something very wrong! */
    csound->pvErrorCode = -29;
    return -1;
}

static int32_t pvoc_writeheader(CSOUND *csound, PVOCFILE *p)
{
    uint32_t  version, size = (uint32_t) 0;
    int32_t       err = 0;

    err |= pvfile_write_tag(p, "RIFF");
    err |= pvfile_write_32(p, &size, 1L);
    if (UNLIKELY(err)) {
      csound->pvErrorCode = -30;
      return -1;
    }
    size = SIZEOF_WFMTEX + sizeof(uint16_t)
                         + sizeof(uint32_t)
                         + sizeof(GUID)
                         + 2 * sizeof(uint32_t)
                         + sizeof(PVOCDATA);
    err |= pvfile_write_tag(p, "WAVE");
    err |= pvfile_write_tag(p, "fmt ");
    err |= pvfile_write_32(p, &size, 1L);
    if (UNLIKELY(err)) {
      csound->pvErrorCode = -30;
      return -1;
    }
    if (UNLIKELY(write_fmt(p) != 0)) {
      csound->pvErrorCode = -31;
      return -1;
    }
    if (UNLIKELY(pvfile_write_16(p, &(p->fmtdata.wBitsPerSample), 1L) != 0)) {
      csound->pvErrorCode = -31;
      return -1;
    }
    /* we will take this from a WAVE_EX file, in due course */
    size = 0;   /* dwChannelMask */
    if (UNLIKELY(pvfile_write_32(p, &size, 1L) != 0)) {
      csound->pvErrorCode = -31;
      return -1;
    }
    if (UNLIKELY(write_guid(p, &KSDATAFORMAT_SUBTYPE_PVOC) != 0)) {
      csound->pvErrorCode = -31;
      return -1;
    }
    version = (uint32_t) 1;
    size = sizeof(PVOCDATA);
    if (UNLIKELY(pvfile_write_32(p, &version, 1L) != 0 ||
                 pvfile_write_32(p, &size, 1L) != 0)) {
      csound->pvErrorCode = -31;
      return -1;
    }
    if (UNLIKELY(write_pvocdata(p) != 0)) {
      csound->pvErrorCode = -31;
      return -1;
    }
    /* VERY experimental; may not even be a good idea...*/
    if (p->customWindow) {
      if (UNLIKELY(pvfile_write_tag(p, "PVXW") != 0)) {
        csound->pvErrorCode = -30;
        return -1;
      }
      size = p->pvdata.dwWinlen * sizeof(float);
      if (UNLIKELY(pvfile_write_32(p, &size, 1L) != 0)) {
        csound->pvErrorCode = -30;
        return -1;
      }
      if (UNLIKELY(pvoc_writeWindow(p, p->customWindow, p->pvdata.dwWinlen) != 0)) {
        csound->pvErrorCode = -32;
        return -1;
      }
    }
    /* no other chunks to write yet! */
    if (UNLIKELY(pvfile_write_tag(p, "data") != 0)) {
      csound->pvErrorCode = -30;
      return -1;
    }
    /* we need to update size later on... */
    size = (uint32_t) 0;
    if (UNLIKELY(pvfile_write_32(p, &size, 1L) != 0)) {
      csound->pvErrorCode = -30;
      return -1;
    }
    p->datachunkoffset = (int32_t) ftell(p->fp);
    p->curpos = p->datachunkoffset;

    return 0;
}

static int32_t pvoc_updateheader(CSOUND *csound, int32_t ofd)
{
    PVOCFILE  *p = pvsys_getFileHandle(csound, ofd);
    uint32_t  riffsize, datasize;

    if (UNLIKELY(p == NULL)) {
      csound->pvErrorCode = -38;
      return 0;
    }
    if (UNLIKELY(fseek(p->fp,
                       (int32_t) (p->datachunkoffset - sizeof(uint32_t)), SEEK_SET)
                 != 0)) {
      csound->pvErrorCode = -33;
      return 0;
    }
    datasize = p->curpos - p->datachunkoffset;
    if (UNLIKELY(pvfile_write_32(p, &datasize, 1L) != 0)) {
      csound->pvErrorCode = -33;
      return 0;
    }
    if (UNLIKELY(fseek(p->fp, (int32_t) sizeof(uint32_t), SEEK_SET) != 0)) {
      csound->pvErrorCode = -33;
      return 0;
    }
    riffsize = p->curpos - 2 * sizeof(uint32_t);
    if (UNLIKELY(pvfile_write_32(p, &riffsize, 1L) != 0)) {
      csound->pvErrorCode = -34;
      return 0;
    }
    if (UNLIKELY(fseek(p->fp, 0L, SEEK_END) != 0)) {
      csound->pvErrorCode = -35;
      return 0;
    }

    return 1;
}

int32_t pvoc_closefile(CSOUND *csound, int32_t ofd)
{
    PVOCFILE  *p = pvsys_getFileHandle(csound, ofd);
    int32_t       rc = 1;

    csound->pvErrorCode = 0;
    if (UNLIKELY(p == NULL)) {
      csound->pvErrorCode = -36;
      return 0;
    }
    if (UNLIKELY(p->fd == NULL)) {
      csound->pvErrorCode = -37;
      csound->Free(csound, p);
      PVFILETABLE[ofd] = NULL;
      return 0;
    }
    if (!p->readonly)
      if (!pvoc_updateheader(csound, ofd))
        rc = 0;

    csound->FileClose(csound, p->fd);
    if (p->to_delete && !p->readonly)
      (void)remove(p->name);
    csound->Free(csound, p->name);
    csound->Free(csound, p->customWindow);
    csound->Free(csound, p);
    PVFILETABLE[ofd] = NULL;

    return rc;
}

/* does not directly address m/c streams, or alternative numeric formats, yet...
 * so for m/c files, write each frame in turn, for each channel.
 * The format requires multi-channel frames to be interleaved in the usual way:
 * if nChannels= 4, the file will contain:
 * frame[0][0],frame[0][1],frame[0][2],frame[0][3],frme[1][0],frame[1][1].....
 *
 * The idea is to offer e.g. a floats version and a longs version ONLY, but
 * independently of the underlying representation, so that the user can write
 * a floats block, even though the underlying format might be longs or doubles.
 * Most importantly, the user does not have to deal with byte-reversal, which
 * would otherwise always be the case it the user had direct access to the file.
 *
 * So these functions are the most likely to change over time!.
 *
 * return 0 for error, 1 for success. This could change....
 */
int32_t pvoc_putframes(CSOUND *csound, int32_t ofd, const float *frame,
                       int32_t numframes)
{
    PVOCFILE  *p = pvsys_getFileHandle(csound, ofd);
    int32_t     towrite;  /* count in 'words' */

    if (UNLIKELY(p == NULL)) {
      csound->pvErrorCode = -38;
      return 0;
    }
    if (UNLIKELY(p->fd == NULL)) {
      csound->pvErrorCode = -37;
      return 0;
    }
    /* NB doubles not supported yet */
    towrite = (int32_t) p->pvdata.nAnalysisBins * 2L * numframes;
    if (UNLIKELY(pvfile_write_32(p, (void*) frame, towrite) != 0)) {
      csound->pvErrorCode = -39;
      return 0;
    }
    p->FramePos += numframes;
    p->curpos += towrite * sizeof(float);
    return 1;
}

/* Simplistic read function
 * best practice here is to read nChannels frames
 * return -1 for error, 0 for EOF, else numframes read
 */
int32_t pvoc_getframes(CSOUND *csound, int32_t ifd, float *frames,
                                    uint32 nframes)
{
    PVOCFILE  *p = pvsys_getFileHandle(csound, ifd);
    int32_t     toread, got;

    if (UNLIKELY(p == NULL)) {
      csound->pvErrorCode = -38;
      return -1;
    }
    if (UNLIKELY(p->fd == NULL)) {
      csound->pvErrorCode = -37;
      return -1;
    }
    toread = (int32_t) p->pvdata.nAnalysisBins * 2L * (int32_t) nframes;
    got = pvfile_read_32(p, frames, toread);
    if (got != toread) {
      if (UNLIKELY(ferror(p->fp))) {
        csound->pvErrorCode = -40;
        return -1;
      }
      p->curpos += (int32_t) (got * sizeof(float));
      got = got / (int32_t) (p->pvdata.nAnalysisBins * 2);
      p->FramePos += got;
      return (int32_t) got;
    }
    p->curpos += (toread * sizeof(float));
    p->FramePos += (int32_t) nframes;

    return (int32_t) nframes;
}

int32_t pvoc_fseek(CSOUND *csound, int32_t ifd, int32_t offset)
{
    PVOCFILE  *p = pvsys_getFileHandle(csound, ifd);
    int32_t   pos, skipframes, skipsize;

    if (UNLIKELY(p == NULL)) {
      csound->pvErrorCode = -38;
      return -1;
    }
    if (UNLIKELY(p->fd == NULL)) {
      csound->pvErrorCode = -37;
      return -1;
    }
    if (offset == 1)
    skipframes = (int32_t) p->fmtdata.nChannels;
    else skipframes = offset;
    skipsize = p->pvdata.dwFrameAlign * skipframes;

    pos = p->datachunkoffset + skipsize;
    if (UNLIKELY(fseek(p->fp, (int32_t) pos, SEEK_SET) != 0)) {
      csound->pvErrorCode = -41;
      return -1;
    }
    p->curpos = pos;
    p->FramePos = skipframes;

    return 0;
}

/* may be more to do in here later on */

int32_t pvsys_release(CSOUND *csound)
{
    int32_t i;

    csound->pvErrorCode = 0;
    for (i = 0; i < csound->pvNumFiles; i++) {
      if (pvsys_getFileHandle(csound, i) != NULL) {
        if (UNLIKELY(!pvoc_closefile(csound, i))) {
          csound->pvErrorCode = -42;
        }
      }
    }
    if (csound->pvNumFiles) {
      csound->Free(csound, csound->pvFileTable);
      csound->pvFileTable = NULL;
      csound->pvNumFiles = 0;
    }
    return (csound->pvErrorCode == 0 ? 1 : 0);
}

/* return raw framecount: channel-agnostic for now */

int32_t pvoc_framecount(CSOUND *csound, int32_t ifd)
{
    PVOCFILE  *p = pvsys_getFileHandle(csound, ifd);
    if (UNLIKELY(p == NULL)) {
      csound->pvErrorCode = -38;
      return -1;
    }
    return p->nFrames;
}
