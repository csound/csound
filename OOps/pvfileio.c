/*
    pvfileio.c:

    Copyright (C) 2000 Richard Dobson

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

/* pvfileio.c */
/* pvocex format test routines*/
/* Initial version RWD May 2000.
 * All rights reserved: work in progress!
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

#include "cs.h"
#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_FCNTL_H
# include <fcntl.h>
#endif
#ifndef mac_classic
#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
#include <sys/stat.h>
#endif
#ifdef WIN32
#include <io.h>
#endif
#ifdef _DEBUG
#include <assert.h>
#endif

#include "pvfileio.h"

/* NB probably no good on 64bit platforms */
#define REVDWBYTES(t)   ( (((t)&0xff) << 24) | (((t)&0xff00) << 8) | \
                          (((t)&0xff0000) >> 8) | (((t)>>24) & 0xff) )
#define REVWBYTES(t)    ( (((t)&0xff) << 8) | (((t)>>8) &0xff) )
#define TAG(a,b,c,d)    ( ((a)<<24) | ((b)<<16) | ((c)<<8) | (d) )
#define WAVE_FORMAT_EXTENSIBLE (0xFFFE)
#define WAVE_FORMAT_PCM (0x0001)
#define WAVE_FORMAT_IEEE_FLOAT (0x0003)


const GUID KSDATAFORMAT_SUBTYPE_PVOC = {
                                        0x8312b9c2,
                                        0x2e6e,
                                        0x11d4,
                                        { 0xa8, 0x24, 0xde, 0x5b,
                                          0x96, 0xc3, 0xab, 0x21 }
};

#if defined MSVC
#define RD_OPTS  O_RDONLY | O_BINARY,_S_IREAD
#define WR_OPTS  O_TRUNC | O_CREAT | O_WRONLY | O_BINARY,_S_IWRITE
#elif defined(mac_classic) || defined(SYMANTEC) || defined(WIN32)
#define RD_OPTS  O_RDONLY | O_BINARY
#define WR_OPTS  O_TRUNC | O_CREAT | O_WRONLY | O_BINARY
#elif defined DOSGCC
#define RD_OPTS  O_RDONLY | O_BINARY, 0
#define WR_OPTS  O_TRUNC | O_CREAT | O_WRONLY | O_BINARY, 0644
#elif defined LATTICE
#define RD_OPTS  O_RDONLY | O_RAW, 0
#define WR_OPTS  O_TRUNC | O_CREAT | O_WRONLY | O_RAW, 0644
#else
#ifndef O_BINARY
# define O_BINARY (0)
#endif
#define RD_OPTS  O_RDONLY | O_BINARY, 0
#define WR_OPTS  O_TRUNC | O_CREAT | O_WRONLY | O_BINARY, 0644
#endif

static  char *pv_errstr = "";

#define MAXFILES (16)
/* or any desired larger number: will be dynamically allocated one day */


typedef struct pvoc_file {
        WAVEFORMATEX fmtdata;
        PVOCDATA pvdata;
        off_t datachunkoffset;
        long nFrames;   /* no of frames in file */
        long FramePos;  /* where we are in file */
        off_t curpos;
        int fd;
        int to_delete;
        int readonly;
        int do_byte_reverse;
        char *name;
        float *customWindow;
} PVOCFILE;

static PVOCFILE *files[MAXFILES];

static int pvoc_writeheader(int ofd);
static int pvoc_readheader(int ifd,WAVEFORMATPVOCEX *pWfpx);


static int write_guid(int fd,int byterev,const GUID *pGuid)
{
    long written;
#ifdef _DEBUG
    assert(fd >= 0);
    assert(pGuid);
#endif

    if (byterev) {
      GUID guid;
      guid.Data1 = REVDWBYTES(pGuid->Data1);
      guid.Data2 = REVWBYTES(pGuid->Data2);
      guid.Data3 = REVWBYTES(pGuid->Data3);
      memcpy((char *) (guid.Data4),(char *) (pGuid->Data4),8);
      written = write(fd,(char *) &guid,sizeof(GUID));
    }
    else
      written = write(fd,(char *) pGuid,sizeof(GUID));

    return written;

}

static int compare_guids(const GUID *gleft, const GUID *gright)
{
    const char *left = (const char *) gleft, *right = (const char *) gright;
    return !memcmp(left,right,sizeof(GUID));
}


static int write_pvocdata(int fd,int byterev,const PVOCDATA *pData)
{
    long written;
    long dwval;
#ifdef _DEBUG
    assert(fd >= 0);
    assert(pData);
#endif


    if (byterev) {
      PVOCDATA data;
      data.wWordFormat   = REVWBYTES(pData->wWordFormat);
      data.wAnalFormat   = REVWBYTES(pData->wAnalFormat);
      data.wSourceFormat = REVWBYTES(pData->wSourceFormat);
      data.wWindowType   = REVWBYTES(pData->wWindowType);
      data.nAnalysisBins = REVDWBYTES(pData->nAnalysisBins);
      data.dwWinlen      = REVDWBYTES(pData->dwWinlen);
      data.dwOverlap     = REVDWBYTES(pData->dwOverlap);
      data.dwFrameAlign  = REVDWBYTES(pData->dwFrameAlign);

      dwval = * (long *) &(pData->fAnalysisRate);
      dwval = REVDWBYTES(dwval);
      data.fAnalysisRate = * (float *) &dwval;

      dwval = * (long *) &(pData->fWindowParam);
      dwval = REVDWBYTES(dwval);
      data.fWindowParam = * (float *) &dwval;

      written = write(fd,(void *) &data,sizeof(PVOCDATA));
    }
    else
      written = write(fd,(void *) pData,sizeof(PVOCDATA));

    return written;

}

