/*  
    AIFF code for reading and writing audio files

    Copyright (C) 1997,2001 Dave Madole, Richard Dobson, John ffitch

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

#ifndef _SNDFILE_
/*  major kludge to implement AIFF-C 32 Floating point "compression" type */
/*  someday I'll rewrite AAAAAALLLLLL this stuff */
/*  Dave Madole 1/97 */

#include        "cs.h"                            /*                AIFF.C    */
#include        "soundio.h"
#include        "aiff.h"
#include        "sfheader.h"
#include        <math.h>
#include        "ieee80.h"
#include <time.h>                               /*RWD 3:2000*/
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifndef TRUE
#define TRUE    1
#define FALSE   0
#endif
#define DEBUG   0

struct FP32Chunk {
        CkHdr   ckHdr;
        unsigned long   applicationSignature;
        float   fmaxamps[AIFF_MAXCHAN+1];
} FP32Chunk;

struct FP32Chunk FP32Chunk;

static char     FORM_ID[4] = {'F','O','R','M'};
static char     COMM_ID[4] = {'C','O','M','M'};
static char     MARK_ID[4] = {'M','A','R','K'};
static char     INST_ID[4] = {'I','N','S','T'};
static char     SSND_ID[4] = {'S','S','N','D'};
static char     APPL_ID[4] = {'A','P','P','L'};
static char     APPL_SIG[4] = {'p','E','r','F'};
static char     FORM_TYPE[4] = {'A','I','F','C'};
static char     FL32_TYPE[4] = {'F','L','3','2'};

static char     FL32_NAME[12] =  Float32Name;
/*RWD 3:2000*/
static char     fl32_TYPE[4] = {'f','l','3','2'};         /* Apple's version */
static char     PEAK_ID[4] = {'P','E','A','K'};
/* RWD all AIFF-C files should contain this  now (Aug 1991 draft spec) */
static char     FVER_ID[4] = {'F','V','E','R'};
#define AIFCVersion1 (0xA2805140)
static AifcFverChunk fver;
#define sizFverChunk    sizeof(AifcFverChunk)

static FormHdr      form;
static CommChunk1   comm1;   /* CommonChunk split    */
static CommChunk2   comm2;   /*  to avoid xtra space */
static CommChunk3   comm3;
static AIFFAppSpecificChunk *appCkPtr;
static SoundDataHdr ssnd;

void GetMaxAmps(float *a);
static int sizFormHdr = sizeof(FormHdr);
static int sizCommChunk1 = sizeof(CkHdr) + sizeof(short); /* to avoid long roundup */
static int sizCommChunk2 = sizeof(CommChunk2);
static int sizCommChunk3 = 0;
static int sizAppChunk = 0;
static int sizSoundDataHdr = sizeof(SoundDataHdr);
static int sframe_size = 0;
static int aifchdrsize = 0;

extern int   bytrevhost(void);
/*RWD 3:2000*/
extern int   write_aiffpeak(int fd,int verbose);
static int sizPeakChunk = sizeof(PeakChunk);            /*but will change if more than mono...*/

#ifdef WORDS_BIGENDIAN
# define benfloat(x) (x)
# define benshort(x) (x)
# define benlong(x)  (x)
# define natshort(x) (x)
# define natlong(x)  (x)
#else
extern long  natlong(long);
extern float benfloat(float x);
extern short benshort(short sval);
extern long  benlong(long lval);
extern short natshort(short sval);
extern long  benlong(long lval);
#endif

