/*  
    sndlib.c:

    Copyright (C) 2004 John ffitch

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

#ifdef _SNDFILE_

#include "cs.h"                         /*             SNDLIB.C       */
#include "soundio.h"
#include <sndfile.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#ifdef RTAUDIO
extern  int     (*rtrecord)(char *, int);
extern  void    (*rtplay)(void *, int);
extern  void    (*rtclose)(void);
extern  void    (*recopen)(int, int, float, int);
extern  void    (*playopen)(int, int, float, int);
#endif

static  SNDFILE *outfile;
static  char    *sfoutname;                     /* soundout filename    */
static  char    *inbuf;
        char    *outbuf;                        /* contin sndio buffers */
static  unsigned inbufrem, outbufrem;           /* in monosamps         */
                                                /* (see openin,iotranset)    */
static  unsigned inbufsiz,  outbufsiz;          /* alloc in sfopenin/out     */
static  int     isfd, isfopen = 0, infilend = 0;/* (real set in sfopenin)    */
static  int     osfd, osfopen = 0;              /* (real set in sfopenout)   */
static  int     pipdevin = 0, pipdevout = 0;    /* mod by sfopenin,sfopenout */
#ifdef RTAUDIO
#define DEVAUDIO 0x7fff         /* unique fd for rtaudio  */
# ifdef sol
extern  int     audiofd;
# endif
#endif
#ifdef PIPES
extern FILE* pin, *pout;
/*sbrandon: added NeXT to line below*/
# if defined(SGI) || defined(LINUX) || defined(__BEOS__) || defined(NeXT) || defined(__MACH__)
#  define _popen popen
#  define _pclose pclose
# endif
#endif
extern void (*spinrecv)(void), (*spoutran)(void), (*nzerotran)(long);
static  int     (*audrecv)(void *, int);
static  void    (*audtran)(void *, int);

extern char *getstrformat(int format);

static int type2sf(int type)
{
    switch (type) {
    case TYP_WAV:
      return SF_FORMAT_WAV;
    case TYP_AIFF:
      return SF_FORMAT_AIFF;
    case TYP_AIFC:
      return SF_FORMAT_IRCAM;
    }
    return SF_FORMAT_RAW;
}

int format2sf(int format)
{
    switch (format) {
    case AE_CHAR:
      return SF_FORMAT_PCM_S8;
#ifdef never
    case AE_ALAW:
      return SF_FORMAT_ALAW
#endif
#ifdef ULAW
    case AE_ULAW:
      return SF_FORMAT_ULAW
#endif
    case AE_SHORT:
      return SF_FORMAT_PCM_16;       /* Signed 16 bit data */
    case AE_LONG:
      return SF_FORMAT_PCM_32;       /* Signed 32 bit data */
    case AE_FLOAT:
      return SF_FORMAT_FLOAT;       /* 32 bit float data */
    case AE_UNCH:
      return SF_FORMAT_PCM_U8;       /* Unsigned 8 bit data (WAV and RAW only) */
    case AE_24INT:
      return SF_FORMAT_PCM_24;       /* Signed 24 bit data */
    case AE_DOUBLE:
      return SF_FORMAT_DOUBLE;       /* 64 bit float data */
    }
    return SF_FORMAT_PCM_16;
}

void writesf(void *b, int len)
{
    int i;
    printf("***writesf:%p, %d\n", b, len);
    for (i=0; i<len; i++) {
    }
    sf_writef_float(outfile, (float*)b, len);
}

void zerosf(int len)
{
    int i;
    MYFLT x = FL(0.0);
    for (i=0; i<len; i++)
      sf_writef_float(outfile, &x, 1);
}

int SAsndgetset(
     char    *infilnam,                          /* Stand-Alone sndgetset() */
     SOUNDIN **ap,                               /* used by SoundAnal progs */
     MYFLT   *abeg_time,
     MYFLT   *ainput_dur,
     MYFLT   *asr,
     int     channel)
{                               /* Return -1 on failure */
    return -1;
}