static int write_fmt(int fd, int byterev,const WAVEFORMATEX *pfmt)
{
    /*NB have to write out each element, as not guaranteed alignment othewise.
     *  Consider it documentation. */

#ifdef _DEBUG
    assert(fd >=0);
    assert(pfmt);
#endif

    if (byterev) {
      WAVEFORMATEX fmt;
      fmt.wFormatTag            = REVWBYTES(pfmt->wFormatTag);
      fmt.nChannels             = REVWBYTES(pfmt->nChannels);
      fmt.nSamplesPerSec        = REVDWBYTES(pfmt->nSamplesPerSec);
      fmt.nAvgBytesPerSec       = REVDWBYTES(pfmt->nAvgBytesPerSec);
      fmt.nBlockAlign           = REVWBYTES(pfmt->nBlockAlign);
      fmt.wBitsPerSample        = REVWBYTES(pfmt->wBitsPerSample);
      fmt.cbSize                = REVWBYTES(pfmt->cbSize);

      if (write(fd,(char *) &(fmt.wFormatTag),sizeof(WORD)) != sizeof(WORD)
          || write(fd,(char *) &(fmt.nChannels),sizeof(WORD)) != sizeof(WORD)
          || write(fd,(char *) &(fmt.nSamplesPerSec),
                   sizeof(DWORD)) != sizeof(DWORD)
          || write(fd,(char *) &(fmt.nAvgBytesPerSec),
                   sizeof(DWORD)) != sizeof(DWORD)
          || write(fd,(char *) &(fmt.nBlockAlign),sizeof(WORD)) != sizeof(WORD)
          || write(fd,(char *) &(fmt.wBitsPerSample),sizeof(WORD)) != sizeof(WORD)
          || write(fd,(char *) &(fmt.cbSize),sizeof(WORD)) != sizeof(WORD))

        return 0;

    }
    else {
      if (write(fd,(char *) &(pfmt->wFormatTag),sizeof(WORD)) != sizeof(WORD)
          || write(fd,(char *) &(pfmt->nChannels),sizeof(WORD)) != sizeof(WORD)
          || write(fd,(char *) &(pfmt->nSamplesPerSec),
                   sizeof(DWORD)) != sizeof(DWORD)
          || write(fd,(char *) &(pfmt->nAvgBytesPerSec),
                   sizeof(DWORD)) != sizeof(DWORD)
          || write(fd,(char *) &(pfmt->nBlockAlign),sizeof(WORD)) != sizeof(WORD)
          || write(fd,(char *) &(pfmt->wBitsPerSample),
                   sizeof(WORD)) != sizeof(WORD)
          || write(fd,(char *) &(pfmt->cbSize),sizeof(WORD)) != sizeof(WORD))

        return 0;

    }

    return SIZEOF_WFMTEX;

}

static int pvoc_writeWindow(int fd,int byterev,float *window,DWORD length)
{
    if (byterev) {
      /* don't corrupt source array! */
      DWORD i;
      long lval, *lp = (long *) window;

      for (i=0;i < length; i++) {
        lval = *lp++;
        lval = REVDWBYTES(lval);
        if (write(fd,(char *)&lval,sizeof(long)) != sizeof(long))
          return 0;
      }
    }
    else {
      size_t n = length*sizeof(float);
      if (write(fd,(char *)window,n) != n)
        return 0;
    }


    return length * sizeof(float);
}

static int pvoc_readWindow(int fd, int byterev, float *window,DWORD length)
{
    if (byterev) {
      DWORD i;
      long lval, *lp = (long *) window;
      for (i=0;i < length;i++) {
        if (read(fd,(char *)&lval,sizeof(long)) != sizeof(long))
          return 0;
        lval = REVDWBYTES(lval);
        *lp++ = lval;
      }
    }
    else {
      size_t n = length*sizeof(float);
      if (read(fd,(char *)window, n) != n)
        return 0;
    }

    return length * sizeof(float);

}

const char *pvoc_errorstr(void)
{
    return (const char *) pv_errstr;
}

/* thanks to the SNDAN programmers for this! */
/* return 0 for big-endian machine, 1 for little-endian machine*/
/* probably no good for 16bit swapping though */
static int byte_order(void)
{
    int   one = 1;
    char* endptr = (char *) &one;
    return (*endptr);
}

/***** loosely modelled on CDP sfsys ******
 *      This is a static array, but could be made dynamic in an OOP sort of way.
 *      The idea is that all low-level operations and data
 *      are completely hidden from the user, so that internal
 *      format changes can be made with little or no disruption
 *      to the public functions.
 * But avoiding the full monty of a C++ implementation.
 *******************************************/

int init_pvsys(void)
{
    int i;

    if (files[0] != NULL) {
      pv_errstr = Str(X_1614,"\npvsys: already imnitialised");
      return 0;
    }
    for (i = 0;i < MAXFILES;i++)
      files[i] = NULL;

    return 1;
}

static void prepare_pvfmt(WAVEFORMATEX *pfmt,unsigned long chans,
                          unsigned long srate, pv_stype stype)
{

#ifdef _DEBUG
    assert(pfmt);
#endif

    pfmt->wFormatTag            = WAVE_FORMAT_EXTENSIBLE;
    pfmt->nChannels             = (WORD) chans;
    pfmt->nSamplesPerSec        = srate;
    switch(stype) {
    case(STYPE_16):
      pfmt->wBitsPerSample = (WORD)16;
      pfmt->nBlockAlign  = (WORD)(chans * 2 *  sizeof(char));
      break;
    case(STYPE_24):
      pfmt->wBitsPerSample = (WORD) 24;
      pfmt->nBlockAlign  = (WORD)(chans *  3 * sizeof(char));
      break;
    case(STYPE_32):
    case(STYPE_IEEE_FLOAT):
      pfmt->wBitsPerSample = (WORD) 32;
      pfmt->nBlockAlign  = (WORD)(chans *  4 * sizeof(char));
      break;
    default:
#ifdef _DEBUG
      assert(0);
#endif
      break;
    }

    pfmt->nAvgBytesPerSec       = pfmt->nBlockAlign * srate;
    /* if we have extended WindowParam fields, or something,
       will need to adjust this */
    pfmt->cbSize                = 62;

}


