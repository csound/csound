/*
    ftgen.c:

    Copyright (C) 1991, 1994, 1995, 1998, 2000, 2004
                  Barry Vercoe, John ffitch, Paris Smaragdis,
                  Gabriel Maldonado, Richard Karpen, Greg Sullivan,
                  Pete Moss, Istvan Varga, Victor Lazzarini

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

#include "csdl.h"
#include <ctype.h>
#include <stdarg.h>
#include "soundio.h"
#include "cwindow.h"
#include <math.h>
#include "cmath.h"

typedef struct {
    OPDS    h;
    MYFLT   *ifno, *p1, *p2, *p3, *p4, *p5, *argums[VARGMAX];
} FTGEN;

typedef struct {
    OPDS    h;
    MYFLT   *ifno, *p1, *p2, *p3, *p4, *p5, *argums[VARGMAX];
    int     fno;
} FTGENTMP;

typedef struct {
    OPDS    h;
    MYFLT   *ifilno, *iflag, *argums[VARGMAX];
} FTLOAD;  /* gab 30 jul 2002 */

typedef struct {
    OPDS    h;
    MYFLT   *ifilno, *ktrig, *iflag, *argums[VARGMAX];
    FTLOAD  p;
} FTLOAD_K; /* gab 30 jul 2002 */

typedef struct {
    OPDS    h;
    MYFLT   *iftno, *ifreeTime;
    int     fno;
} FTFREE;

/* set up and call any GEN routine */

static int ftgen(ENVIRON *csound, FTGEN *p)
{
    MYFLT   *fp;
    FUNC    *ftp;
    EVTBLK  *ftevt;
    int     nargs;

    *p->ifno = FL(0.0);
    ftevt = (EVTBLK*) csound->Calloc(csound, sizeof(EVTBLK));
    ftevt->opcod = 'f';
    ftevt->strarg = NULL;
    fp = &ftevt->p[1];
    *fp++ = *p->p1;                                     /* copy p1 - p5 */
    *fp++ = ftevt->p2orig = FL(0.0);                    /* force time 0 */
    *fp++ = ftevt->p3orig = *p->p3;
    *fp++ = *p->p4;
    if (p->XSTRCODE) {                                  /* string argument: */
      int n = (int) ftevt->p[4];
      *fp++ = SSTRCOD;
      if (n < 0) n = -n;
      if (n == 1 || n == 23 || n == 28) {       /*   must be Gen01, 23 or 28 */
        ftevt->strarg = (char*) p->p5;
      }
      else {
        csound->Free(csound, ftevt);
        return csound->InitError(csound, Str("ftgen string arg not allowed"));
      }
    }
    else {
      *fp++ = *p->p5;
      ftevt->strarg = NULL;                             /* else no string */
    }
    if ((nargs = p->INOCOUNT - 5) > 0) {
      MYFLT **argp = p->argums;
      while (nargs--)                                   /* copy rem arglist */
        *fp++ = **argp++;
    }
    ftevt->pcnt = p->INOCOUNT;
    if (csound->hfgens(csound, &ftp, ftevt, 1) == 0) {  /* call the fgen */
      if (ftp != NULL)
        *p->ifno = (MYFLT) ftp->fno;                    /* record the fno */
    }
    else {
      csound->Free(csound, ftevt);
      return csound->InitError(csound, Str("ftgen error"));
    }
    csound->Free(csound, ftevt);
    return OK;
}

static int ftfree(ENVIRON *csound, FTFREE *p)
{
    FUNC    *ftp = NULL;
    EVTBLK  evt;

    if (p->fno == 0 && *p->ifreeTime != FL(0.0)) {
      p->fno = (int) MYFLT2LRND(*p->iftno);
      return (csound->RegisterDeinitCallback(csound, p,
                                             (int (*)(void*, void*)) ftfree));
    }
    if (!p->fno)
      p->fno = (int) MYFLT2LRND(*p->iftno);
    if (p->fno < 0)
      p->fno = -(p->fno);
    evt.strarg = NULL;
    evt.opcod = 'f';
    evt.pcnt = 1;
    evt.p[1] = (MYFLT) p->fno;
    p->fno = 0;
    if (csound->hfgens(csound, &ftp, &evt, 0) != 0)
      return csound->InitError(csound, Str("ftfree: invalid table number"));

    return OK;
}

