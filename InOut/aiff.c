/*  
    aiff.c:

    Copyright (C) 1991 Barry Vercoe, John ffitch, Richard Dobson

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

#include        "cs.h"                            /*                AIFF.C    */
#include        "soundio.h"
#include        "aiff.h"
#include        "sfheader.h"
#include        <math.h>
#include        "ieee80.h"

#include <time.h>
int write_aiffpeak(int fd,int verbose);

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#if !defined(mac_classic) && !defined( SYMANTEC ) && !defined( LINUX )
extern int write(int, const void*, unsigned int);
extern off_t lseek(int, off_t, int);
#endif

#ifndef TRUE       /* -EAD */
#define TRUE    1
#define FALSE   0
#endif
#define DEBUG   0

static char     FORM_ID[4] = {'F','O','R','M'};
static char     COMM_ID[4] = {'C','O','M','M'};
static char     MARK_ID[4] = {'M','A','R','K'};
static char     INST_ID[4] = {'I','N','S','T'};
static char     SSND_ID[4] = {'S','S','N','D'};
static char     FORM_TYPE[4] = {'A','I','F','F'};
static char     NAME_ID[4] = {'N', 'A', 'M', 'E'};      /* ckID for Name Chunk */
static char     AUTHOR_ID[4] = {'A', 'U', 'T', 'H'};    /* ckID for Author Chunk */
static char     COPYRIGHT_ID[4] = {'(', 'c', ')', ' '}; /* ckID for Copyright Chunk */
static char     ANNOTATION_ID[4] = {'A', 'N', 'N', 'O'};/* ckID for Annotation Chunk */
static char     PEAK_ID[4] = {'P','E','A','K'};

static FormHdr      form;
static CommChunk1   comm1;   /* CommonChunk split    */
static CommChunk2   comm2;   /*  to avoid xtra space */
static SoundDataHdr ssnd;

static int sizFormHdr = sizeof(FormHdr);
static int sizCommChunk1 = sizeof(CkHdr) + sizeof(short); /* to avoid long roundup */
static int sizCommChunk2 = sizeof(CommChunk2);
static int sizSoundDataHdr = sizeof(SoundDataHdr);
static int sframe_size;
static int aiffhdrsiz = sizeof(FormHdr)
                      + sizeof(CkHdr) + sizeof(short)
                      + sizeof(CommChunk2)
                      + sizeof(SoundDataHdr);
/*RWD 3:2000 */
static int sizPeakChunk = sizeof(PeakChunk);            /*but will change if more than mono...*/

int bytrevhost(void)
{
/*    return(*(long *)FORM_ID != 'FORM'); */
    return(*(long *)FORM_ID != 0x464f524d);
}

#ifdef WORDS_BIGENDIAN
# define benfloat(x) (x)
# define benshort(x) (x)
# define benlong(x)  (x)
# define natshort(x) (x)
# define natlong(x)  (x)
#else
short benshort(short sval)   /* coerce a natural short into a bigendian short */
{
    char  benchar[2];
    char *p = benchar;

    *p++ = 0xFF & (sval >> 8);
    *p   = 0xFF & sval;
    printf("%.4x -> %.4x\n", sval, *(short *)benchar);
    return(*(short *)benchar);
}

long benlong(long lval)       /* coerce a natural long into a bigendian long */
{
    char  benchar[4];
    char *p = benchar;

    *p++ = (char)(0xFF & (lval >> 24));
    *p++ = (char)(0xFF & (lval >> 16));
    *p++ = (char)(0xFF & (lval >> 8));
    *p   = (char)(0xFF & lval);
    printf("%.8x -> %.8x\n", lval, *(long *)benchar);
    return(*(long *)benchar);
}

float benfloat(float x)
{
    union { long l ; float f; } cheat;
    cheat.f = x; cheat.l = benlong(cheat.l); return cheat.f;
}

short natshort(short sval)    /* coerce a bigendian short into a natural short */
{
    unsigned char benchar[2];
    short natshrt;

    *(short *)benchar = sval;
    natshrt = benchar[0];
    natshrt <<= 8;
    natshrt |= benchar[1];
/*     printf("%.4x <- %.4x\n", sval, natshrt); */
    return(natshrt);
}