/* lots of different ways of doing this! */
/* we will need  one in the form:
 * in pvoc_fmtcreate(const char *fname,PVOCDATA *p_pvfmt, WAVEFORMATEX *p_wvfmt);
 */

/* a simple minimalist function to begin with!*/
/*set D to 0, and/or dwWinlen to 0, to use internal default */
/* fWindow points to userdef window, or is NULL */
/* NB currently this does not enforce a soundfile extension; */
/* probably it should... */
int  pvoc_createfile(const char *filename,
                     DWORD fftlen,DWORD overlap,DWORD chans,
                     DWORD format,long srate,
                     pv_stype stype,pv_wtype wtype,
                     float wparam,float *fWindow,DWORD dwWinlen)
{

    int i;
    long N,D;
    char *pname;
    PVOCFILE *pfile = NULL;
    float winparam = 0.0f;

    N = fftlen;                               /* keep the CARL varnames for now */
    D = overlap;

    if (N == 0 || chans <=0 || filename==NULL || D > N) {
      pv_errstr = Str(X_1615,"\npvsys: bad arguments");
      return -1;
    }
    if (format < PVOC_AMP_FREQ || format > PVOC_COMPLEX) {
      pv_errstr = Str(X_1616,"\npvsys: bad format parameter");
      return -1;
    }

    if (!(wtype >= PVOC_DEFAULT && wtype <= PVOC_CUSTOM)) {
      pv_errstr = Str(X_1617,"\npvsys: bad window type");
      return -1;
    }

    /* load it, but can not write until we have a PVXW chunk definition...*/
    if (wtype==PVOC_CUSTOM) {

    }

    if (wtype==PVOC_DEFAULT)
      wtype = PVOC_HAMMING;

    if (wtype==PVOC_KAISER)
      if (wparam != 0.0f)
        winparam = wparam;
    /*will need an internal default for window parameters...*/

    for (i=0;i < MAXFILES;i++)
      if (files[i]==NULL)
        break;
    if (i==MAXFILES) {
      pv_errstr = Str(X_1618,"\npvsys: too many files open");
      return -1;
    }
    pfile = (PVOCFILE *) malloc(sizeof(PVOCFILE));

    if (pfile==NULL) {
      pv_errstr = Str(X_1619,"\npvsys: no memory");
      return -1;
    }
    pname = (char *) malloc(strlen(filename)+1);
    if (pname == NULL) {
      free(pfile);
      pv_errstr = Str(X_1619,"\npvsys: no memory");
      return -1;
    }
    pfile->customWindow = NULL;
    /* setup rendering inforamtion */
    prepare_pvfmt(&pfile->fmtdata,chans,srate,stype);

    strcpy(pname,filename);

    pfile->pvdata.wWordFormat = PVOC_IEEE_FLOAT;
    pfile->pvdata.wAnalFormat = (WORD) format;
    pfile->pvdata.wSourceFormat =
      (stype == STYPE_IEEE_FLOAT ? WAVE_FORMAT_IEEE_FLOAT : WAVE_FORMAT_PCM);
    pfile->pvdata.wWindowType = wtype;
    pfile->pvdata.nAnalysisBins = (N>>1) + 1;
    if (dwWinlen==0)
      pfile->pvdata.dwWinlen            = N;
    else
      pfile->pvdata.dwWinlen    = dwWinlen;
    if (D==0)
      pfile->pvdata.dwOverlap   = N/8;
    else
      pfile->pvdata.dwOverlap = D;
    pfile->pvdata.dwFrameAlign = pfile->pvdata.nAnalysisBins * 2 * sizeof(float);
    pfile->pvdata.fAnalysisRate = (float)srate / (float) pfile->pvdata.dwOverlap;
    pfile->pvdata.fWindowParam = winparam;
    pfile->to_delete = 0;
    pfile->readonly = 0;
    if (fWindow!= NULL) {
      pfile->customWindow = malloc(dwWinlen * sizeof(float));
      if (pfile->customWindow==NULL) {
        pv_errstr = Str(X_1620,"\npvsys: no memory for custom window");
        return -1;
      }
      memcpy((void *)(pfile->customWindow),
             (void *)fWindow, dwWinlen * sizeof(float));
    }


    pfile->fd = open(filename,WR_OPTS);
    if (pfile->fd < 0) {
      free(pname);
      if (pfile->customWindow)
        free(pfile->customWindow);
      free(pfile);

      pv_errstr = Str(X_1621,"\npvsys: unable to create file");
      return -1;
    }

    pfile->datachunkoffset = 0;
    pfile->nFrames = 0;
    pfile->FramePos = 0;
    pfile->curpos = 0;
    pfile->name = pname;
    pfile->do_byte_reverse = !byte_order();
    files[i] = pfile;

    if (!pvoc_writeheader(i)) {
      close(pfile->fd);
      remove(pfile->name);
      free(pfile->name);
      if (pfile->customWindow)
        free(pfile->customWindow);
      free(pfile);
      files[i] = NULL;
      return -1;
    }

    return i;
}