static int ftload(ENVIRON *csound, FTLOAD *p)
{
#if 0
    /* FIXME: this opcode is broken and needs to be fixed */
    MYFLT **argp = p->argums;
    FUNC  *ftp;
    char  filename[MAXNAME];
    int   nargs = p->INOCOUNT - 2;
    FILE  *file = NULL;
    int   (*err_func)(void *, const char *, ...);
    FUNC  *(*ft_func)(void *, MYFLT *);

    if (strcmp(p->h.optext->t.opcod, "ftload") != 0) {
      nargs--;
      ft_func = (FUNC *(*)(void *, MYFLT *)) csound->FTFindP;
      err_func = (int (*)(void *, const char *, ...)) csound->PerfError;
    }
    else {
      ft_func = (FUNC *(*)(void *, MYFLT *)) csound->FTFind;
      err_func = (int (*)(void *, const char *, ...)) csound->InitError;
    }

    if (nargs <= 0)
      goto err2;

    if (p->XSTRCODE)                        /* if char string name given */
      strcpy(filename, (char*) p->ifilno);  /* FIXME: and what if not ?  */
    if (*p->iflag <= FL(0.0)) {
      if (!(file = fopen(filename, "rb"))) goto err3;
      while (nargs--)  {
        FUNC header;
        FGDATA *ff = &(p->h.insdshead->csound->ff);

        ff->fno = (int) **argp;
        fread(&header, sizeof(FUNC)-sizeof(MYFLT)-SSTRSIZ, 1, file);
        /* ***** Need to do byte order here ***** */
        ff->flen = header.flen;
        header.fno = ff->fno;
        if ((ftp = ft_func(csound, *argp)) != NULL) {
          MYFLT *table = ftp->ftable;
          memcpy(ftp, &header, sizeof(FUNC)-sizeof(MYFLT)-SSTRSIZ);
          ftp = ftalloc(csound);
          fread(table, sizeof(float), ff->flen, file);
          /* ***** Need to do byte order here ***** */
        }
        else goto err;
        argp++;
      }
    }
    else {
      if (!(file = fopen(filename, "r"))) goto err3;
      while (nargs--) {
        FUNC header;
        char s[64], *s1;
        FGDATA *ff = &(csound->ff);

        ff->fno = (int) **argp;
        /* IMPORTANT!! If FUNC structure and/or GEN01ARGS structure
           will be modified, the following code has to be modified too */
        fgets(s, 64, file);
        fgets(s, 64, file);  s1 = strchr(s, ' ')+1;
        header.flen = atol(s1);
        fgets(s, 64, file);  s1 = strchr(s, ' ')+1;
        header.lenmask = atol(s1);
        fgets(s, 64, file);  s1 = strchr(s, ' ')+1;
        header.lobits = atol(s1);
        fgets(s, 64, file);  s1 = strchr(s, ' ')+1;
        header.lomask = atol(s1);
        fgets(s, 64, file);  s1 = strchr(s, ' ')+1;
        header.lodiv = (MYFLT)atof(s1);
        fgets(s, 64, file);  s1 = strchr(s, ' ')+1;
        header.cvtbas = (MYFLT)atof(s1);
        fgets(s, 64, file);  s1 = strchr(s, ' ')+1;
        header.cpscvt = (MYFLT)atof(s1);
        fgets(s, 64, file);  s1 = strchr(s, ' ')+1;
        header.loopmode1 = (short) atol(s1);
        fgets(s, 64, file);  s1 = strchr(s, ' ')+1;
        header.loopmode2 = (short) atol(s1);
        fgets(s, 64, file);  s1 = strchr(s, ' ')+1;
        header.begin1 = atol(s1);
        fgets(s, 64, file);  s1 = strchr(s, ' ')+1;
        header.end1 = atol(s1);
        fgets(s, 64, file);  s1 = strchr(s, ' ')+1;
        header.begin2 = atol(s1);
        fgets(s, 64, file);  s1 = strchr(s, ' ')+1;
        header.end2 = atol(s1);
        fgets(s, 64, file);  s1 = strchr(s, ' ')+1;
        header.soundend = atol(s1);
        fgets(s, 64, file);  s1 = strchr(s, ' ')+1;
        header.flenfrms = atol(s1);
        fgets(s, 64, file);  s1 = strchr(s, ' ')+1;
        header.nchanls = atol(s1);
        fgets(s, 64, file);  s1 = strchr(s, ' ')+1;
        header.fno = atol(s1);
        fgets(s, 64, file);  s1 = strchr(s, ' ')+1;
        header.gen01args.gen01 = (MYFLT)atof(s1);
        fgets(s, 64, file);  s1 = strchr(s, ' ')+1;
        header.gen01args.ifilno = (MYFLT)atof(s1);
        fgets(s, 64, file);  s1 = strchr(s, ' ')+1;
        header.gen01args.iskptim = (MYFLT)atof(s1);
        fgets(s, 64, file);  s1 = strchr(s, ' ')+1;
        header.gen01args.iformat = (MYFLT)atof(s1);
        fgets(s, 64, file);  s1 = strchr(s, ' ')+1;
        header.gen01args.channel = (MYFLT)atof(s1);
        fgets(s, 64, file);  s1 = strchr(s, ' ')+1;
        header.gen01args.sample_rate = (MYFLT)atof(s1);
        fgets(s, 64, file);
        /* WARNING! skips header.gen01args.strarg from saving/loading
           in text format */
        ff->flen = header.flen;
        header.fno = ff->fno;
        if ((ftp = ft_func(csound, *argp)) != NULL) {
          long j;
          MYFLT *table = ftp->ftable;
          memcpy(ftp, &header, sizeof(FUNC)-sizeof(MYFLT));
/*        ftp = ftalloc(csound);        FIXME: should fix reallocation */
          for (j=0; j < ff->flen; j++) {
            fgets(s, 64, file);
            table[j] = (MYFLT)atof(s);
          }
          fgets(s, 64, file);
        }
        else goto err;
        argp++;
      }
    }
    fclose(file);
    return OK;
 err:
    fclose(file);
    return err_func(csound, Str("ftload: Bad table number. Loading is possible "
                                "only into existing tables."));
 err2:
    return err_func(csound, Str("ftload: no table numbers"));
 err3:
    return err_func(csound, Str("ftload: unable to open file"));
#endif
    return NOTOK;
}