long getsndin(int fd, MYFLT *fp, long nlocs, SOUNDIN *p)
        /* a simplified soundin */
{
    return 0;
}

HEADATA *readheader(            /* read soundfile hdr, fill HEADATA struct */
    int ifd,                    /*   called by sfopenin() and sndinset()   */
    char *sfname,               /* NULL => no header, nothing to preserve  */
    SOUNDIN *p)
{
    return NULL;
}

void rewriteheader(int ofd, long datasize, int verbose)
{
    return;
}

int sndgetset(SOUNDIN *p)       /* core of soundinset                */
                                /* called from sndinset, SAsndgetset, & gen01 */
                                /* Return -1 on failure */
{
    return -1;
}

int sreadin(                    /* special handling of sound input       */
    int     infd,               /* to accomodate reads thru pipes & net  */
    char    *inbuf,             /* where nbytes rcvd can be < n requested*/
    int     nbytes,             /*  */
    SOUNDIN *p)                 /* extra arg passed for filetyp testing  */
{                               /* on POST-HEADER reads of audio samples */
    return 0;
}

void writeheader(int ofd, char *ofname) 
{
    return;
}

int sndinset(SOUNDIN *p)    /* init routine for instr soundin   */
                             /* shares above sndgetset with SAsndgetset, gen01*/
{
    return 0;
}

int soundin(SOUNDIN *p)
{
    return 0;
}

void sfopenin(void)             /* init for continuous soundin */
{
    return;
}

void sfopenout(void)                            /* init for sound out       */
{                                               /* (not called if nosound)  */
    printf("***sfopenout\n");
#ifdef NeXT
    if (O.outfilename == NULL && !O.filetyp) O.outfilename = "test.snd";
        else if (O.outfilename == NULL) O.outfilename = "test";
#else
    if (O.outfilename == NULL) {
      if (O.filetyp == TYP_WAV) O.outfilename = "test.wav";
      else if (O.filetyp == TYP_AIFF) O.outfilename = "test.aif";
      else O.outfilename = "test";
    }
#endif
    printf("***O.outfilename=%s\n", O.outfilename);
    if (strcmp(O.outfilename,"stdout") == 0) {
      sfoutname = O.outfilename;
      osfd = O.stdoutfd;              /* send sound to stdout if requested */
      pipdevout = 1;
    }
#ifdef PIPES
    else if (O.outfilename != NULL && O.outfilename[0]=='|') {
      FILE *_popen(const char *, const char *);
      pout = _popen(O.outfilename+1, "w");
      osfd = fileno(pout);
      pipdevout = 1;
      if (O.filetyp == TYP_AIFF ||
          O.filetyp == TYP_AIFC ||
          O.filetyp == TYP_WAV) {
        printf(Str(X_400,"Output file type changed to IRCAM for use in pipe\n"));
        O.filetyp = TYP_IRCAM;
      }
    }
#endif
#ifdef RTAUDIO
    else if (strcmp(O.outfilename,"devaudio") == 0
             || strncmp(O.outfilename,"devaudio", 8) ==0
             || strncmp(O.outfilename,"dac", 3) ==0
# ifdef LINUX
             || strcmp(O.outfilename,"/dev/dsp") ==0
# endif
             || strcmp(O.outfilename,"dac") == 0) {
#if defined(WIN32) || defined(HAVE_ALSA)
      if (strncmp(O.outfilename,"devaudio", 8) == 0)
        sscanf(O.outfilename+8, "%d", &rtout_dev);
      else if (strncmp(O.outfilename,"dac", 3) == 0)
        sscanf(O.outfilename+3, "%d", &rtout_dev);
#endif
      sfoutname = O.outfilename;
      playopen(nchnls, O.outsampsiz, (float)esr, 2);  /* open devaudio for out */
      audtran = rtplay;                        /* & redirect audio puts */
#ifdef NeXT /*sbrandon: even RT playback has to be swapped*/
# ifdef __LITTLE_ENDIAN__
        audtran = swaprtplay;
# endif
#endif
      osfd = DEVAUDIO;                         /* dummy file descriptor */
      pipdevout = 1;                           /* no backward seeks !   */
#ifdef sol
      if (O.sfheader)
        writeheader(audiofd,"devaudio");
#endif
#if defined(mills_macintosh) || defined(SYMANTEC)
      O.outformat = AE_SHORT;
#endif
#ifdef mills_macintosh
      transport.state |= kGenRealtime;
#endif
      goto outset;                        /* no header needed      */
    }
#endif
    else if (strcmp(O.outfilename,"null") == 0) {
      osfd = -1;
      sfoutname = mmalloc((long)strlen(retfilnam)+1);
      strcpy(sfoutname, retfilnam);       /*   & preserve the name */
    }
    else {  /* else open sfdir or cwd */
      SF_INFO sfinfo;
      sfinfo.frames = -1;
      sfinfo.samplerate = (int)esr;
      sfinfo.channels = nchnls ;
      sfinfo.format = type2sf(O.filetyp)|format2sf(O.outformat);
      sfinfo.sections = 0;
      sfinfo.seekable = 0;
      if ((osfd = openout(O.outfilename, 3)) < 0) 
        dies(Str(X_1187,"sfinit: cannot open %s"), retfilnam);
      sfoutname = mmalloc((long)strlen(retfilnam)+1);
      strcpy(sfoutname, retfilnam);       /*   & preserve the name */
      printf("***Opening sndfile %s\n", O.outfilename);
      outfile = sf_open_fd(osfd, SFM_WRITE, &sfinfo, 1);
      printf("***Done\n");
      if (strcmp(sfoutname, "/dev/audio") == 0) {
        /*      ioctl(   );   */
        pipdevout = 1;
      }
      spoutran = writesf;
      nzerotran = zerosf;
    }
#if defined(SYMANTEC)
    AddMacHeader(sfoutname,nchnls,esr,O.outsampsiz);  /* set Mac resource */
    SetMacCreator(sfoutname);               /*   set creator & file type */
#endif
    if (O.sfheader) {
#ifdef mills_macintosh
      transport.osfd = osfd;
      transport.eoheader = lseek(osfd,(off_t)0L,SEEK_CUR);
#endif
    }
 outset:
    printf("***finished sfopenout\n");
}

