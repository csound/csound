/*  
    wave.c:

    Copyright (C) 1995, 2001 Barry Vercoe, Richard Dobson, John ffitch

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

#ifndef SNDLIBFILE

#include        "cs.h"                            /*                WAVE.C   */
#include        "soundio.h"
#include        "wave.h"
#include        "sfheader.h"
#include        <math.h>
#include        <time.h>                  /*RWD 3:2000 moved here */
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifndef TRUE
#define TRUE    1
#define FALSE   0
#endif

static struct wav_head formhdr;
static long   framesize;
/* RWD.2.98 to avoid recalculating  variable  headersizes */
static long     this_hdrsize    = 0;
static short    this_format     = 0;
static long     datasize_offset = 0;
static off_t    factchunk_offset= 0;
/*RWD 3:2000*/
static off_t    peakchunk_offset = 0;
static float    natlfloat(float x);
/*RWD 3:2000 moved here */
static int      write_wavpeak(int fd, int verbose);
static float    lenfloat(float x);

static long     nchans = 0;     /* because not enough info passed to wavReWriteHdr() ! */

#ifdef WORDS_BIGENDIAN
static short lenshort(short sval) /* coerce a natural short into a littlendian short */
{
    char  benchar[2];
    char *p = benchar;

    *p++ = 0xFF & sval;
    *p   = 0xFF & (sval >> 8);
    return(*(short *)benchar);
}

static long lenlong(long lval)     /* coerce a natural long into a littlendian long */
{
    char  benchar[4];
    char *p = benchar;

    *p++ = (char)(0xFF & lval);
    *p++ = (char)(0xFF & (lval >> 8));
    *p++ = (char)(0xFF & (lval >> 16));
    *p   = (char)(0xFF & (lval >> 24));
    return(*(long *)benchar);
}

static short natlshort(short sval) /* coerce a littlendian short into a natural short */
{
    unsigned char benchar[2];
    short natshort;

    *(short *)benchar = sval;
    natshort = benchar[1];
    natshort <<= 8;
    natshort |= benchar[0];
    return(natshort);
}

static long natllong(long lval)     /* coerce a littlendian long into a natural long */
{
    unsigned char benchar[4];
    unsigned char *p = benchar + 3;
    long natlong;

    *(long *)benchar = lval;
    natlong = *p--;
    natlong <<= 8;
    natlong |= *p--;
    natlong <<= 8;
    natlong |= *p--;
    natlong <<= 8;
    natlong |= *p;
    return(natlong);
}
#else
# define lenshort(x)  (x)
# define lenlong(x)   (x)
# define natlshort(x) (x)
# define natllong(x)  (x)
#endif