int pvoc_openfile(const char *filename,PVOCDATA *data,WAVEFORMATEX *fmt)
{
    int i;
    WAVEFORMATPVOCEX wfpx;
    char *pname;
    PVOCFILE *pfile = NULL;

    if (data==NULL || fmt==NULL) {
      pv_errstr = Str(X_1622,"\npvsys: Internal error: NULL data arrays");
      return -1;
    }

    for (i=0;i < MAXFILES;i++)
      if (files[i]==NULL)
        break;
    if (i==MAXFILES) {
      pv_errstr = Str(X_1618,"\npvsys: too many files open");
      return -1;
    }

    pfile = (PVOCFILE *) malloc(sizeof(PVOCFILE));
    if (pfile==NULL) {
      pv_errstr = Str(X_1623,"\npvsys: no memory for file data");
      return -1;
    }
    pfile->customWindow = NULL;
    pname = (char *) malloc(strlen(filename)+1);
    if (pname == NULL) {
      free(pfile);
      pv_errstr = Str(X_1619,"\npvsys: no memory");
      return -1;
    }
    pfile->fd = open(filename,RD_OPTS);
    if (pfile->fd < 0) {
      if (!isfullpath((char*)filename) && sadirpath != NULL) {
        char *name = catpath(sadirpath, (char*)filename);
        free(pname);
        pname = (char *) realloc(pname, strlen(name)+1);
        if (pname == NULL) {
          free(pfile);
          pv_errstr = Str(X_1619,"\npvsys: no memory");
          return -1;
        }
        strcpy(pname,name);
        pfile->fd = open(name,RD_OPTS);
      }
      if (pfile->fd < 0) {
        free(pname);
        free(pfile);
        pv_errstr = Str(X_1624,"\npvsys: unable to open file");
        return -1;
      }
    }
    strcpy(pname,filename);
    pfile->datachunkoffset = 0;
    pfile->nFrames = 0;
    pfile->curpos = 0;
    pfile->FramePos = 0;
    pfile->name = pname;
    pfile->do_byte_reverse = !byte_order();
    pfile->readonly = 1;
    files[i] = pfile;

    if (!pvoc_readheader(i,&wfpx)) {
      close(pfile->fd);
      free(pfile->name);
      if (pfile->customWindow)
        free(pfile->customWindow);
      free(pfile);
      files[i] = NULL;
      return -1;
    }

    memcpy((char *)data, (char *)&(wfpx.data),sizeof(PVOCDATA));
    memcpy((char *)fmt,(char *)&(wfpx.wxFormat.Format),SIZEOF_WFMTEX);

    files[i] = pfile;

    return i;

}
/*RWD TODO: add byterev stuff*/
static int pvoc_readfmt(int fd,int byterev,WAVEFORMATPVOCEX *pWfpx)
{
    unsigned long dword;
    unsigned short word;

#ifdef _DEBUG
    assert(fd >= 0);
    assert(pWfpx);
#endif

    if (read(fd,(char *) &(pWfpx->wxFormat.Format.wFormatTag),
             sizeof(WORD)) != sizeof(WORD)
        || read(fd,(char *) &(pWfpx->wxFormat.Format.nChannels),
                sizeof(WORD)) != sizeof(WORD)
        || read(fd,(char *) &(pWfpx->wxFormat.Format.nSamplesPerSec),
                sizeof(DWORD)) != sizeof(DWORD)
        || read(fd,(char *) &(pWfpx->wxFormat.Format.nAvgBytesPerSec),
                sizeof(DWORD)) != sizeof(DWORD)
        || read(fd,(char *) &(pWfpx->wxFormat.Format.nBlockAlign),
                sizeof(WORD)) != sizeof(WORD)
        || read(fd,(char *) &(pWfpx->wxFormat.Format.wBitsPerSample),
                sizeof(WORD)) != sizeof(WORD)
        || read(fd,(char *) &(pWfpx->wxFormat.Format.cbSize),
                sizeof(WORD)) != sizeof(WORD)) {
      pv_errstr = Str(X_1625,"\npvsys: error reading Source format data");
      return 0;
    }

    if (byterev) {
      word = pWfpx->wxFormat.Format.wFormatTag;
      pWfpx->wxFormat.Format.wFormatTag= REVWBYTES(word);
      word = pWfpx->wxFormat.Format.nChannels;
      pWfpx->wxFormat.Format.nChannels = REVWBYTES(word);
      dword =   pWfpx->wxFormat.Format.nSamplesPerSec;
      pWfpx->wxFormat.Format.nSamplesPerSec = REVDWBYTES(dword);
      dword = pWfpx->wxFormat.Format.nAvgBytesPerSec;
      pWfpx->wxFormat.Format.nAvgBytesPerSec = REVDWBYTES(dword);
      word = pWfpx->wxFormat.Format.nBlockAlign;
      pWfpx->wxFormat.Format.nBlockAlign = REVWBYTES(word);
      word = pWfpx->wxFormat.Format.wBitsPerSample;
      pWfpx->wxFormat.Format.wBitsPerSample = REVWBYTES(word);
      word = pWfpx->wxFormat.Format.cbSize;
      pWfpx->wxFormat.Format.cbSize = REVWBYTES(word);

    }

    /* the first clues this is pvx format...*/
    if (pWfpx->wxFormat.Format.wFormatTag != WAVE_FORMAT_EXTENSIBLE) {
      pv_errstr = Str(X_1626,"\npvsys: not a WAVE_EX file");
      return 0;
    }

    if (pWfpx->wxFormat.Format.cbSize != 62) {
      pv_errstr = Str(X_1627,"\npvsys: bad size for fmt chunk");
      return 0;
    }

    if (read(fd,(char *)&(pWfpx->wxFormat.Samples.wValidBitsPerSample),
             sizeof(WORD)) != sizeof(WORD)
        || read(fd,(char *)&(pWfpx->wxFormat.dwChannelMask),
                sizeof(DWORD)) != sizeof(DWORD)
        || read(fd,(char *)&(pWfpx->wxFormat.SubFormat),
                sizeof(GUID)) != sizeof(GUID)) {
      pv_errstr = Str(X_1628,"\npvsys: error reading Extended format data");
      return 0;
    }

    if (byterev) {
      word = pWfpx->wxFormat.Samples.wValidBitsPerSample;
      pWfpx->wxFormat.Samples.wValidBitsPerSample = REVWBYTES(word);
      dword = pWfpx->wxFormat.dwChannelMask;
      pWfpx->wxFormat.dwChannelMask = REVDWBYTES(dword);

      dword = pWfpx->wxFormat.SubFormat.Data1;
      pWfpx->wxFormat.SubFormat.Data1 = REVDWBYTES(dword);
      word = pWfpx->wxFormat.SubFormat.Data2;
      pWfpx->wxFormat.SubFormat.Data2 = REVWBYTES(word);
      word = pWfpx->wxFormat.SubFormat.Data3;
      pWfpx->wxFormat.SubFormat.Data3 = REVWBYTES(word);
      /* don't need to reverse the char array */
    }

    /* ... but this is the clincher */
    if (!compare_guids(&(pWfpx->wxFormat.SubFormat),&KSDATAFORMAT_SUBTYPE_PVOC)) {
      pv_errstr = Str(X_1629,"\npvsys: not a PVOC-EX file");
      return 0;
    }

    if (read(fd,(char *) &(pWfpx->dwVersion),sizeof(DWORD)) != sizeof(DWORD)
        || read(fd,(char *) &(pWfpx->dwDataSize),sizeof(DWORD)) != sizeof(DWORD)
        || read(fd,(char *) &(pWfpx->data),sizeof(PVOCDATA)) != sizeof(PVOCDATA)) {
      pv_errstr = Str(X_1630,"\npvsys: error reading Extended pvoc format data");
      return 0;
    }

    if (byterev) {
      dword = pWfpx->dwVersion;
      pWfpx->dwVersion = REVDWBYTES(dword);

      /* check it now! */
      if (pWfpx->dwVersion != PVX_VERSION) {
        pv_errstr = Str(X_1631,"\npvsys: unknown pvocex Version");
        return 0;
      }

      dword = pWfpx->dwDataSize;
      pWfpx->dwDataSize = REVDWBYTES(dword);

      word = pWfpx->data.wWordFormat;
      pWfpx->data.wWordFormat= REVWBYTES(word);
      word = pWfpx->data.wAnalFormat;
      pWfpx->data.wAnalFormat= REVWBYTES(word);
      word = pWfpx->data.wSourceFormat;
      pWfpx->data.wSourceFormat= REVWBYTES(word);
      word = pWfpx->data.wWindowType;
      pWfpx->data.wWindowType= REVWBYTES(word);

      dword = pWfpx->data.nAnalysisBins;
      pWfpx->data.nAnalysisBins = REVDWBYTES(dword);
      dword = pWfpx->data.dwWinlen;
      pWfpx->data.dwWinlen = REVDWBYTES(dword);
      dword = pWfpx->data.dwOverlap;
      pWfpx->data.dwOverlap = REVDWBYTES(dword);
      dword = pWfpx->data.dwFrameAlign;
      pWfpx->data.dwFrameAlign = REVDWBYTES(dword);

      dword = * (DWORD *)&(pWfpx->data.fAnalysisRate);
      dword = REVDWBYTES(dword);
      pWfpx->data.fAnalysisRate = *(float *)&dword;
      dword = * (DWORD *)&(pWfpx->data.fWindowParam);
      dword = REVDWBYTES(dword);
      pWfpx->data.fWindowParam = *(float *)&dword;

    }
    if (pWfpx->dwVersion != PVX_VERSION) {
      pv_errstr = Str(X_1631,"\npvsys: unknown pvocex Version");
      return 0;
    }

    return 1;
}