void aifcWriteHdr(              /* Write AIFF header at start of file.   */
    int fd,                     /* Called after open, before data writes */
    int sampsize,               /* sample size in bytes */
    int nchls,
    double sr)                  /* sampling rate */
{
#if DEBUG
    printf("aifcWriteHdr: fd %d sampsize %d nchls %d sr %f\n",
           fd,sampsize,nchls,sr);
#endif
    sframe_size = sampsize * nchnls;
    form.ckHdr.ckID = *(long *) FORM_ID;
    form.ckHdr.ckSize = 0;                  /* leave for aiffReWriteHdr */
    form.formType = *(long *) FORM_TYPE;

    comm1.ckHdr.ckID = *(long *) COMM_ID;
    /*RWD 3:2000 add fver chunk */
    fver.ckHdr.ckID = *(long *) FVER_ID;
    fver.ckHdr.ckSize = benlong(sizeof(long));
    fver.version = benlong(AIFCVersion1);

    /*RWD 3:2000 must include pascal string size byte, and round up if necessary*/
    comm1.ckHdr.ckSize  = (long)sizeof(short) + sizCommChunk2 + sizeof(CKID) +
      (short)(FL32_NAME[0] + 1);
    if (comm1.ckHdr.ckSize & 1)
      comm1.ckHdr.ckSize++;
    comm1.ckHdr.ckSize = benlong(comm1.ckHdr.ckSize);

    comm1.numChannels = benshort((short)nchls);

    comm2.numSampleFrames = 0;              /* leave for aiffReWriteHdr */
    comm2.sampleSize = benshort((short)(sampsize * 8));
    double_to_ieee_80(sr,(unsigned char*)comm2.sampleRate);  /* insert 80-bit srate */

    comm3.compressionType = *(long *)FL32_TYPE;
                /*RWD 3:2000 write NAME[0] + 1 to include size byte     */
    memcpy(comm3.compressionName,FL32_NAME,FL32_NAME[0] + 1);/* Pascal string */
    sizCommChunk3 = sizeof(comm3.compressionType)+(short)(FL32_NAME[0]+1);
    if (sizCommChunk3 & 0x01) sizCommChunk3 += 1;

    sizAppChunk = sizeof(CkHdr)+sizeof(CKID)+(sizeof(float)*(nchnls+1));
    FP32Chunk.ckHdr.ckID = *(long *) APPL_ID;
                /*RWD 3:2000 below, needed to use benlong() */
    FP32Chunk.ckHdr.ckSize = benlong(sizeof(CKID)+(sizeof(float)*(nchnls+1)));
    FP32Chunk.applicationSignature = *(long *) APPL_SIG;
    ssnd.ckHdr.ckID = *(long *) SSND_ID;
    ssnd.ckHdr.ckSize = 0;                  /* leave for aifcReWriteHdr */
    ssnd.offset = 0;
    ssnd.blockSize = 0;

    if (write(fd, (char *)&form, sizFormHdr) != sizFormHdr       ||
        write(fd,(char *) &fver,sizFverChunk) != sizFverChunk    ||     /*RWD 3:2000*/
        write(fd, (char *)&comm1,sizCommChunk1) != sizCommChunk1 ||
        write(fd, (char *)&comm2,sizCommChunk2) != sizCommChunk2 ||
        write(fd, (char *)&comm3,sizCommChunk3) != sizCommChunk3 ||
        write(fd, (char *)&FP32Chunk,sizAppChunk) != sizAppChunk ||
        ((sizPeakChunk = write_aiffpeak(fd,0))==0) || /*RWD 3:2000*/
        write(fd, (char *)&ssnd, sizSoundDataHdr) != sizSoundDataHdr )
           die(Str(X_742,"error writing AIFF-C header"));
           aifchdrsize = sizFormHdr + sizCommChunk1 +
                         sizCommChunk2 + sizCommChunk3 +
             sizPeakChunk + sizFverChunk + /*RWD 3:2000*/
             sizAppChunk + sizSoundDataHdr;
           mfree((char *)appCkPtr);
}

