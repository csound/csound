/*  
    sfheader.c:

    Copyright (C) 1991 Barry Vercoe, John ffitch, matt ingalls

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

#include "cs.h"                         /*             SFHEADER.C       */
#include "soundio.h"
#include <stdio.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

extern OPARMS O;

#ifdef SFIRCAM

#include "sfheader.h"
static  char    *incodbeg,  *incodend;   /* re-defined by each readheader */
static  char    *outcodbeg, *outcodend;  /* defined during writeheader */

typedef struct {                  /* header for each sfcode block */
        short   type;
        short   blksiz;
} CODE_HDR;

static short codblksiz[] = {   sizeof(CODE_HDR),
                               sizeof(CODE_HDR) + sizeof(SFMAXAMP),
                               sizeof(CODE_HDR) + sizeof(SFAUDIOENCOD),
                               sizeof(CODE_HDR) + sizeof(SFPVDATA),
                               sizeof(CODE_HDR) + sizeof(SFCOMMENT)  };

static void putendcode(char *cp)
{
        CODE_HDR *cdhp = (CODE_HDR *)cp;
        cdhp->type = SF_END;
        cdhp->blksiz = sizeof(CODE_HDR);  /* header, no body */
}

/* These next rtn ptr to beg of struct reqd (not the CODE_HDR preceding it).
   SR, NCHNLS, magic number, bytes/chan, are NOT coded via these routines. */

char *findsfcode(int ctype)     /* locate start of sfcode in current in_header */
                                /*    incodbeg,incodend prev set by readheader */
{                               /*      used here,  & by ugens8.c (PVOC)       */
        char     *cp;
        CODE_HDR *cdhp;

        if (ctype <= 0 || ctype > SF_CODMAX)
            die(Str(X_883,"illegal sfcode type"));
        for (cp = incodbeg; cp < incodend;) { /* starting from beg codespace */
            cdhp = (CODE_HDR *)cp;
            if (cdhp->type == ctype)             /* if find required code */
                return(cp + sizeof(CODE_HDR));   /*   return ptr to data  */
            if (cdhp->type == SF_END)            /* cannot find -- exit    */
                break;
            if (cdhp->blksiz <= 0                /* if false-sized struct or */
             || (cp += cdhp->blksiz) < incodbeg) {/* wrap-around from bad hdr */
                err_printf(Str(X_1186,"sfheader codes corrupted\n"));    /* complain */
                break;
            }
        }
        return(NULL);                        /* no-find: return NULL pointer */
}

char *creatsfcode(int ctype)    /* add a new sfcode struct to current out_header */
                                /*   outcodbeg,outcodend prev set by writeheader */
{
        char     *cp;
        CODE_HDR *cdhp;

        if (ctype <= 0 || ctype > SF_CODMAX)
            die(Str(X_883,"illegal sfcode type"));
        for (cp=outcodbeg; cp<outcodend; ) { /* starting from beg codespace  */
            cdhp = (CODE_HDR *)cp;
            if (cdhp->type == SF_END) {             /* if find end code      */
                cdhp->type = ctype;                 /*   redo as newtyp hdr  */
                cdhp->blksiz = codblksiz[ctype];
                putendcode(cp+cdhp->blksiz);        /*   reconstruct endcode */
                return(cp+sizeof(CODE_HDR));        /*   & rtn newtyp datptr */
            }
            if (cdhp->blksiz <= 0                /* if false-sized struct or */
             ||(cp += cdhp->blksiz) < outcodbeg) {/* wrap-around from bad hdr */
                err_printf(Str(X_1186,"sfheader codes corrupted\n"));    /* complain */
                break;
            }
        }
        return(NULL);                       /* bad ptrs: return NULL pointer */
}

#endif

#ifndef _SNDFILE_

#ifdef NeXT
#include <sound/soundstruct.h>
#endif /* NeXT */

#ifdef SFSUN41
#define INFOMAX   100    /* blksiz for SUN info data */
static  char    *ininfop, *outinfop;   /* blks of INFOMAX bytes for SUN info */
#endif