static int pvoc_readheader(int ifd,WAVEFORMATPVOCEX *pWfpx)
{
    DWORD tag, size,riffsize;
    int fmtseen = 0, windowseen = 0;

#ifdef _DEBUG
    assert(pWfpx);
    assert(files[ifd]);
    assert(files[ifd]->fd >= 0);
    size = sizeof(WAVEFORMATEXTENSIBLE);
    size += 2 * sizeof(DWORD);
    size += sizeof(PVOCDATA);
#endif

    if (read(files[ifd]->fd,(char *) &tag,sizeof(DWORD)) != sizeof(DWORD)
        || read(files[ifd]->fd,(char *) &size,sizeof(DWORD)) != sizeof(DWORD)) {
      pv_errstr = Str(X_1632,"\npvsys: error reading header");
      return 0;
    }
    if (files[ifd]->do_byte_reverse)
      size = REVDWBYTES(size);
    else
      tag = REVDWBYTES(tag);

    if (tag != TAG('R','I','F','F')) {
      pv_errstr = Str(X_1633,"\npvsys: not a RIFF file");
      return 0;
    }
    if (size < 24 * sizeof(DWORD) + SIZEOF_FMTPVOCEX) {
      pv_errstr = Str(X_1634,"\npvsys: file too small");
      return 0;
    }
    riffsize = size;
    if (read(files[ifd]->fd,(char *) &tag,sizeof(DWORD)) != sizeof(DWORD)) {
      pv_errstr = Str(X_1555,"\npvsys: error reading header");
      return 0;
    }

    if (!files[ifd]->do_byte_reverse)
      tag = REVDWBYTES(tag);

    if (tag != TAG('W','A','V','E')) {
      pv_errstr = Str(X_1636,"\npvsys: not a WAVE file");
      return 0;
    }
    riffsize -= sizeof(DWORD);
    /*loop for chunks */
    while (riffsize > 0) {
      if (read(files[ifd]->fd,(char *) &tag,sizeof(DWORD)) != sizeof(DWORD)
          || read(files[ifd]->fd,(char *) &size,sizeof(DWORD)) != sizeof(DWORD)) {
        pv_errstr = Str(X_1632,"\npvsys: error reading header");
        return 0;
      }
      if (files[ifd]->do_byte_reverse)
        size = REVDWBYTES(size);
      else
        tag = REVDWBYTES(tag);
      riffsize -= 2 * sizeof(DWORD);
      switch (tag) {
      case TAG('f','m','t',' '):
        /* bail out if not a pvoc file: not trying to read all WAVE formats!*/
        if (size < SIZEOF_FMTPVOCEX) {
          pv_errstr = Str(X_1629,"\npvsys: not a PVOC-EX file");
          return 0;
        }
        if (!pvoc_readfmt(files[ifd]->fd,files[ifd]->do_byte_reverse,pWfpx)) {
          pv_errstr = Str(X_1637,"\npvsys: error reading format chunk");
          return 0;
        }
        riffsize -=  SIZEOF_FMTPVOCEX;
        fmtseen = 1;
        memcpy((char *)&(files[ifd]->fmtdata),
               (char *)&(pWfpx->wxFormat),SIZEOF_WFMTEX);
        memcpy((char *)&(files[ifd]->pvdata),
               (char *)&(pWfpx->data),sizeof(PVOCDATA));
        break;
      case TAG('P','V','X','W'):
        if (!fmtseen) {
          pv_errstr = Str(X_1638,"\npvsys: PVXW chunk found before fmt chunk.");
          return 0;
        }
        if (files[ifd]->pvdata.wWindowType!=PVOC_CUSTOM) {
          /*whaddayado? can you warn the user and continue?*/
          pv_errstr = Str(X_1639,
                          "\npvsys: PVXW chunk found but custom "
                          "window not specified");
          return 0;
        }
        files[ifd]->customWindow =
          malloc(files[ifd]->pvdata.dwWinlen*sizeof(float));
        if (files[ifd]->customWindow == NULL) {
          pv_errstr = Str(X_1640,"\npvsys: no memory for custom window data.");
          return 0;
        }
        if (pvoc_readWindow(files[ifd]->fd,files[ifd]->do_byte_reverse,
                            files[ifd]->customWindow,files[ifd]->pvdata.dwWinlen)
            != (int)(files[ifd]->pvdata.dwWinlen * sizeof(float))) {
          pv_errstr = Str(X_1641,"\npvsys: error reading window data.");
          return 0;
        }
        windowseen = 1;
        break;
      case TAG('d','a','t','a'):
        if (riffsize - size  != 0) {
          pv_errstr = Str(X_1642,"\npvsys: bad RIFF file");
          return 0;
        }

        if (!fmtseen) {
          pv_errstr = Str(X_1643,
                          "\npvsys: bad format, data chunk before fmt chunk");
          return 0;
        }

        if (files[ifd]->pvdata.wWindowType==PVOC_CUSTOM)
          if (!windowseen) {
            pv_errstr = Str(X_1644,"\npvsys: custom window chunk PVXW not found");
            return 0;
          }

        files[ifd]->datachunkoffset = lseek(files[ifd]->fd,(off_t)0,SEEK_CUR);
        files[ifd]->curpos = files[ifd]->datachunkoffset;
        /* not m/c frames, for now */
        files[ifd]->nFrames = size / files[ifd]->pvdata.dwFrameAlign;
        return 1;
        break;
      default:
        /* skip any onknown chunks */
        riffsize -= 2 * sizeof(DWORD);
        if (lseek(files[ifd]->fd,(off_t)size,SEEK_CUR) < 0) {
          pv_errstr = Str(X_1645,"\npvsys: error skipping unknown WAVE chunk");
          return 0;
        }
        riffsize -= size;
        break;
      }

    }
    /* if here, something very wrong!*/
    pv_errstr = Str(X_1646,"\npvsys: bad format in RIFF file");
    return 0;

}