void iotranset(void)
{
    return;
}


void sfclosein(void)
{
    return;
}

void sfcloseout(void)
{
    int nb;
    if (!osfopen) return;
    if ((nb = (O.outbufsamps-outbufrem) * O.outsampsiz) > 0)/* flush outbuffer */
      audtran(outbuf, nb);
    sf_close(outfile);
#ifdef RTAUDIO
    if (osfd == DEVAUDIO) {
      if (!isfopen || isfd != DEVAUDIO)
        rtclose();     /* close only if not open for input too */
      goto report;
    }
#endif
#ifdef PIPES
    if (pout!=NULL) {
      int _pclose(FILE*);
      _pclose(pout);
      pout = NULL;
    }
#endif
#ifdef RTAUDIO
 report:
#endif
    printf(Str(X_44,"%ld %d-byte soundblks of %s written to %s"),
           nrecs, outbufsiz, getstrformat(O.outformat), sfoutname);
    if (strcmp(O.outfilename,"devaudio") == 0       /* realtime output has no
                                                       header */
        || strcmp(O.outfilename,"dac") == 0) printf("\n");
    else if (O.sfheader == 0) printf(" (raw)\n");
    else
      printf(" %s\n",
             O.filetyp == TYP_AIFF ? "(AIFF)" :
             O.filetyp == TYP_AIFC ? "(AIFF-C)" :
             O.filetyp == TYP_WAV ? "(WAV)" :
#ifdef mac_classic
                                             "(SDII)"
#elif defined(SFIRCAM)
                                             "(IRCAM)"
#elif defined(NeXT)
                                             "(NeXT)"
#else
                                             "(Raw)"
#endif
             );
    osfopen = 0;
}

#endif