#ifdef SFIRCAM
static  char    *inhdrblk, *outhdrblk;  /* (whichever) soundfile header blks */
#endif
static  HEADATA HeadData;               /* datablk for general return        */
extern  void    aiffReadHeader(int,char *,HEADATA *,long,SOUNDIN *);
extern  void    aifcReadHeader(int,char *,HEADATA *,long,SOUNDIN *);
extern  void    wavReadHeader(int,char *,HEADATA *,long,SOUNDIN *);
extern  int     is_aiff_form(long), is_wav_form(long);
extern  int     is_aiff_formtype(int), is_aifc_formtype(int);
extern  char    *getstrformat(int);

HEADATA *readheader(            /* read soundfile hdr, fill HEADATA struct */
    int ifd,                    /*   called by sfopenin() and sndinset()   */
    char *sfname,               /* NULL => no header, nothing to preserve  */
    SOUNDIN *p)
{
    long    headfirstlong;
    HEADATA *hdp = &HeadData;
#if !defined(mills_macintosh) && !defined(SYMANTEC)
    static struct   stat statbuf;
#endif
    memset(hdp, 0, sizeof(HEADATA)); /* Clear structure */
                                                        /* read the 1st long */
    sreadin(ifd, (char *)&headfirstlong, sizeof(long), p);
    if (is_aiff_form(headfirstlong)) {             /* if AIFF/AIFC form  */
      if (is_aiff_formtype(ifd)) {
        aiffReadHeader(ifd,sfname,hdp,headfirstlong,p);
                /*RWD 3:2000*/
        p->sr = hdp->sr;
        p->nchanls = (short) hdp->nchanls;
        p->filetyp = TYP_AIFF;
        p->getframes = hdp->audsize/hdp->sampsize;
        return(hdp);
      }
      else if (is_aifc_formtype(ifd)) {             /* if AIFC form       */
        aifcReadHeader(ifd,sfname,hdp,headfirstlong,p);  /*   read hdr.. */
        /*RWD 3:2000*/
        p->sr = hdp->sr;
        p->nchanls = (short) hdp->nchanls;
        p->filetyp = TYP_AIFC;
        p->getframes = hdp->audsize/hdp->sampsize;
        return(hdp);                                 /*   and report back  */
      }
    }
    else if (is_wav_form(headfirstlong)) {         /* if WAV form        */
      wavReadHeader(ifd,sfname,hdp,headfirstlong,p);   /*   read hdr.. */
      p->sr = hdp->sr;
      p->nchanls = (short) hdp->nchanls;
      p->filetyp = TYP_WAV;
      p->getframes = hdp->audsize/hdp->sampsize;
      return(hdp);                               /*   and report back  */
    }
    else hdp->filetyp = 0; {        /* else neither AIFF nor WAV   */
#if !defined(mills_macintosh) && !defined(SYMANTEC)
# ifdef LATTICE
      stat(retfilnam, &statbuf);          /* This is a cheat and may fail */
# else
      fstat(ifd, &statbuf);
#endif
      hdp->audsize = statbuf.st_size;       /* audsize including header */
#endif
    }
#ifdef SFIRCAM
    if (headfirstlong == SF_MAGIC) {      /* if IRCAM header, read rem */
      SFHEADER *sfh;
      SFAUDIOENCOD *aep;
      
      if (inhdrblk == NULL)
        inhdrblk = mmalloc((long)sizeof(SFHEADER));
      sfh = (SFHEADER *) inhdrblk;
      sreadin(ifd, inhdrblk+sizeof(long), sizeof(SFHEADER)-sizeof(long), p);
      hdp->sr = (long) sfsrate(sfh);
      hdp->nchanls = sfchans(sfh);        /*   and record the data */
      hdp->sampsize = sfclass(sfh);
      incodbeg = &sfcodes(sfh);
      incodend = (char *)sfh + sizeof(SFHEADER);
      if ((aep = (SFAUDIOENCOD *) findsfcode(SF_AUDIOENCOD)) != NULL)
        hdp->format = aep->encoding;
      else {                       /* if no audioencode info,          */
        switch (hdp->sampsize) { /* FORMAT BASED ONLY ON BYTE-COUNT! */
#ifdef ULAW
        case SF_ULAW:   hdp->format = AE_ULAW;   break;
#endif
        case SF_SHORT:  hdp->format = AE_SHORT;  break;
        case SF_FLOAT:  hdp->format = AE_FLOAT;  break;
        case SF_DOUBLE: hdp->format = AE_DOUBLE;  break;
        case SF_24INT:  hdp->format = AE_24INT;  break;
        default:        hdp->format = O.informat;
        }
        if (O.msglevel & WARNMSG) {
          printf(Str(X_605,"WARNING: audio_in %s format unclear, deducing %s\n"),
                 sfname, getstrformat((int)hdp->format));
        }
      }
      hdp->hdrsize = sizeof(SFHEADER);   /* hdrsize (for later seeks)  */
      hdp->audsize -= sizeof(SFHEADER);
      hdp->readlong = 0;                 /* now aligned on audio_start */
    }
    else if (BYTREVL(headfirstlong) == SF_MAGIC) {  /* else byte-rev: bad */
      if (O.msglevel & WARNMSG) {
        printf(Str(X_57,"WARNING: %s is soundfile with bytes in the wrong order\n"),sfname);
      }
      return(NULL);
    }
    else {
      hdp->hdrsize = 0;                  /* else no header,        */
      hdp->readlong = 1;                 /*   but we read a long   */
      hdp->firstlong = headfirstlong;    /*   which had this value */
    }
    return(hdp);                /* SFIRCAM always returns hdp, and data */
#elif defined(NeXT)
    if (NXSwapBigIntToHost(headfirstlong) == SND_MAGIC) { /* if IRCAM header, read rem */
      SNDSoundStruct *sfh;
      int  n;
      void  (*bytrevtemp)(void);
      
      if (inhdrblk == NULL)
        inhdrblk = mmalloc((long)sizeof(SNDSoundStruct));
      sfh = (SNDSoundStruct *) inhdrblk;
#ifdef __LITTLE_ENDIAN__        /* reverse header */
      bytrevtemp = p->bytrev;
      p->bytrev = bytrev4;
#endif
      sreadin(ifd, inhdrblk+sizeof(long),
              sizeof(SNDSoundStruct)-sizeof(long), p);
#ifdef __LITTLE_ENDIAN__
      p->bytrev = bytrevtemp;
#endif
      hdp->sr = sfh->samplingRate;
      hdp->nchanls = sfh->channelCount;
      switch(sfh->dataFormat) {   /* map NeXT formats to ours */
#ifdef ULAW
      case SND_FORMAT_MULAW_8:   hdp->format = AE_ULAW;
        hdp->sampsize = 1;      break;
#endif
     case SND_FORMAT_LINEAR_8:  hdp->format = AE_CHAR;
        hdp->sampsize = 1;      break;
      case SND_FORMAT_LINEAR_16: hdp->format = AE_SHORT;
        hdp->sampsize = 2;      break;
      case SND_FORMAT_LINEAR_32: hdp->format = AE_LONG;
        hdp->sampsize = 4;      break;
      case SND_FORMAT_FLOAT:     hdp->format = AE_FLOAT;
        hdp->sampsize = 4;      break;
      default:        hdp->format = O.informat;
      }
      hdp->hdrsize = sfh->dataLocation;
      hdp->audsize -= sfh->dataLocation;
      if (sfh->dataLocation > sizeof(SNDSoundStruct))  /* skip rem hdr */
        lseek(ifd, (off_t)(sfh->dataLocation-sizeof(SNDSoundStruct)),
              SEEK_CUR);
      hdp->readlong = 0;                 /* now aligned on audio_start */
    }
    else if (BYTREVL(headfirstlong) == SND_MAGIC) { /* else byte-rev: bad */
      if (O.msglevel & WARNMSG)
        printf(Str(X_57,"WARNING: %s is soundfile with bytes in the wrong order\n"),sfname);
      return(NULL);
    }
    else {
      hdp->hdrsize = 0;                  /* else no header,        */
      hdp->readlong = 1;                 /*   but we read a long   */
      hdp->firstlong = headfirstlong;    /*   which had this value */
    }
    return(hdp);
#elif defined(SFSUN41)
    if (inhdrblk == NULL) {
      inhdrblk = mmalloc((long)sizeof(Audio_hdr));
# ifndef sol
      ininfop = mcalloc((long)INFOMAX);
# endif
    }
    if (audio_isaudiofile(sfname) == TRUE) {
      Audio_hdr *ahp = (Audio_hdr *) inhdrblk;
      char *infop = NULL;
      unsigned ilen;
      if (audio_read_filehdr(ifd,ahp,ininfop, INFOMAX) != AUDIO_SUCCESS)
        die(Str(X_726,"error reading audio_filehdr"));
# ifdef sol
      lseek(ifd,(off_t)0,SEEK_SET);                /* rewind audio file */
# endif
      hdp->sr = ahp->sample_rate;
      hdp->sampsize = ahp->bytes_per_unit;     /* record the data */
      hdp->nchanls = ahp->channels;
      switch (ahp->encoding) {                /* Convert format code */
#ifdef never
      case AUDIO_ENCODING_ALAW:
        hdp->format = AE_ALAW;   break;
#endif
#ifdef ULAW
      case AUDIO_ENCODING_ULAW:
        hdp->format = AE_ULAW;   break;
#endif
      case AUDIO_ENCODING_LINEAR:
        switch (ahp->bytes_per_unit) {
        case 1:     hdp->format = AE_CHAR;  break;
        case 2:     hdp->format = AE_SHORT;  break;
          /*RWD 5:2001*/
        case 3:     hdp->format = AE_24INT; break;
        case 4:     hdp->format = AE_LONG;  break;
        default:    sprintf(errmsg,
                            Str(X_1322,"unexpected audio input length of %d (linear)"),
                            (int) ahp->bytes_per_unit);
          die(errmsg);
        }
        break;
      case AUDIO_ENCODING_FLOAT:
        switch (ahp->bytes_per_unit) {
        case 4:     hdp->format = AE_FLOAT;  break;
        default:    sprintf(errmsg,
                            Str(X_1321,"unexpected audio input length of %d (float)"),
                            (int) ahp->bytes_per_unit);
          die(errmsg);
        }
        break;
      default:
#ifdef ULAW
        hdp->format = AE_ULAW;
#else
        hdp->format = AE_SHORT;
#endif
        if (O.msglevel & WARNMSG) {
          if (O.msglevel & WARNMSG)
            printf(Str(X_605,"WARNING: audio_in %s format unclear, deducing %s\n"),
                  sfname, getstrformat((int)hdp->format));
        }
        break;
      }
      hdp->hdrsize = -1;               /* don't know real headdata size */
      hdp->audsize -= sizeof(Audio_hdr); /* but audio cannot include hdr */
      hdp->readlong = 0;                 /* now aligned on audio_start  */
      return(hdp);
    }
    else return(NULL);
#elif defined(mac_classic) || defined(SYMANTEC)
    {
      MYFLT   fsr;               /* file sample rate */
      int     fnch, fbpd;        /* file channels & bytes per datum */
      
      if (ReadMacHeader(sfname,&fnch,&fsr,&fbpd) == 0) {  /* get rsrc */
        hdp->sr = fsr;
        hdp->nchanls = fnch;                         /*  record data */
        hdp->sampsize = fbpd;
        hdp->format = (fbpd == 4)?AE_FLOAT: AE_SHORT; /* 2 poss fmts */
        /*RWD 5:2001 ??? */
        if (fbpd ==3)
          hdp->format = AE_24INT;
        /*anything else to do?*/
        
        hdp->hdrsize = 0;                     /* no header on file   */
        hdp->audsize = lseek( ifd, (off_t)0L, SEEK_END); /* find eof */
        lseek(ifd, (off_t)0L, SEEK_SET); /* now go back to start of file */
        hdp->readlong = 0;                /* aligned to audio_start  */
        return(hdp);
      }
      else return(NULL);
    }
#else
    hdp->hdrsize = 0;                  /* else no header reader, */
    hdp->readlong = 1;                 /*   but we read a long   */
    hdp->firstlong = headfirstlong;    /*   which had this value */
    return(hdp);                       /* so return hdp and data */
#endif
}