long natlong(long lval)      /* coerce a bigendian long into a natural long */
{
    unsigned char benchar[4];
    unsigned char *p = benchar;
    long natlng;

    *(long *)benchar = lval;
    natlng = *p++;
    natlng <<= 8;
    natlng |= *p++;
    natlng <<= 8;
    natlng |= *p++;
    natlng <<= 8;
    natlng |= *p;
/*     printf("%.8x <- %.8x\n", lval, natlng); */
    return(natlng);
}
#endif

void aiffWriteHdr(              /* Write AIFF header at start of file.   */
    int fd,                     /* Called after open, before data writes */
    int sampsize,               /* sample size in bytes */
    int nchls,
    double sr)                  /* sampling rate */
{
#if DEBUG
    printf("aiffWriteHdr: fd %d sampsize %d nchls %d sr %f\n",
           fd,sampsize,nchls,sr);
#endif
    sframe_size = sampsize * nchnls;
    form.ckHdr.ckID = *(long *) FORM_ID;
    form.ckHdr.ckSize = 0;                  /* leave for aiffReWriteHdr */
    form.formType = *(long *) FORM_TYPE;
    comm1.ckHdr.ckID = *(long *) COMM_ID;
    comm2.numSampleFrames = 0;              /* leave for aiffReWriteHdr */
    comm1.ckHdr.ckSize = benlong((long)sizeof(short) + sizCommChunk2);
    comm1.numChannels = benshort((short)nchls);
    comm2.sampleSize = benshort((short)(sampsize * 8));
    double_to_ieee_80(sr,(unsigned char*)comm2.sampleRate);  /* insert 80-bit srate */
    ssnd.ckHdr.ckID = *(long *) SSND_ID;
    ssnd.ckHdr.ckSize = 0;                  /* leave for aiffReWriteHdr */
    ssnd.offset = 0;
    ssnd.blockSize = 0;

    if ( write(fd, (char *)&form, sizFormHdr) != sizFormHdr         ||
         write(fd, (char *)&comm1,sizCommChunk1) != sizCommChunk1   ||
         write(fd, (char *)&comm2,sizCommChunk2) != sizCommChunk2   ||
         /*RWD 3:2000*/
         (peakchunks && (sizPeakChunk = write_aiffpeak(fd,0))==0)   ||
         write(fd, (char *)&ssnd, sizSoundDataHdr) != sizSoundDataHdr )
      die(Str(X_741,"error writing AIFF header"));

    /*RWD 3:2000 adjust overall size */
    aiffhdrsiz = sizeof(FormHdr) + sizeof(CkHdr) + sizeof(short) +
                 sizeof(CommChunk2) + sizeof(SoundDataHdr);
    if (peakchunks) aiffhdrsiz += sizPeakChunk;
}

void aiffReWriteHdr(            /* Write proper sizes into AIFF header */
    int   fd,                   /*         called before closing file  */
    long  datasize,             /*         & optionally under -R       */
    int   verbose)              /* Flag to say whether to trace peaks  */
{
    long endpos = (long)lseek(fd, (off_t)0L, SEEK_END);
    long num_sframes, ssnd_size, form_size;

    if (datasize != endpos - aiffhdrsiz)
      die(Str(X_898,"inconsistent AIFF sizes"));
    num_sframes = datasize / sframe_size;
    ssnd_size = datasize + 2 * sizeof(long);
    form_size = endpos - sizeof(CkHdr);
#if DEBUG
    printf("aiffReWriteHdr: fd %d\n", fd);
    printf("endpos %lx num_sframes %lx ssnd_size %lx form_size %lx\n",
           endpos, num_sframes, ssnd_size, form_size);
#endif
    form.ckHdr.ckSize = benlong(form_size);
    comm2.numSampleFrames = benlong(num_sframes);
    ssnd.ckHdr.ckSize = benlong(ssnd_size);
    if (lseek(fd, (off_t)0L, SEEK_SET))
      die(Str(X_1183,"seek error while updating AIFF header"));
    if (write(fd, (char *)&form, sizFormHdr) != sizFormHdr              ||
        write(fd, (char *)&comm1,sizCommChunk1) != sizCommChunk1        ||
        write(fd, (char *)&comm2,sizCommChunk2) != sizCommChunk2        ||
        /*RWD 3:2000 */
        (peakchunks && write_aiffpeak(fd,verbose) != sizPeakChunk)      ||
        write(fd, (char *)&ssnd, sizSoundDataHdr) != sizSoundDataHdr )
      die(Str(X_737,"error while rewriting AIFF header"));
    lseek(fd, (off_t)endpos, SEEK_SET);
}