static int ftload_k(ENVIRON *csound, FTLOAD_K *p)
{
    if (*p->ktrig != FL(0.0))
      return ftload(csound, &(p->p));
    return OK;
}

static int ftsave(ENVIRON *csound, FTLOAD *p)
{
    MYFLT **argp = p->argums;
    char  filename[MAXNAME];
    int   nargs = p->INOCOUNT - 2;
    FILE  *file = NULL;
    int   (*err_func)(void *, const char *, ...);
    FUNC  *(*ft_func)(void *, MYFLT *);

    if (strcmp(p->h.optext->t.opcod, "ftsave") != 0) {
      nargs--;
      ft_func = (FUNC *(*)(void *, MYFLT *)) csound->FTFindP;
      err_func = (int (*)(void *, const char *, ...)) csound->PerfError;
    }
    else {
      ft_func = (FUNC *(*)(void *, MYFLT *)) csound->FTFind;
      err_func = (int (*)(void *, const char *, ...)) csound->InitError;
    }

    if (nargs <= 0)
      goto err2;

    if (p->XSTRCODE)                        /* if char string name given */
      strcpy(filename, (char*) p->ifilno);  /* FIXME: and what if not ?  */
    if (*p->iflag <= FL(0.0)) {
      if (!(file = fopen(filename, "wb"))) goto err3;
      while (nargs--) {
        FUNC *ftp;

        if ((ftp = ft_func(csound, *argp)) != NULL) {
          MYFLT *table = ftp->ftable;
          long flen = ftp->flen;
          fwrite(ftp, sizeof(FUNC)-sizeof(MYFLT)-SSTRSIZ, 1, file);
          fwrite(table, sizeof(MYFLT), flen, file);
        }
        else goto err;
        argp++;
      }
    }
    else {
      if (!(file = fopen(filename, "w"))) goto err3;
      while (nargs--)  {
        FUNC *ftp;

        if ((ftp = ft_func(csound, *argp)) != NULL) {
          long flen = ftp->flen;
          long j;
          MYFLT *table = ftp->ftable;
          /* IMPORTANT!! If FUNC structure and/or GEN01ARGS structure
             will be modified, the following code has to be modified too */
          fprintf(file,"======= TABLE %ld size: %ld values ======\n",
                  ftp->fno, ftp->flen);
          fprintf(file,"flen: %ld\n", ftp->flen);
          fprintf(file,"lenmask: %ld\n", ftp->lenmask);
          fprintf(file,"lobits: %ld\n",ftp->lobits);
          fprintf(file,"lomask: %ld\n",ftp->lomask);
          fprintf(file,"lodiv: %f\n",ftp->lodiv);
          fprintf(file,"cvtbas: %f\n",ftp->cvtbas);
          fprintf(file,"cpscvt: %f\n",ftp->cpscvt);
          fprintf(file,"loopmode1: %d\n", (int) ftp->loopmode1);
          fprintf(file,"loopmode2: %d\n", (int) ftp->loopmode2);
          fprintf(file,"begin1: %ld\n",ftp->begin1);
          fprintf(file,"end1: %ld\n",ftp->end1);
          fprintf(file,"begin2: %ld\n",ftp->begin2);
          fprintf(file,"end2: %ld\n",ftp->end2);
          fprintf(file,"soundend: %ld\n",ftp->soundend);
          fprintf(file,"flenfrms: %ld\n",ftp->flenfrms);
          fprintf(file,"nchnls: %ld\n",ftp->nchanls);
          fprintf(file,"fno: %ld\n",ftp->fno);

          fprintf(file,"gen01args.gen01: %f\n",ftp->gen01args.gen01);
          fprintf(file,"gen01args.ifilno: %f\n",ftp->gen01args.ifilno);
          fprintf(file,"gen01args.iskptim: %f\n",ftp->gen01args.iskptim);
          fprintf(file,"gen01args.iformat: %f\n",ftp->gen01args.iformat);
          fprintf(file,"gen01args.channel: %f\n",ftp->gen01args.channel);
          fprintf(file,"gen01args.sample_rate: %f\n",
                       ftp->gen01args.sample_rate);
          /* WARNING! skips ftp->gen01args.strarg from saving/loading in
             text format */
          fprintf(file,"---------END OF HEADER--------------\n");

          for (j=0; j < flen; j++) {
            MYFLT val = table[j];
            fprintf(file,"%f\n",val);
          }
          fprintf(file,"---------END OF TABLE---------------\n");
        }
        else goto err;
        argp++;
      }
    }
    fclose(file);
    return OK;
 err:
    fclose(file);
    return err_func(csound, Str("ftsave: Bad table number. Saving is possible "
                                "only for existing tables."));
 err2:
    return err_func(csound, Str("ftsave: no table numbers"));
 err3:
    return err_func(csound, Str("ftsave: unable to open file"));
}