extern void aiffWriteHdr(int, int, int, double);
extern void aifcWriteHdr(int, int, int, double);
extern void wavWriteHdr(int, int, int, double);

void writeheader(int ofd, char *ofname)  /* write an sfheader struct into output stream */
                                /*        called only by sfopenout()           */
{
    if (ofd<0) return;          /* For /dev/null device */
    if (O.filetyp) {
        if (O.filetyp == TYP_AIFF) {
                /*RWD 3:2000 AIFF cannot contain floats: use AIFF-C */
          if (O.outformat== AE_FLOAT) {
            O.filetyp= TYP_AIFC;
            aifcWriteHdr(ofd,O.outsampsiz,nchnls,esr);
          }
          else
            aiffWriteHdr(ofd,O.outsampsiz,nchnls,esr);
        }
        else if (O.filetyp == TYP_AIFC) {
          aifcWriteHdr(ofd,O.outsampsiz,nchnls,esr);
        }
        else if (O.filetyp == TYP_WAV)
            wavWriteHdr(ofd,O.outsampsiz,nchnls,esr);
    }
    else {
#ifdef SFIRCAM
        SFHEADER *sfh;
        int n;
        SFAUDIOENCOD *aep;
        outhdrblk = mcalloc((long)sizeof(SFHEADER));    /* allocate hdr blk */
        sfh = (SFHEADER *)outhdrblk;
        sfmagic(sfh) = SF_MAGIC;
        sfsrate(sfh) = (float)esr;                      /*  assgn headrvals */
        sfchans(sfh) = nchnls;
        sfclass(sfh) = O.outsampsiz;
        outcodbeg = &sfcodes(sfh);                      /* set sfcode limits */
        outcodend = (char *)sfh + sizeof(SFHEADER);
        putendcode(outcodbeg);                          /* initial sfendcode */
        if ((aep = (SFAUDIOENCOD *) creatsfcode(SF_AUDIOENCOD)) != NULL) {
            aep->encoding = O.outformat;                /*  ..add encode blk */
            aep->grouping = 1;
        }
        if ((n = write(ofd,(char*)sfh,sizeof(SFHEADER))) < sizeof(SFHEADER))
                die(Str(X_1200,"soundfile header write error.  aborting ..."));
#elif defined(NeXT)
        SNDSoundStruct *sfh;
        int n;
        outhdrblk = mcalloc((long)sizeof(SNDSoundStruct)); /* alloc hdr blk */
        sfh = (SNDSoundStruct *)outhdrblk;
        sfh->magic = SND_MAGIC;
        sfh->samplingRate = esr;                        /*  assgn headrvals */
        sfh->channelCount = nchnls;
        sfh->dataLocation = sizeof(SNDSoundStruct);
        sfh->dataSize = -1;
        /* set formats .. */
        switch(O.outformat)
            {
#ifdef ULAW
        case AE_ULAW:   sfh->dataFormat = SND_FORMAT_MULAW_8;   break;
#endif
        case AE_CHAR:   sfh->dataFormat = SND_FORMAT_LINEAR_8;  break;
        case AE_SHORT:  sfh->dataFormat = SND_FORMAT_LINEAR_16; break;
        case AE_LONG:   sfh->dataFormat = SND_FORMAT_LINEAR_32; break;
        case AE_FLOAT:  sfh->dataFormat = SND_FORMAT_FLOAT;     break;
                /*RWD 5:2001 ??? either it builds, or it doesn't! */
        case AE_24INT:  sfh->dataFormat = SND_FORMAT_LINEAR_24; break;
        default:        sfh->dataFormat = SND_FORMAT_LINEAR_16; /* lose.. */
            }
        if ((n = write(ofd,(char*)sfh,sizeof(SNDSoundStruct)))<sizeof(SNDSoundStruct))
                die(Str(X_1200,"soundfile header write error.  aborting ..."));
#elif defined(SFSUN41)
        Audio_hdr *hp;
        unsigned encode;               /* chk that requested coding is legal */
        if (!(encode = 
#ifdef never
              (O.outformat==AE_ALAW)? AUDIO_ENCODING_ALAW :
#endif
#ifdef ULAW
              (O.outformat==AE_ULAW)? AUDIO_ENCODING_ULAW :
#endif
              (O.outformat==AE_CHAR)? AUDIO_ENCODING_LINEAR :
              (O.outformat==AE_SHORT)? AUDIO_ENCODING_LINEAR :
              /*RWD 5:2001 ???
                       (O.outformat==AE_24INT)? AUDIO_ENCODING_LINEAR :
              */
              (O.outformat==AE_LONG)? AUDIO_ENCODING_LINEAR :
              (O.outformat==AE_FLOAT)? AUDIO_ENCODING_FLOAT : 0))
                die(Str(X_847,"illegal encode for SFSUN41"));
        outhdrblk = mcalloc((long)sizeof(Audio_hdr));   /* allocate hdr blk */
        hp = (Audio_hdr *)outhdrblk;
        hp->sample_rate = (unsigned) esr;           /*  & fill in its values */
        hp->samples_per_unit = 1;
        hp->bytes_per_unit = (unsigned)O.outsampsiz;
        hp->channels = (unsigned) nchnls;
        hp->encoding = encode;
        hp->data_size = AUDIO_UNKNOWN_SIZE;
# ifdef sol
        if (strcmp(ofname,"devaudio")!=0 && strcmp(ofname,"/dev/audio")!=0) {
          if (audio_write_filehdr(ofd, hp, NULL, 0) != AUDIO_SUCCESS)
                die(Str(X_674,"could not write the outfile header"));
        }
        else {
/*        err_printf("configuring real time output device %d\n",ofd); */
          if (audio_set_play_config(ofd,hp) != AUDIO_SUCCESS)
                die(Str(X_671,"could not configure output device"));
        }
# else
        if (audio_write_filehdr(ofd, hp, NULL, 0) != AUDIO_SUCCESS)
                die(Str(X_674,"could not write the outfile header"));
# endif
#elif defined(mac_classic)
        AddMacHeader(ofname,nchnls,esr,O.outsampsiz);
#endif
    }
}