#ifdef mills_macintosh
void aifffResetFrameSize(int sampsize, int nchanls)
{
    sframe_size = sampsize * nchanls;
    comm2.sampleSize = 16;
}
#endif

int is_aiff_form(long firstlong) /* test a long for aiff form ID                 */
                                 /* called by readheader prior to aiffReadHeader */
{
    return (firstlong == *(long *)FORM_ID);
}

int is_aiff_formtype(int fd) /* test a long for aiff form ID                 */
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

void aiffReadHeader(            /* Read AIFF header, fill hdr, &  */
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
    int mark_read = 0, inst_read = 0, loops_read = 0;
    int comm_read = 0, ssnd_read = 0;
    long ssnd_offset, ssnd_pos = 0, pos, ckSize;
    short sampsize, nmarkers = 0, nn;
    MARKER  *markersp=NULL, *mp = NULL;
    Loop    *ilp = NULL;
    AIFFDAT *adp = NULL;
    char    *err;
    double  sr, oct;
    /*RWD 3:2000*/
    hdr->peaksvalid = 0;
    hdr->peak_do_rescaling = 1; /* we will have a command arg eventually*/
    p->fscalefac = 1.0f;
    p->do_floatscaling = 0;
    p->filetyp = 0;             /* ensure no bytrevs in sreadin for now */
    if (!is_aiff_form(firstlong))   /* double check it's a form header  */
      die(Str(X_612,"bad form for aiffReadHeader"));         /* & read remainder */
    sreadin(fd,(char *)&form + sizeof(long),sizeof(FormHdr) - sizeof(long),p);
    if (form.formType != *(long *) FORM_TYPE)
      die(Str(X_772,"form header not type aiff"));
    hdr->readlong = FALSE;
    while (1) {                              /* read in the next header */
      if (sreadin(fd,(char *)&ckHdr,sizeof(CkHdr),p) < sizeof(CkHdr))
        break;
      pos = (long)lseek(fd, (off_t)0L, SEEK_CUR);
      if (ckHdr.ckID == *(long *) COMM_ID) {    /* CommChunk hdr: rd rem 1 */
        sreadin(fd,(char *)&comm1 + sizeof(CkHdr), sizeof(short), p);
        sreadin(fd,(char *)&comm2, sizCommChunk2, p); /* + all of part 2 */
/*              printf("ID: numChannels = %d\tnumSampleFrames=%ld(%lx)\n", */
/*                     natshort(comm1.numChannels), */
/*                     natlong(comm2.numSampleFrames), */
/*                     natlong(comm2.numSampleFrames)); */
        sampsize = natshort(comm2.sampleSize);
        if (sampsize <= 8) {            /* parse CommChunk to hdr format */
          hdr->format = AE_CHAR;
          hdr->sampsize = sizeof(char);
        }
        else if (sampsize <= 16) {
          hdr->format = AE_SHORT;
          hdr->sampsize = sizeof(short);
        }
        else if (sampsize <= 24) {
          /*RWD 2:2001 x*/
          /* die(Str(X_186,"AIFF 3-byte samples not supported"));*/
          hdr->format = AE_24INT;
          hdr->sampsize = 3;
        }
        else {
          hdr->format = AE_LONG;
          hdr->sampsize = sizeof(long);
        }
        hdr->nchanls = natshort(comm1.numChannels);
        /* decode 80-bit srate */
        sr = ieee_80_to_double((unsigned char*)comm2.sampleRate);
/*              printf("  : sampleSize=%d\tsampleRate=%f\n", */
/*                     natshort(comm2.sampleSize), sr); */
        hdr->sr = (long) sr;
        comm_read = TRUE;
      }
      else if (ckHdr.ckID == *(long *) MARK_ID) {       /* MarkersChunk: */
        sreadin(fd,(char *)&nmarkers, sizeof(short), p);
#if !defined(WORDS_BIGENDIAN)
        nmarkers = natshort(nmarkers);
#endif
/*              printf("MARK_ID: %d\n", nmarkers); */
        markersp = (MARKER *) mcalloc((long)sizeof(MARKER) * nmarkers);
        for (nn = nmarkers, mp = markersp; nn--; mp++) {  /* for nmarkrs */
          u_char psiz, pstring[256];             /* read ID/postn pair */
          sreadin(fd,(char *)&mp->markerID, sizeof(short), p);
          sreadin(fd,(char *)&mp->position, sizeof(long), p);
          sreadin(fd,(char *)&psiz, 1, p);       /* leave unnatural,   */
          psiz |= 01;                            /*     & skip pstring */
          sreadin(fd, (char *)pstring, (int)psiz, p);
/*                  printf("   :MarkerID=%d(%x); position=%ld(%lx); %*s\n", */
/*                      natshort(mp->markerID), natshort(mp->markerID), */
/*                      natlong(mp->position), natlong(mp->position), */
/*                         psiz, pstring); */
        }
        mark_read = TRUE;
      }
      else if (ckHdr.ckID == *(long *) INST_ID) {       /* Instr Chunk:  */
        int subhdrsiz = sizeof(InstrChunk) - sizeof(CkHdr);
/*                 int i; */
/*                 void *pp = &instr; */
/*              printf("INST_ID\n"); */
        hdr->aiffdata = adp = (AIFFDAT *)mcalloc((long)sizeof(AIFFDAT));
        sreadin(fd,(char *)&instr + sizeof(CkHdr), subhdrsiz, p);
/*              for (i=0; i<sizeof(InstrChunk); i++) { */
/*                  if ((i&31) == 0) putchar('\n'); */
/*                  if ((i&3) == 0) putchar(' '); */
/*                  printf("%.2x", ((char*)pp)[i]); */
/*              } */
/*              putchar('\n'); */
/*              printf("  : baseNote=%2x\tdetune=%2x\tlowNote=%2x\n", */
/*                     instr.baseNote, instr.detune, instr.lowNote); */
/*              printf("  : highNote=%2x\tlowVelocity=%2x\thighVelocity=%2x\n", */
/*                     instr.highNote, instr.lowVelocity, instr.highVelocity); */
/*              printf("  : gain=%d(%x)\n", natshort(instr.gain), */
/*                     natshort(instr.gain)); */
/*              printf("lp: sustain %d start %d(%x) end %d(%x)\n", */
/*                     natshort(instr.sustainLoop.playMode), */
/*                     natshort(instr.sustainLoop.beginLoop), */
/*                     natshort(instr.sustainLoop.beginLoop), */
/*                     natshort(instr.sustainLoop.endLoop), */
/*                     natshort(instr.sustainLoop.endLoop)); */
/*              printf("lp: release %d start %d(%x) end %d(%x)\n", */
/*                     natshort(instr.releaseLoop.playMode), */
/*                     natshort(instr.releaseLoop.beginLoop), */
/*                     natshort(instr.releaseLoop.beginLoop), */
/*                     natshort(instr.releaseLoop.endLoop), */
/*                     natshort(instr.releaseLoop.endLoop)); */
        oct = (instr.baseNote + instr.detune/100.) / 12. + 3.;
        adp->natcps = (float)(pow((double)2.0, oct) * ONEPT);
        adp->gainfac = (float)exp((double)(natshort(instr.gain)) * LOG10D20);
        inst_read = TRUE;
      }
      else if (ckHdr.ckID == *(long *) SSND_ID) {       /* SoundDataHdr: */
        int subhdrsiz = sizeof(SoundDataHdr) - sizeof(CkHdr);
        sreadin(fd,(char *)&ssnd + sizeof(CkHdr), subhdrsiz, p);
/*              printf("SSND_ID: offset=%ld(%lx) blockSize=%ld(%lx)\n", */
/*                     natlong(ssnd.offset), natlong(ssnd.offset), */
/*                     natlong(ssnd.blockSize), natlong(ssnd.blockSize)); */
        ssnd_offset = natlong(ssnd.offset);
        ssnd_pos = pos + subhdrsiz + ssnd_offset;
        hdr->hdrsize = ssnd_pos;
        hdr->audsize = natlong(ckHdr.ckSize) - subhdrsiz - ssnd_offset;
        hdr->filetyp = TYP_AIFF;
        ssnd_read = TRUE;
      }
      else if (ckHdr.ckID == *(long *)NAME_ID ||
               ckHdr.ckID == *(long *)AUTHOR_ID ||
               ckHdr.ckID == *(long *)COPYRIGHT_ID ||
               ckHdr.ckID == *(long *)ANNOTATION_ID) {
        int i=natlong(ckHdr.ckSize);            /* Counter */
        char c[24];            /* Buffer */
        printf("%4s: ", (char *)(&ckHdr.ckID), i);
        while (i>0) {
          int n = (i>=24 ? 24 : i);
          n = read(fd, c, n); /* Skip the chunk */
          printf("%.*s", n, c);
          i -= n;
        }
        printf("\n");
        continue;
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
          die(Str(X_426,"bad size for PEAK chunk in AIFF file"));
        if (comm_read) {
          sreadin(fd,(char *) &version,sizeof(long),p);
#if !defined(WORDS_BIGENDIAN)
          version = benlong(version);
#endif
          if (version != PEAKCHUNK_VERSION)
            die(Str(X_427,"unknown PEAK chunk version in AIFF file"));
          sreadin(fd,(char *) &timestamp,sizeof(long),p);
          hdr->peak_timestamp = benlong(timestamp);
          for (i=0; i < hdr->nchanls;i++) {
            sreadin(fd,(char *) &hdr->peaks[i],sizeof(PositionPeak),p);
#if !defined(WORDS_BIGENDIAN)
            hdr->peaks[i].value = benfloat(hdr->peaks[i].value);
            hdr->peaks[i].position = benlong(hdr->peaks[i].position);
#endif
            if (maxpeak< hdr->peaks[i].value) maxpeak=hdr->peaks[i].value;
          }
          printf(Str(X_1513,"read PEAK data:\ncreation time: %s"),
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
          die(Str(X_1501,"AIFF format error: PEAK chunk found before COMM chunk"));
      }
      else {              /* I do not recognise this chunk */
        int i;            /* Counter */
        int c;            /* Buffer */
        printf(Str(X_509,"unknown chunk %.4s of size %ld\n"),
               (char *)(&ckHdr.ckID), natlong(ckHdr.ckSize));
        for (i=natlong(ckHdr.ckSize); i>0; i--) {
          read(fd, (char*)&c, sizeof(char)); /* Skip the chunk */
        }
        continue;
      }
      if (mark_read && inst_read && !loops_read) {
        ilp = &instr.sustainLoop;
        adp->loopmode1 = natshort(ilp->playMode);
/*              printf("Loops: mode1=%d.....\n", adp->loopmode1); */
        for (nn = nmarkers, mp = markersp; nn--; mp++) {
/*                  printf("%x ==? %x/%x ==? %x\n", */
/*                         mp->markerID,ilp->beginLoop, */
/*                         mp->markerID, ilp->endLoop); */
          if (mp->markerID == ilp->beginLoop)
            adp->begin1 = natlong(mp->position);
          if (mp->markerID == ilp->endLoop)
            adp->end1 = natlong(mp->position);
        }
        ilp = &instr.releaseLoop;
        adp->loopmode2 = natshort(ilp->playMode);
        for (nn = nmarkers, mp = markersp; nn--; mp++) {
/*                  printf("%x ==? %x/%x ==? %x\n", */
/*                         mp->markerID,ilp->beginLoop, */
/*                         mp->markerID, ilp->endLoop); */
          if (mp->markerID == ilp->beginLoop)
            adp->begin2 = natlong(mp->position);
          if (mp->markerID == ilp->endLoop)
            adp->end2 = natlong(mp->position);
        }
        err = NULL;
/*              printf("loop structure: natcps=%f mode1=%d begin1=%ld end1=%ld\n", */
/*                     adp->natcps, adp->loopmode1, adp->begin1, adp->end1); */
/*              printf("loop structure: mode2=%d begin2=%ld end2=%ld\n", */
/*                     adp->loopmode2, adp->begin2, adp->end2); */
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
          printf(Str(X_298,"INFILE ERROR: illegal %s info in aiff file %s\n"),
                 err,fname);
          hdr->aiffdata = NULL;
          mfree((char *)adp);
        }
        mfree(markersp);
        loops_read = TRUE;
      }
      /* if read CommonChunk,SoundDataHdr,Loops, */
      if (comm_read && ssnd_read && loops_read)         /*   we're done */
        break;
      ckSize = natlong(ckHdr.ckSize); /* else seek past this chunk to nxt */
      if (ckSize & 1)  ckSize++;      /*      rnded up to even byte bndry */
      if ((long)lseek(fd, (off_t)(pos + ckSize), SEEK_SET) != pos + ckSize)
        die(Str(X_739,"error while seeking past AIFF chunk"));
    }

/*      if ((adp = hdr->aiffdata) == NULL)
            return; */
    {
      char channame[100];
      switch (hdr->nchanls) {
      case 1:
        strcpy(channame, "");
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
      printf(Str(X_65,"%s: AIFF, %ld %s samples"), fname,
             hdr->audsize/hdr->sampsize/hdr->nchanls, channame);
    }
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
    else {
      printf(Str(X_86,", no looping\n"));
      /* Experimentally add loop info if missing */
      hdr->aiffdata = adp = (AIFFDAT *)mcalloc((long)sizeof(AIFFDAT));
      adp->begin1 = 0; adp->end1 = hdr->audsize/hdr->sampsize/hdr->nchanls;
      adp->loopmode1 = 1;
    }
    if ((long)lseek(fd,(off_t)ssnd_pos,SEEK_SET) != ssnd_pos)
      die(Str(X_731,"error seeking to start of sound data"));
}

/*RWD 3:2000 added retval ,verbose arg: avoid writing empty data */
/*now returns full size of chunk */
/*NB PEAK chunk always requires normalised values*/
int write_aiffpeak(int fd,int verbose)
{
    PeakChunk data;
    int i;
    long size;          /*RWD 3:2000 */
    data.ckID = *(long *) PEAK_ID;
    data.chunkDataSize =
      sizeof(PeakChunk) + (nchnls-1)*sizeof(PositionPeak) - 2*sizeof(long);
        /*RWD 3:2000: must also be big-endian */
    size = data.chunkDataSize + 2* sizeof(long);
    data.chunkDataSize = benlong(data.chunkDataSize);
    data.version = benlong(1);
    data.timeStamp = benlong(time(0));
    write(fd, (char *)&data, sizeof(PeakChunk)-sizeof(PositionPeak));
    for (i=0; i<nchnls; i++) {
      /* RWD 3:2000 PEAK chunk must reflect clipping and truncation */
      data.peak[0].value = (float)(omaxamp[i] * dbfs_to_float);
/*       data.peak[0].value = (float)(omaxamp[i]/FL(32768.0)); */
      if (O.informat == AE_SHORT) {
        if (data.peak[0].value > 1.0f) data.peak[0].value = 1.0f;
        if (data.peak[0].value < MIN_SHORTAMP)
          data.peak[0].value = 0.0f;
      }
      /* RWD 6:2001 and 24bit too */
      else if (O.informat == AE_24INT) {
        if (data.peak[0].value>1.0f) data.peak[0].value = 1.0f;
        if (data.peak[0].value < MIN_24AMP)
          data.peak[0].value = 0.0f;
      }
      else if (O.informat == AE_LONG) {
        if (data.peak[0].value > 1.0f) data.peak[0].value = 1.0f;
        if (data.peak[0].value < MIN_LONGAMP)
          data.peak[0].value = 0.0f;
      }
          /* RWD 3:2000: print info then byte-reverse */
      if (verbose)
        printf(Str(X_1515,"peak CH %d: %f  (written: %f) at %ld\n"),
               i+1, omaxamp[i],data.peak[0].value, omaxpos[i]);
      data.peak[0].value = benfloat(data.peak[0].value);
      data.peak[0].position = benlong(omaxpos[i]);
      if (write(fd, (char *)&data.peak, sizeof(PositionPeak))<0)
        return 0;                /*RWD 3:2000*/
    }
    return size;                        /*RWD 3:2000*/
}