/* RWD.2.98 many changes to set correct variable-header length etc */
void wavWriteHdr(               /* Write WAV header at start of file.  */
    int fd,                     /* Called after open, before data writes*/
    int sampsize,               /* sample size in bytes */
    int nchls,
    double sr)                  /* sampling rate */
{
        long databytes;
        PeakChunk peakdata;             /*RWD 3:2000*/
        int i;

#ifdef DEBUG
        printf("wavWriteHdr: fd %d sampsize %d nchls %d sr %f\n",
                fd,sampsize,nchls,sr);
#endif
        framesize = sampsize * nchls;
        databytes = 0;          /* reset later by wavReWriteHdr */

        formhdr.magic = *((long *)RIFF_ID);
        formhdr.len0 = lenlong(databytes + 36/*WAVHDRSIZ*/);
        formhdr.magic1 = *((long *)WAVE_ID);
        formhdr.magic2 = *((long *)FMT_ID);
        /*formhdr.len = lenlong((long)16); */        /* length of format chunk */
/* RWD.2.98 */
        this_format = (short)(sampsize==4 ? (O.outformat==AE_FLOAT ? 3 : 1) : 1);
        formhdr.format = lenshort(this_format); /* PCM */
        formhdr.len = this_format == 3 ? lenlong(18L) : lenlong(16L);
        nchans = nchnls;       /* RWD for wavReWriteHdr() for fact chunk */
        formhdr.nchns = lenshort((short)nchls);
        formhdr.rate = lenlong((long)sr);
        formhdr.aver = lenlong((long)(sr * framesize));
        formhdr.nBlockAlign = lenshort((short)framesize); /* bytes per frame */
        formhdr.size = lenshort((short)(8 * sampsize)); /* bits per sample */
        /* RWD.2.98 have to do these later */
#if 0
        formhdr.magic3 = *((long *)DATA_ID);
        formhdr.datasize = lenlong(databytes);
#endif
        if (write(fd, (char *)&formhdr, sizeof(formhdr)) != sizeof(formhdr))
            die(Str(X_743,"error writing WAV header"));
        /* RWD add cbSize if format = 3 */
        if (this_format==3) {
          short cbSize = 0;
          long factid = *((long *)FACT_ID);
          long chunksize = lenlong( sizeof(long));
          long factdata = 0;    /* filled in later... */
          if ((write(fd,(char *) &cbSize,sizeof(short)) != sizeof(short)) ||
              (write(fd,(char *)&factid,sizeof(long)) <0)                 ||
              (write(fd,(char *)&chunksize,sizeof(long)) <0)              ||
              ((factchunk_offset = lseek(fd,(off_t)0,SEEK_CUR)) < 0)      ||
              (write(fd,(char *)&factdata,sizeof(long)) < 0))
            die(Str(X_743,"error writing WAV header"));
          /* add statutory fact chunk for non-PCM formats */
        }
        if (peakchunks) {
          /*RWD 3:2000 create space for PEAK chunk */
          if ((peakchunk_offset = lseek(fd,(off_t)0,SEEK_CUR)) < 0)
            die(Str(X_743,"error writing WAV header"));
          peakdata.ckID = *(long *) PEAK_ID;
          peakdata.chunkDataSize = lenlong(sizeof(PeakChunk) + (nchnls-1)*sizeof(PositionPeak));
          peakdata.version = lenlong(1);
          peakdata.timeStamp = lenlong(0); /*dummy time to start with */
          if (write(fd, (char *)&peakdata,
                    sizeof(PeakChunk)-sizeof(PositionPeak)) < 0)
            die(Str(X_743,"error writing WAV header"));
          /*write some dummy data to start with */
          for (i=0; i<nchnls; i++) {
            peakdata.peak[0].value = lenfloat(0.0f);
            peakdata.peak[0].position = lenlong(0);
            if (write(fd, (char *)&peakdata.peak, sizeof(PositionPeak)) < 0)
              die(Str(X_743,"error writing WAV header"));
          }
        }
        /* RWD.2.98 then write data chunk info */
        {
          long magic3 = *((long *)DATA_ID);
          long datasize = lenlong(databytes);
          if ((write(fd,(char *)&magic3,  sizeof(long)) != sizeof(long)) ||
              ((datasize_offset = lseek(fd,(off_t)0,SEEK_CUR)) < 0)             ||
              (write(fd,(char *)&datasize,sizeof(long)) != sizeof(long)))
            die(Str(X_743,"error writing WAV header"));
        }
        this_hdrsize = datasize_offset+sizeof(long);
}

/* RWD: WHY aren't the same params passed here as to wavWriteHdr() ???? */
void wavReWriteHdr(int fd,        /* Write proper sizes into WAV header  */
                   long datasize, /*        called before closing file   */
                   int verbose)
{                                 /*        & optionally under -R        */
    long  endpos = lseek(fd,(off_t)0L,SEEK_CUR);
    /* RWD.2.98: all changed to deal with variable headersize */
    long length = lenlong(endpos-8);
    long size = lenlong(datasize);
    long size_in_samps = lenlong((datasize / sizeof(float)) / nchans);/* for fact chunk */
    if (endpos != datasize + this_hdrsize) {
      die(Str(X_900,"inconsistent WAV size")); /* RWD.2.98: should be able to fudge something! */
    }
    if (lseek(fd, (off_t)4L, SEEK_SET)<0                         ||
        write(fd, (char *)&length, sizeof(long)) != sizeof(long) ||
        (lseek(fd,(off_t)datasize_offset,SEEK_SET) < 0)          ||
        write(fd,(char *)&size,sizeof(long)) != sizeof(long))
      die(Str(X_729,"error rewriting WAV header"));
    /* update fact chunk if non-PCM format */
    if (this_format==(short)3)
      if ((lseek(fd,(off_t)factchunk_offset,SEEK_SET) <0) ||
          (write(fd,(char *)&size_in_samps,sizeof(long)) != sizeof(long)))
        die(Str(X_729,"error rewriting WAV header"));
    if (peakchunks) {
      /*RWD 3:2000 complete PEAK chunk */
      if ((lseek(fd,peakchunk_offset,SEEK_SET) < 0) || write_wavpeak(fd, verbose))
        die(Str(X_729,"error rewriting WAV header"));
    }
    lseek(fd, (off_t)endpos, SEEK_SET);
}