extern void aiffReWriteHdr(int, long, int);
extern void aifcReWriteHdr(int, long, int);
extern void wavReWriteHdr(int, long, int);
void rewriteheader(int ofd, long datasize, int verbose) /* write MaxAmps (IRCAM) or datasize (SUN) */
                                /* to existing hdr; called by sfcloseout() */
                                /*   & optionally by audwrite() under -R   */
{
    if (ofd<0) return;      /* For /dev/null */
    if (O.filetyp == TYP_AIFF)
      aiffReWriteHdr(ofd, datasize, verbose);
    else if (O.filetyp == TYP_AIFC)
      aifcReWriteHdr(ofd, datasize, verbose);
    else if (O.filetyp == TYP_WAV)
      wavReWriteHdr(ofd, datasize, verbose);
    else {
#ifdef SFIRCAM
      SFHEADER *sfh;
      SFMAXAMP *maxp;
      int n;

      sfh = (SFHEADER *)outhdrblk; /* update init hdr blk with maxamps */
      if ((maxp = (SFMAXAMP *) creatsfcode(SF_MAXAMP)) != NULL)
        for (n = 0; n < SF_MAXCHAN; n++)
          maxp->value[n] = (float)omaxamp[n];
      lseek(ofd,(off_t)0L,0);
      if (wheader(ofd,(char*)sfh))
        die(Str(X_730,"error rewriting sfheader"));
#endif
#ifdef NeXT
      /* attempt to rewrite datasize in initial header */
      /* datasize should be number of BYTES in SOUND only - not header */
      long dpos = (unsigned int)&(((SNDSoundStruct *)NULL)->dataSize);
      /* dpos is the offset of the dataSize field within soundstruct */
      
      if ((long)lseek(ofd, (off_t)dpos, SEEK_SET) == dpos)
        write(ofd, &datasize, sizeof(datasize));
#endif /* NeXT */
#ifdef SFSUN41
      {
        int n;
        if ((n = audio_rewrite_filesize(ofd, datasize)) != AUDIO_SUCCESS
            && n != AUDIO_ERR_NOEFFECT)
          die(Str(X_744,"error writing size into sfheader"));
      }
#endif
    }
}

#endif