static int ftsave_k_set(ENVIRON *csound, FTLOAD_K *p)
{
    memcpy(&(p->p.h), &(p->h), sizeof(OPDS));
    p->p.ifilno = p->ifilno;
    p->p.iflag = p->iflag;
    memcpy(p->p.argums, p->argums, sizeof(MYFLT*) * (p->INOCOUNT - 3));
    return OK;
}

static int ftsave_k(ENVIRON *csound, FTLOAD_K *p)
{
    if (*p->ktrig != FL(0.0))
      return ftsave(csound, &(p->p));
    return OK;
}

#define S(x)    sizeof(x)

static OENTRY localops[] = {
  { "ftgen",    S(FTGEN),     1,  "i",  "iiiiTm", (SUBR) ftgen, NULL, NULL    },
/*
  { "ftgentmp", S(FTGENTMP),  1,  "i",  "iiiiTm", (SUBR) ftgentmp, NULL, NULL },
*/
  { "ftfree",   S(FTFREE),    1,  "",   "ii",     (SUBR) ftfree, NULL, NULL   },
  { "ftsave",   S(FTLOAD),    1,  "",   "Tim",    (SUBR) ftsave, NULL, NULL   },
  { "ftload",   S(FTLOAD),    1,  "",   "Tim",    (SUBR) ftload, NULL, NULL   },
  { "ftsavek",  S(FTLOAD_K),  3,  "",   "Tkim",   (SUBR) ftsave_k_set,
                                                  (SUBR) ftsave_k, NULL       },
  { "ftloadk",  S(FTLOAD_K),  3,  "",   "Tkim",   (SUBR) ftsave_k_set,
                                                  (SUBR) ftload_k, NULL       }
};

LINKAGE