static int pvoc_writeheader(int ofd)
{
    long tag,size,version;
    WORD validbits;

    const GUID *pGuid =  &KSDATAFORMAT_SUBTYPE_PVOC;

#ifdef _DEBUG
    assert(files[ofd] != NULL);
    assert(files[ofd]->fd >=0);
#endif

    tag = TAG('R','I','F','F');
    size = 0;
    if (files[ofd]->do_byte_reverse)
      size = REVDWBYTES(size);
    if (!files[ofd]->do_byte_reverse)
      tag = REVDWBYTES(tag);

    if (write(files[ofd]->fd,(char*)&tag,sizeof(long)) != sizeof(long)
        || write(files[ofd]->fd,(char*)&size,sizeof(long)) != sizeof(long)) {
      pv_errstr = Str(X_1647,"\npvsys: error writing header");
      return 0;
    }

    tag = TAG('W','A','V','E');
    if (!files[ofd]->do_byte_reverse)
      tag = REVDWBYTES(tag);
    if (write(files[ofd]->fd,(char*)&tag,sizeof(long)) != sizeof(long)) {
      pv_errstr = Str(X_1647,"\npvsys: error writing header");
      return 0;
    }

    tag = TAG('f','m','t',' ');
    size =      SIZEOF_WFMTEX + sizeof(WORD) +
      sizeof(DWORD)
      + sizeof(GUID)
      + 2*sizeof(DWORD)
      + sizeof(PVOCDATA);
    if (files[ofd]->do_byte_reverse)
      size = REVDWBYTES(size);
    if (!files[ofd]->do_byte_reverse)
      tag = REVDWBYTES(tag);
    if (write(files[ofd]->fd,(char *)&tag,sizeof(long)) != sizeof(long)
        || write(files[ofd]->fd,(char *)&size,sizeof(long)) != sizeof(long)) {
      pv_errstr = Str(X_1647,"\npvsys: error writing header");
      return 0;
    }

    if (write_fmt(files[ofd]->fd,
                  files[ofd]->do_byte_reverse,
                  &(files[ofd]->fmtdata)) != SIZEOF_WFMTEX) {
      pv_errstr = Str(X_1648,"\npvsys: error writing fmt chunk");
      return 0;
    }

    validbits = files[ofd]->fmtdata.wBitsPerSample;      /*nothing fancy here */
    if (files[ofd]->do_byte_reverse)
      validbits = REVWBYTES(validbits);

    if (write(files[ofd]->fd,(char *) &validbits,sizeof(WORD)) != sizeof(WORD)) {
      pv_errstr = Str(X_1648,"\npvsys: error writing fmt chunk");
      return 0;
    }
    /* we will take this from a WAVE_EX file, in due course */
    size = 0;   /*dwChannelMask*/
    if (write(files[ofd]->fd,(char *)&size,sizeof(long)) != sizeof(DWORD)) {
      pv_errstr = Str(X_1648,"\npvsys: error writing fmt chunk");
      return 0;
    }

    if (write_guid(files[ofd]->fd,
                   files[ofd]->do_byte_reverse,
                   pGuid) != sizeof(GUID)) {
      pv_errstr = Str(X_1648,"\npvsys: error writing fmt chunk");
      return 0;
    }
    version  = 1;
    size = sizeof(PVOCDATA);
    if (files[ofd]->do_byte_reverse) {
      version = REVDWBYTES(version);
      size = REVDWBYTES(size);
    }

    if (write(files[ofd]->fd,(char*)&version,sizeof(long)) != sizeof(long)
        || write(files[ofd]->fd,(char*)&size,sizeof(long)) != sizeof(long)) {
      pv_errstr = Str(X_1648,"\npvsys: error writing fmt chunk");
      return 0;
    }


    if (write_pvocdata(files[ofd]->fd,
                       files[ofd]->do_byte_reverse,
                       &(files[ofd]->pvdata)) != sizeof(PVOCDATA)) {
      pv_errstr = Str(X_1648,"\npvsys: error writing fmt chunk");
      return 0;

    }

    /* VERY experimental; may not even be a good idea...*/

    if (files[ofd]->customWindow) {
      tag = TAG('P','V','X','W');
      size = files[ofd]->pvdata.dwWinlen * sizeof(float);
      if (files[ofd]->do_byte_reverse)
        size = REVDWBYTES(size);
      else
        tag = REVDWBYTES(tag);
      if (write(files[ofd]->fd,(char *)&tag,sizeof(long)) != sizeof(long)
          || write(files[ofd]->fd,(char *)&size,sizeof(long)) != sizeof(long)) {
        pv_errstr = Str(X_1649,"\npvsys: error writing header");
        return 0;
      }
      if (pvoc_writeWindow(files[ofd]->fd,
                           files[ofd]->do_byte_reverse,
                           files[ofd]->customWindow,
                           files[ofd]->pvdata.dwWinlen) !=
          (int)(files[ofd]->pvdata.dwWinlen * sizeof(float))) {
        pv_errstr = Str(X_1650,"\npvsys: error writing window data.");
        return 0;
      }
    }

    /* no other chunks to write yet! */
    tag = TAG('d','a','t','a');
    if (!files[ofd]->do_byte_reverse)
      tag = REVDWBYTES(tag);
    if (write(files[ofd]->fd,(char*)&tag,sizeof(long)) != sizeof(long)) {
      pv_errstr = Str(X_1649,"\npvsys: error writing header");
      return 0;
    }

    /* we need to update size later on...*/

    size = 0;
    if (write(files[ofd]->fd,(char*)&size,sizeof(long)) != sizeof(long)) {
      pv_errstr = Str(X_1649,"\npvsys: error writing header");
      return 0;
    }
    files[ofd]->datachunkoffset = lseek(files[ofd]->fd,(off_t)0,SEEK_CUR);

    files[ofd]->curpos = files[ofd]->datachunkoffset;
    return 1;
}