void aifcReWriteHdr(            /* Write proper sizes into AIFF header */
    int   fd,                   /*         called before closing file  */
    long  datasize,             /*         & optionally under -R       */
    int   verbose)              /* Flag to say whether to trace peaks  */
{
    long endpos = lseek(fd,(off_t)0L,SEEK_CUR);
    long num_sframes, ssnd_size, form_size;

    if (datasize != endpos - aifchdrsize)
      die(Str(X_899,"inconsistent AIFF-C sizes"));
    num_sframes = datasize / sframe_size;
    ssnd_size = datasize + 2 * sizeof(long);
    form_size = endpos - sizeof(CkHdr);
#if DEBUG
    printf("aifcReWriteHdr: fd %d\n", fd);
    printf("endpos %lx num_sframes %lx ssnd_size %lx form_size %lx\n",
           endpos, num_sframes, ssnd_size, form_size);
#endif
    form.ckHdr.ckSize = benlong(form_size);
    comm2.numSampleFrames = benlong(num_sframes);
    GetMaxAmps(&(FP32Chunk.fmaxamps[0]));
    ssnd.ckHdr.ckSize = benlong(ssnd_size);
    if (lseek(fd, (off_t)0L, SEEK_SET))
      die(Str(X_1184,"seek error while updating AIFF-C header"));
    if ( write(fd, (char *)&form, sizFormHdr) != sizFormHdr       ||
         write(fd,(char *) &fver,sizFverChunk) != sizFverChunk    ||    /*RWD 3:2000*/
         write(fd, (char *)&comm1,sizCommChunk1) != sizCommChunk1 ||
         write(fd, (char *)&comm2,sizCommChunk2) != sizCommChunk2 ||
         write(fd, (char *)&comm3,sizCommChunk3) != sizCommChunk3 ||
         write(fd, (char *)&FP32Chunk,sizAppChunk) != sizAppChunk ||
         write_aiffpeak(fd,verbose) != sizPeakChunk                       ||    /*RWD 3:2000 */
         write(fd, (char *)&ssnd, sizSoundDataHdr) != sizSoundDataHdr )
      die(Str(X_738,"error while rewriting AIFF-C header"));
    lseek(fd, (off_t)endpos, 0);
}

void aifcfResetFrameSize(int sampsize, int nchanls) {
    sframe_size = sampsize * nchanls;
    comm2.sampleSize = 32;
}

extern int is_aiff_form(long);
int is_aifc_form(long firstlong) /* test a long for aiff form ID               */
                        /* called by readheader prior to aiffReadHeader */
{
    return (firstlong == *(long *)FORM_ID);
}

int is_aifc_formtype(int fd) /* test a long for aiff form ID                 */
                             /* called by readheader prior to aiffReadHeader */
{
    FormHdr form;
    long saveloc = lseek(fd,(off_t)0L,SEEK_CUR);
    read(fd,(char *)&form + sizeof(long),sizeof(FormHdr) - sizeof(long));
    lseek(fd,(off_t)saveloc,SEEK_SET);
    return (form.formType == *(long *)FORM_TYPE);
}

typedef struct {
  short markerID;
  long  position;
} MARKER;