int is_wav_form(long firstlong) /* test a long for wav form ID                 */
                                /* called by readheader prior to wavReadHeader */
{
    return (firstlong == *(long *)RIFF_ID);
}

void wavReadHeader(             /* Read WAV header, fill hdr, & */
    int fd,                     /* postn rd ptr to start of samps*/
    char *fname,
    HEADATA *hdr,               /* datablock for passing data back */
    long firstlong,
    SOUNDIN *p)
{
        struct wav_head form;
                /*RWD 3:2000*/
        hdr->peaksvalid = 0;
        hdr->peak_do_rescaling = 1;     /* we will have a command arg eventually*/
        p->fscalefac = 1.0f;
        p->do_floatscaling = 0;
        p->filetyp = 0;                 /* ensure no bytrev on sreadin here */
        if (!is_wav_form(firstlong))    /* double check it's a form header */
            die(Str(X_613,"bad form for wavReadHeader"));         /* & read remainder */
        sreadin(fd,(char *)&form + sizeof(long),sizeof(form) - sizeof(long),p);
        if (form.magic1 != *(long *) WAVE_ID) {
            err_printf( Str(X_291,"Got form.magic = %lx\n"), form.magic);
            die(Str(X_773,"form header not type wav"));
        }
        hdr->sr = natllong(form.rate);
        hdr->nchanls = natlshort(form.nchns);
        hdr->sampsize = (natlshort(form.size)) / 8;
        hdr->format = (hdr->sampsize == 2 ? AE_SHORT :
                       hdr->sampsize == 1 ? AE_UNCH :
                       hdr->sampsize == 3 ? AE_24INT :     /*RWD 2001 */
                       form.format == 3 ? AE_FLOAT :
                       AE_LONG);
        hdr->hdrsize = sizeof(form);
        hdr->filetyp = TYP_WAV;
        hdr->aiffdata = NULL;
        /* hdr->audsize = natllong(form.datasize);  RWD.2.98 see below... */
        hdr->readlong = FALSE;
        hdr->firstlong = 0;
        /* RWD.2.98 check for cbSize field */
        if (form.len==18) {
          short cbSize;
          if (read(fd,(void*)&cbSize,sizeof(short)) <0)
            die(Str(X_732,"error skipping unknown chunk in WAV file"));
          if (cbSize>0)
            die(Str(X_727,"error reading format data: is this a compressed file?"));
        }
/* RWD.2.98 now step through all following chunks, until we find data chunk */
        {
          long chunk_id, chunksize;
          if ((read(fd,(void *)&chunk_id,sizeof(long)) < 0)   ||
              (read(fd,(void *) &chunksize,sizeof(long)) < 0) ||
              natllong(chunksize) < 0)  /*TODO: can we have zero-sized chunks???? */
            die(Str(X_728,"error reading unknown chunk in WAV file"));
          while (chunk_id != *(long*)DATA_ID) {
            if (chunk_id == *(long*)SMPL_ID) {
              SMPL smpl_chunk;
              smpl_chunk.chunkSize = natllong(chunksize);
/*                printf("Found smpl chunk\n"); */
              if (read(fd,(void*)&smpl_chunk.dwManufacturer,
                       sizeof(SMPL)-2*sizeof(long)) < 0)
                die(Str(X_732,"error skipping unknown chunk in WAV file"));
            }
                        /*RWD 3:2000 */
            else if (chunk_id == *(long *) PEAK_ID) {
              int i;
              long version, timestamp,peak_chunksize,expected_size;
              float maxpeak = 0.0f;
              peak_chunksize = natllong(chunksize);
              expected_size = sizeof(PeakChunk) - 2 * sizeof(long);
              expected_size += (hdr->nchanls -1) * sizeof(PositionPeak);
              if (peak_chunksize!=expected_size)
                die(Str(X_1509,"bad size for PEAK chunk in WAV file"));
              if (read(fd,(char *) &version,sizeof(long)) < 0)
                die(Str(X_1510,"error reading PEAK chunk in WAV file"));
              version = natllong(version);
              if (version != PEAKCHUNK_VERSION)
                die(Str(X_1511,"unknown PEAK chunk version in WAV file"));
              if (read(fd,(char *) &timestamp,sizeof(long)) < 0)
                die(Str(X_1512,"error reading PEAK chunk in WAV file"));
              hdr->peak_timestamp = natllong(timestamp);
              for (i=0; i < hdr->nchanls;i++) {
                if (read(fd,(char *) &hdr->peaks[i],sizeof(PositionPeak))< 0)
                  die("error reading PEAK chunk in WAV file");
#ifdef WORDS_BIGENDIAN
                hdr->peaks[i].value = natlfloat(hdr->peaks[i].value);
                hdr->peaks[i].position = natllong(hdr->peaks[i].position);
#endif
                if (maxpeak < hdr->peaks[i].value) maxpeak = hdr->peaks[i].value;
              }
              printf(Str(X_1513,"Read PEAK data:\ncreation time: %s"),
                     ctime((time_t *) &(hdr->peak_timestamp)));
              for (i=0;i < hdr->nchanls;i++) {
                printf(Str(X_1514,
                           "CH %d: peak = %.6f at sample %d: %.4lf secs\n"),
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
 /*RWD 3:2000 NB changed: chunk read must be called each pass */
            else if (lseek(fd,(off_t)natllong(chunksize),SEEK_CUR) < 0)
              die(Str(X_732,"error skipping unknown chunk in WAV file"));
            if (read(fd,(void*)&chunk_id,sizeof(long)) < 0  ||
                read(fd,(void*)&chunksize,sizeof(long)) < 0 ||
                natllong(chunksize) < 0) {
              die(Str(X_728,"error reading unknown chunk in WAV file"));
            }
          }
          hdr->audsize = natllong(chunksize);
          hdr->hdrsize = (long)lseek(fd,(off_t)0,SEEK_CUR);    /* RWD Apr 97 */
        }
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
          printf(Str(X_68,"%s: WAVE, %ld %s samples\n"), fname,
                 hdr->audsize/hdr->sampsize/hdr->nchanls, channame);
        }

}

#ifdef mills_macintosh
void wavResetFrameSize(int sampsize, int nchanls) {
    framesize = sampsize * nchanls;
    formhdr.aver = lenlong((long)(formhdr.rate * framesize));
    formhdr.nBlockAlign = lenshort((short)framesize);  /* ??? */
    formhdr.size = lenshort((short)(8 * sampsize));
}
#endif


static float lenfloat(float x)
{
    union { long l ; float f; } cheat;
    cheat.f = x; cheat.l = lenlong(cheat.l); return cheat.f;
}

/*RWD 3:2000 need this too for PEAK chunk*/
static float natlfloat(float x)
{
    union { long l ; float f; } cheat;
    cheat.f = x; cheat.l = natllong(cheat.l); return cheat.f;

}


/*RWD 3:2000 added retval (0 = success), adjusted size field calc*/
static int write_wavpeak(int fd, int verbose)
{
    PeakChunk data;
    int i;
    data.ckID = *(long *) PEAK_ID;
    data.chunkDataSize = lenlong(sizeof(PeakChunk) + (nchnls-1)*sizeof(PositionPeak)
                                 - 2 * sizeof(long));
        /*RWD 3:2000 chunk size excludes tag and size fields*/
    data.version = lenlong(1);
    data.timeStamp = lenlong(time(NULL));
    write(fd, (char *)&data, sizeof(PeakChunk)-sizeof(PositionPeak));
    for (i=0; i<nchnls; i++) {
      /*RWD 3:2000 peak vals are always normalised */
      /* NB this would be 32767, not 32768. ~shouldn't~ matter...*/
      data.peak[0].value = (float)omaxamp[i]/(float)e0dbfs;
      /* PEAK chunk must reflect clipping and truncation */
      if (O.informat == AE_SHORT) {
        if (data.peak[0].value>1.0f) data.peak[0].value = 1.0f;
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
        if (data.peak[0].value>1.0f) data.peak[0].value = 1.0f;
        if (data.peak[0].value < MIN_LONGAMP)
          data.peak[0].value = 0.0f;
      }
          /*RWD 3:2000 write info, then reverse data*/
      if (verbose)
        printf(Str(X_1515,"peak CH %d: %f  (written: %f) at %ld\n"),
               i+1,omaxamp[i],data.peak[0].value, omaxpos[i]);
#if !defined(WORDS_BIGENDIAN)
      data.peak[0].value = lenfloat(data.peak[0].value);
#endif
      data.peak[0].position = lenlong(omaxpos[i]);
      if (write(fd, (char *)&data.peak, sizeof(PositionPeak)) < 0)
        return 1;       /*RWD 3:2000 */
    }
    return 0;      /*RWD 3:2000 */
}

#endif