static int pvoc_updateheader(int ofd)
{
    long riffsize,datasize;
    unsigned long pos;

#ifdef _DEBUG
    assert(files[ofd]);
    assert(files[ofd]->fd >= 0);
    assert(files[ofd]->curpos == lseek(files[ofd]->fd,(off_t)0,SEEK_CUR));
#endif

    datasize = files[ofd]->curpos - files[ofd]->datachunkoffset;
    pos = lseek(files[ofd]->fd,
                (off_t)(files[ofd]->datachunkoffset-sizeof(DWORD)),
                SEEK_SET);
    if (pos != (unsigned long)files[ofd]->datachunkoffset-sizeof(DWORD)) {
      pv_errstr = Str(X_1651,"\npvsys: error updating data chunk");
      return 0;
    }

    if (files[ofd]->do_byte_reverse)
      datasize = REVDWBYTES(datasize);
    if (write(files[ofd]->fd,(char *) &datasize,sizeof(DWORD)) != sizeof(DWORD)) {
      pv_errstr = Str(X_1651,"\npvsys: error updating data chunk");
      return 0;
    }

    riffsize = files[ofd]->curpos - 2* sizeof(DWORD);
    if (files[ofd]->do_byte_reverse)
      riffsize = REVDWBYTES(riffsize);
    pos = lseek(files[ofd]->fd,(off_t)sizeof(DWORD),SEEK_SET);
    if (pos != sizeof(DWORD)) {
      pv_errstr = Str(X_1651,"\npvsys: error updating data chunk");
      return 0;
    }
    if (write(files[ofd]->fd,(char *) &riffsize,sizeof(DWORD)) != sizeof(DWORD)) {
      pv_errstr = Str(X_1652,"\npvsys: error updating riff chunk");
      return 0;
    }

    pos = lseek(files[ofd]->fd,(off_t)0,SEEK_END);
    if (pos < 0) {
      pv_errstr = Str(X_1653,"\npvsys: error seeking to end of file");
      return 0;
    }
    return 1;
}