void aifcReadHeader(            /* Read AIFF header, fill hdr, &  */
  int fd,                       /* postn rd ptr to start of samps */
  char *fname,
  HEADATA *hdr,                 /* datablock for passing data back */
  long firstlong,
  SOUNDIN *p)
{
    CkHdr        ckHdr;
    FormHdr      form;
    CommChunk1   comm1;
    CommChunk2   comm2;
    InstrChunk   instr;
    SoundDataHdr ssnd;
    int mark_read = 0, inst_read = 0, loops_read = 0, peak_read = 0;
    int comm_read = 0, ssnd_read = 0;
    int fver_read = 0;  /*RWD 3:2000*/
    long fver_version = 0;
    long ssnd_offset, ssnd_pos=0L, pos, ckSize;
    long Appl_ID = 0;
    short sampsize, nmarkers = 0, nn;
    MARKER  *markersp = NULL, *mp = NULL;
    Loop    *ilp = NULL;
    AIFFDAT *adp = NULL;
    char    *err;
    double  sr, oct;

        /*RWD 3:2000*/
    hdr->peaksvalid = 0;
    hdr->peak_do_rescaling = 1;         /* we will have a command arg eventually*/
    p->fscalefac = 1.0f;
    p->do_floatscaling = 0;

    p->filetyp = 0;             /* ensure no bytrevs in sreadin for now */
    if (!is_aiff_form(firstlong))   /* double check it's a form header  */
      die(Str(X_611,"bad form for aifcReadHeader"));/* & read remainder */
    sreadin(fd,(char *)&form + sizeof(long),sizeof(FormHdr) - sizeof(long),p);
    if (form.formType != *(long *) FORM_TYPE)
      die(Str(X_771,"form header not type 'AIFC'"));
    hdr->readlong = FALSE;
    while (1) {                              /* read in the next header */
      if (sreadin(fd,(char *)&ckHdr,sizeof(CkHdr),p) < sizeof(CkHdr))
        break;
      pos = lseek(fd,(off_t)0L,SEEK_CUR);
                /*RWD 3:2000 check for FVER chunk*/
      if (ckHdr.ckID == *(long *) FVER_ID) {
        sreadin(fd,(char *) &fver_version,sizeof(long),p);
        /*check the value after everythign else is read in */
        fver_read =1;
      }
      else if (ckHdr.ckID == *(long *) COMM_ID) {  /* CommChunk hdr: rd rem 1 */
        char pad;               /*RWD 3:2000*/
        sreadin(fd,(char *)&comm1 + sizeof(CkHdr), sizeof(short), p);
        sreadin(fd,(char *)&comm2, sizCommChunk2, p); /* + all of part 2 */
        sreadin(fd,(char *)&comm3, sizeof(CKID)+1, p);
        sreadin(fd,(char *)&comm3+sizeof(CKID)+1,
                (short)comm3.compressionName[0], p);
        /*RWD 3:2000 read pad byte if necessary */
        if (!(comm3.compressionName[0] & 1))
          sreadin(fd,&pad,sizeof(char),p);
        /* this reflects sampsize BEFORE compression
           and should be ignored when dealing with floats */
        sampsize = natshort(comm2.sampleSize);
        /*RWD 3:2000 need to recognise both forms */
        if (!((comm3.compressionType == *(long *) FL32_TYPE) ||
              (comm3.compressionType == *(long *) fl32_TYPE))) {
          die(Str(X_224,"Compression Type is not FL32"));
        }
        /* if we ever support any other compression types then we can no
           longer assume that aiff-c is 32 bit float and will have to do
           stuff below.. */
        /*if (sampsize <= 8) {*/          /* parse CommChunk to hdr format */
        /*  hdr->format = AE_CHAR;
            hdr->sampsize = sizeof(char);
            }
            else if (sampsize <= 16) {
            hdr->format = AE_SHORT;
            hdr->sampsize = sizeof(short);
            }
            else if (sampsize <= 24)
            die(Str(X_187,"AIFF-C 3-byte samples not supported"));
            else */
        {
          hdr->format = AE_FLOAT;
          hdr->sampsize = sizeof(float);
        }
        hdr->nchanls = natshort(comm1.numChannels);
        /* decode 80-bit srate */
        sr = ieee_80_to_double((unsigned char*)comm2.sampleRate);
        hdr->sr = (long) sr;
        comm_read = TRUE;
      }
      /*RWD 3:2000*/
      else if (ckHdr.ckID == *(long *) PEAK_ID) {
        int i;
        long version, timestamp,chunksize,expected_size;
        float maxpeak = 0.0f;
        chunksize = natlong(ckHdr.ckSize);
        expected_size = sizeof(PeakChunk) - 2 * sizeof(long);
        expected_size += (hdr->nchanls -1) * sizeof(PositionPeak);
        if (chunksize!=expected_size)
          die(Str(X_1502,"bad size for PEAK chunk in AIFC file"));
        if (comm_read) {
          sreadin(fd,(char *) &version,sizeof(long),p);
          version = benlong(version);
          if (version != PEAKCHUNK_VERSION) 
            die(Str(X_1503,"unknown PEAK chunk version in AIFC file"));
          sreadin(fd,(char *) &timestamp,sizeof(long),p);
          hdr->peak_timestamp = benlong(timestamp);
          for (i=0; i < hdr->nchanls;i++) {
            sreadin(fd,(char *) &hdr->peaks[i],sizeof(PositionPeak),p);
            hdr->peaks[i].value = benfloat(hdr->peaks[i].value);
            hdr->peaks[i].position = benlong(hdr->peaks[i].position);
            if (maxpeak< hdr->peaks[i].value) maxpeak = hdr->peaks[i].value;
          }
          printf(Str(X_1513,"Read PEAK data:\ncreation time: %s"),
                 ctime((time_t *) &(hdr->peak_timestamp)));
          for (i=0;i < hdr->nchanls;i++) {
            printf(Str(X_1514,"CH %d: peak = %.6f at sample %d: %.4lf secs\n"),
                   i+1, hdr->peaks[i].value,hdr->peaks[i].position,
                   (double)(hdr->peaks[i].position)/ hdr->sr);
          }
          hdr->peaksvalid = 1;
          if (maxpeak > 0.0f && maxpeak > 1.0f) {
            p->fscalefac = 1.0f / maxpeak;
            p->do_floatscaling = 1;
            printf(Str(X_1516,"Input scale factor = %f\n"),p->fscalefac);
          }
        }
        else
          die(Str(X_1504,"AIFC format error: PEAK chunk found before COMM chunk"));
      }
      else if (ckHdr.ckID == *(long *) MARK_ID) {       /* MarkersChunk: */
        sreadin(fd,(char *)&nmarkers, sizeof(short), p);
        nmarkers = natshort(nmarkers);
        /*          printf("nmarkers = %d\n",nmarkers); */
        if (nmarkers != 0) {
          markersp = (MARKER *) mcalloc((long)sizeof(MARKER) * nmarkers);
          for (nn = nmarkers, mp = markersp; nn--; mp++) {  /* for nmarkrs */
            u_char psiz, pstring[256];             /* read ID/postn pair */
            sreadin(fd,(char *)&mp->markerID, sizeof(short), p);
            sreadin(fd,(char *)&mp->position, sizeof(long), p);
            /*  printf("markerID = %d position = %ld\n",
                mp->markerID, mp->position); */
            sreadin(fd,(char *)&psiz, 1, p);       /* leave unnatural,   */
            psiz |= 01;                            /*     & skip pstring */
            sreadin(fd, (char *)pstring, (int)psiz, p);
          }
        }
        mark_read = TRUE;
      }
      else if (ckHdr.ckID == *(long *) APPL_ID) {       /* Instr Chunk:  */
        int subhdrsiz = natlong(ckHdr.ckSize);  /*RWD 3:2000 use natlong() */
        sreadin(fd,(char *)&Appl_ID, sizeof(Appl_ID), p);
                        /*RWD 3:2000 APPL chunk can be absolutely anything! */
        if (Appl_ID != *(long *) APPL_SIG) {
          long toskip;
          if (O.msglevel & WARNMSG)
            printf(Str(X_191,"WARNING: Application Signature not pErF\n"));
          toskip = natlong(ckHdr.ckSize) - sizeof(long);
          if (toskip & 1)
            toskip++;
          if (lseek(fd,(off_t)toskip,SEEK_CUR) < 0)
            die(Str(X_1768,"error skipping unrecognised APPL chunk"));
          continue;
        }
        subhdrsiz -= sizeof(Appl_ID);
        if (hdr->aiffdata == NULL)
          hdr->aiffdata = adp = (AIFFDAT *)mcalloc((long)sizeof(AIFFDAT));
        sreadin(fd,(char *)&(adp->fmaxamps), subhdrsiz, p);
        peak_read = TRUE;
      }
      else if (ckHdr.ckID == *(long *) INST_ID) { /* Instr chunk */
        int subhdrsiz = sizeof(InstrChunk) - sizeof(CkHdr);
        if (hdr->aiffdata == NULL)
          hdr->aiffdata = adp = (AIFFDAT *)mcalloc((long)sizeof(AIFFDAT));
        sreadin(fd,(char *)&instr + sizeof(CkHdr), subhdrsiz, p);
        oct = (instr.baseNote + instr.detune/100.0) / 12.0 + 3.0;
        adp->natcps = (float)(pow(2.0, oct) * ONEPT);
        adp->gainfac = (float)exp((double)(natshort(instr.gain)) * LOG10D20);
        inst_read = TRUE;
      }
      else if (ckHdr.ckID == *(long *) SSND_ID) { /* SoundDataHdr: */
        int subhdrsiz = sizeof(SoundDataHdr) - sizeof(CkHdr);
        sreadin(fd,(char *)&ssnd + sizeof(CkHdr), subhdrsiz, p);
        ssnd_offset = natlong(ssnd.offset);
        ssnd_pos = pos + subhdrsiz + ssnd_offset;
        hdr->hdrsize = ssnd_pos;
        hdr->audsize = natlong(ckHdr.ckSize) - subhdrsiz - ssnd_offset;
        hdr->filetyp = TYP_AIFC;
        ssnd_read = TRUE;
        /* RWD 3:2000 calculate if we have any more chunks to read...*/
      }
      if (hdr->hdrsize + hdr->audsize - sizeof(ckHdr) ==
          (unsigned int)natlong(form.ckHdr.ckSize))
        break;
/*RWD 3:2000 moved outside while{} loop */
      else {
        ckSize = natlong(ckHdr.ckSize); /* else seek past this chunk to nxt */
        if (ckSize & 1)  ckSize++;      /*      rnded up to even byte bndry */
        if (lseek(fd, (off_t)(pos + ckSize), 0) != pos + ckSize)
          die(Str(X_740,"error while seeking past AIFF-C chunk"));
        continue;
      }
    }
    if (mark_read && inst_read && !loops_read) {
      ilp = &instr.sustainLoop;
      adp->loopmode1 = natshort(ilp->playMode);
      if (adp->loopmode1 && nmarkers > 0) {
        for (nn = nmarkers, mp = markersp; nn--; mp++) {
          if (mp->markerID == ilp->beginLoop)
            adp->begin1 = natlong(mp->position);
          if (mp->markerID == ilp->endLoop)
            adp->end1 = natlong(mp->position);
        }
      }
      ilp = &instr.releaseLoop;
      adp->loopmode2 = natshort(ilp->playMode);
      if (adp->loopmode2 && nmarkers > 0) {
        for (nn = nmarkers, mp = markersp; nn--; mp++) {
          if (mp->markerID == ilp->beginLoop)
            adp->begin2 = natlong(mp->position);
          if (mp->markerID == ilp->endLoop)
            adp->end2 = natlong(mp->position);
        }
      }
      err = NULL;
      if (adp->natcps <= 0.0)
          err = Str(X_616,"baseNote");
      if (adp->loopmode1 < 0 || adp->loopmode1 > 3)
        err = Str(X_1252,"sustain loop playMode");
      else if (adp->loopmode1
               && (adp->begin1 < 0 || adp->begin1 >= adp->end1))
        err = Str(X_1253,"sustain loop");
      else if (adp->loopmode2 < 0 || adp->loopmode2 > 3)
        err = Str(X_1159,"release loop playMode");
      else if (adp->loopmode2
               && (adp->begin2 < 0 || adp->begin2 >= adp->end2))
        err = Str(X_1160,"release loop");
      if (err != NULL) {
        printf(Str(X_297,"INFILE ERROR: illegal %s info in AIFF-C file %s\n"),
               err,fname);
        hdr->aiffdata = NULL;
        mfree((char *)adp);
      }
      if (markersp != NULL) mfree(markersp);
      loops_read = TRUE;
    }
    /*RWD 3: 2000*/
    if (!comm_read)
        die(Str(X_1505,"no COMM chunk in AIFF-C file!"));
    if (!ssnd_read)
        die(Str(X_1506,"no SSND chunk in AIFF-C file!"));
    if (fver_read) {
      if (natlong(fver_version) != AIFCVersion1)
        printf(Str(X_1507,"warning: unexpected FVER version in  AIFF-C file\n"));
    }
    else
        printf(Str(X_1508,
                   "warning: no FVER chunk in AIFF-C file - may be"
                   " obsolete format\n"));

    printf(Str(X_66,"%s: AIFF-C, %ld%s samples"), fname,
           hdr->audsize/hdr->sampsize/hdr->nchanls,
           hdr->nchanls==1 ? " " : " stereo");

    if (inst_read) {
      if (instr.detune == 0)
        printf(Str(X_81,", baseFrq %4.1f (midi %d), gain %d db"),
               adp->natcps, instr.baseNote, natshort(instr.gain));
      else
        printf(Str(X_82,", baseFrq %4.1f (midi %d, detune %d), gain %d db"),
               adp->natcps, instr.baseNote, instr.detune, natshort(instr.gain));
    }
    if (loops_read) {
      printf(Str(X_88,", sustnLp: mode %d"), adp->loopmode1);
      if (adp->loopmode1)
        printf(Str(X_80,", %ld to %ld"), adp->begin1,adp->end1);
      printf(Str(X_87,", relesLp: mode %d"), adp->loopmode2);
      if (adp->loopmode2)
        printf(Str(X_80,", %ld to %ld"), adp->begin2,adp->end2);
      printf("\n");
    }
    else printf(Str(X_86,", no looping\n"));
    if (!hdr->peaksvalid && peak_read) { /*RWD 3:2000 no need to report twice! */
      int i = 0;
      printf(Str(X_1116,"overall peak: %f\n"),adp->fmaxamps[0]);
      printf(Str(X_657,"channel peaks:\n"));
      for (i=0; i<hdr->nchanls; i++)
        printf("%1d: %f%s", i, adp->fmaxamps[i+1], i+1 % 4 == 0 ? "\n" : " ");
      if (i+1%4) printf("\n");
    }
    if ((long)lseek(fd,(off_t)ssnd_pos,0) != ssnd_pos)
      die(Str(X_731,"error seeking to start of sound data"));
  }

extern OPARMS O;

void GetMaxAmps(float *a)
{
    int i,j;
    MYFLT lmaxamp = 0.0f;
    MYFLT absmax = (MYFLT)fabs(*a);
    for (i=0; i<nchnls; i++) {
      j = i+1;
      a[j] = (float)maxamp[i];
      if (absmax < lmaxamp) {
        *a = (float)lmaxamp;
        absmax = (MYFLT)fabs(*a);
      }
      lmaxamp = (MYFLT)fabs(smaxamp[i]);
      if (absmax < lmaxamp) {
        *a = (float)lmaxamp;
        absmax = (MYFLT)fabs(*a);
      }
      if ((fabs(a[j]) < lmaxamp))
        a[j] = (float)smaxamp[i];
      lmaxamp = (MYFLT)fabs(omaxamp[i]);
      if (absmax < lmaxamp) {
        *a = (float)lmaxamp;
        absmax = (MYFLT)fabs(*a);
      }
      if ((fabs(a[j]) < lmaxamp))
        a[j] = (float)omaxamp[i];
    }
}

#endif