int pvoc_closefile(int ofd)
{
    if (files[ofd]==NULL) {
      pv_errstr = Str(X_1654,"\npvsys: file does not exist");
      return 0;
    }

    if (files[ofd]->fd < 0) {
      pv_errstr = Str(X_1655,"\npvsys: file not open");
      return 0;
    }
    if (!files[ofd]->readonly)
      if (!pvoc_updateheader(ofd))
        return 0;

    close(files[ofd]->fd);
    if (files[ofd]->to_delete && !(files[ofd]->readonly))
      remove(files[ofd]->name);

    free(files[ofd]->name);
    free(files[ofd]);
    files[ofd] = NULL;

    return 1;
}

/* does not directly address m/c streams, or alternative numeric formats, yet....
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
int pvoc_putframes(int ofd,const float *frame,long numframes)
{
    DWORD i;
    DWORD towrite;  /* count in 'words' */
    long temp,*lfp;


    if (files[ofd]==NULL) {
      pv_errstr = Str(X_1656,"\npvsys: bad file descriptor");
      return 0;
    }
    if (files[ofd]->fd < 0) {
      pv_errstr = Str(X_1527,"\npvsys: file not open");
      return 0;
    }
    /* NB doubles not supported yet */

    towrite = files[ofd]->pvdata.nAnalysisBins * 2 * numframes;

    if (files[ofd]->do_byte_reverse) {
      /* do this without overwriting source data! */
      lfp = (long *) frame;
      for (i=0;i < towrite; i++) {
        temp = *lfp++;
        temp = REVDWBYTES(temp);
        if (write(files[ofd]->fd,(char *) &temp,sizeof(long)) != sizeof(long)) {
          pv_errstr = Str(X_1657,"\npvsys: error writing data");
          return 0;
        }
      }
    }
    else {
      size_t n = towrite * sizeof(float);
      if (write(files[ofd]->fd,(char *) frame, n) != (int)n) {
        pv_errstr = Str(X_1657,"\npvsys: error writing data");
        return 0;
      }

    }

    files[ofd]->FramePos += numframes;
    files[ofd]->curpos += towrite * sizeof(float);
    return 1;
}

/* Simplistic read function
 * best practice here is to read nChannels frames *
 * return -1 for error, 0 for EOF, else numframes read
 */
int pvoc_getframes(int ifd, float *frames, unsigned long nframes)
{
    long i;
    long toread;
    long temp,*lfp;
    long got;
    int rc = -1;
    if (files[ifd]==NULL) {
      pv_errstr = Str(X_1658,"\npvsys: bad file descriptor");
      return rc;
    }
    if (files[ifd]->fd < 0) {
      pv_errstr = Str(X_1655,"\npvsys: file not open");
      return rc;
    }

    toread = files[ifd]->pvdata.nAnalysisBins * 2 * nframes;

    if (files[ifd]->do_byte_reverse) {
      lfp = (long *) frames;
      for (i=0;i < toread;i++) {
        if ((got=read(files[ifd]->fd,(char *) &temp,sizeof(long))) <0) {
          pv_errstr = Str(X_1635,"\npvsys: error reading data");
          return rc;
        }
        if (got < (long) sizeof(long)) {
          /* file size incorrect? */
          return 0;                     /* assume EOF */
        }
        temp = REVDWBYTES(temp);
        *lfp++ = temp;
      }
       rc =  nframes;
    }
    else {
      size_t n = toread * sizeof(float);
      if ((got = read(files[ifd]->fd, (char *)frames, n)) < (int)n) {
        if (got < 0) {
          pv_errstr = Str(X_1635,"\npvsys: error reading data");
          return rc;
        }
        else if (got < (int)n) {
          /* some error in file size: return integral number of frames read */
          toread  = got / (files[ifd]->pvdata.nAnalysisBins * 2 * sizeof(float));
          rc = toread;
        }
      }
      else
        rc = nframes;
    }
    files[ifd]->curpos += (toread * sizeof(float));
    files[ifd]->FramePos += nframes;

    return rc;
}

int pvoc_rewind(int ifd,int skip_first_frame)
{
    int rc = -1;
    int fd;
    long pos;
    long skipsize = 0;
    long skipframes = 0;

    if (files[ifd]==NULL) {
      pv_errstr = Str(X_1656,"\npvsys: bad file descriptor");
      return rc;
    }
    if (files[ifd]->fd < 0) {
      pv_errstr = Str(X_1655,"\npvsys: file not open");
      return rc;
    }
    skipsize =  files[ifd]->pvdata.dwFrameAlign * files[ifd]->fmtdata.nChannels;
    skipframes = files[ifd]->fmtdata.nChannels;

    fd = files[ifd]->fd;
    pos = files[ifd]->datachunkoffset;
    if (skip_first_frame) {
      skipsize =  files[ifd]->pvdata.dwFrameAlign * skipframes;
      pos += skipsize;
    }
    if (lseek(fd,(off_t)pos,SEEK_SET) != pos ) {
      pv_errstr = Str(X_1659,"\npvsys: error rewinding file");
      return rc;
    }
    files[ifd]->curpos = files[ifd]->datachunkoffset + skipsize;
    files[ifd]->FramePos = skipframes;

    return 0;

}

/* may be more to do in here later on */
int pvsys_release(void)
{
    int i;

    for (i=0;i < MAXFILES;i++) {
      if (files[i]) {
#ifdef _DEBUG
        fprintf(stderr,"\nDEBUG WARNING: files still open!\n");
#endif
        if (!pvoc_closefile(i)) {
          pv_errstr = Str(X_1660,"\npvsys: unable to close file on termination");
          return 0;
        }
      }
    }
    return 1;
}

/*return raw framecount:  channel-agnostic for now */
int pvoc_framecount(int ifd)
{
    if (files[ifd]==NULL)
      return -1;

    return files[ifd]->nFrames;
}

