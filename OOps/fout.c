/*
    fout.c:

    Copyright (C) 1999 Gabriel Maldonado, John ffitch

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

/* Opcodes By Gabriel Maldonado, 1999 */
/* Code modified by JPff to remove fixed size arrays, allow
   AIFF and WAV, and close files neatly.  Also bugs fixed */

#include "fout.h"
#include <ctype.h>

#ifdef  USE_DOUBLE
#define sf_write_MYFLT	sf_write_double
#define sf_read_MYFLT	sf_read_double
#else
#define sf_write_MYFLT	sf_write_float
#define sf_read_MYFLT	sf_read_float
#endif

struct fileinTag {
    SNDFILE* file;
    char  *name;
    long  cnt;
    int   hdr;
};

static struct fileinTag *file_opened = NULL;
static int file_max = 0;        /* Size of file_opened structure */
static int file_num = -1;              /* Last number used */

static void close_files(void)
{
    while (file_num>=0) {
      printf("%d (%s):", file_num, file_opened[file_num].name);
/*       fflush(file_opened[file_num].file); */
      if (file_opened[file_num].hdr) {
        rewriteheader(file_opened[file_num].file,
                      /*file_opened[file_num].cnt,*/ 1);
      }
      sf_close(file_opened[file_num].file);
      file_num--;
      printf("\n");
    }
}

static int outfile_float(OUTFILE *p)
{
    int nsmps = ksmps, j, nargs = p->nargs, k=0;
    MYFLT **args = p->argums;
    do {
      for (j = 0;j< nargs;j++)
        sf_write_MYFLT(p->fp, &(args[j][k]), 1);
      k++;
    } while (--nsmps);
    return OK;
}


static int outfile_int(OUTFILE *p)
{
    int nsmps = ksmps, j, nargs = p->nargs, k=0;
    MYFLT **args = p->argums;
    short tmp;
    do {
      for (j = 0;j< nargs;j++) {
        tmp = (short)    args[j][k];
        sf_write_MYFLT(p->fp,&tmp, 1);
      }
      k++;
    } while (--nsmps);
    return OK;
}


static int outfile_int_head(OUTFILE *p)
{
    int nsmps= ksmps, j, nargs = p->nargs, k=0;
    MYFLT **args = p->argums;
    do {
      for (j = 0;j< nargs;j++) {
        short tmp = (short) args[j][k];
        sf_write_MYFLT(p->fp, &tmp, 1);
      }
      k++;
    } while (--nsmps);
    p->cnt++;                   /* Count cycle */
    file_opened[p->idx].cnt += ksmps * sizeof(short)*nargs;
    if ((kcounter& 0x3f)==0) {         /* Every 64 cycles */
      fflush(p->fp);
      rewriteheader(p->fp, /*p->cnt * ksmps * sizeof(short)*nargs,*/ 0);
    }
    return OK;
}


int outfile_set(OUTFILE *p)
{
    int n=0;
    if (*p->fname == SSTRCOD) { /* if char string name given */
      int j;
      char fname[FILENAME_MAX];
      /*extern char *unquote(char *name); */
      if (p->STRARG == NULL) strcpy(fname,unquote(currevent->strarg));
      else strcpy(fname, unquote(p->STRARG));
      for (j=0; j<file_num; j++) {
        if (!strcmp(file_opened[j].name,fname)) {
          p->fp = file_opened[j].file;
          p->idx = n = j;
          goto done;
        }
      }
      if ((p->fp = fopen(fname,"wb")) == NULL)
        dies(Str(X_1465,"fout: cannot open outfile %s"),fname);
      else { /* put the file in the opened stack */
        file_num++;
        if (file_num>=file_max) {
          if (file_max==0) atexit(close_files);
          file_max += 4;
          file_opened = (struct fileinTag*)
            mrealloc(file_opened, sizeof(struct fileinTag)*file_max);
        }
        file_opened[file_num].name = (char*) mmalloc(strlen(fname)+1);
        strcpy(file_opened[file_num].name, fname);
        file_opened[file_num].file = p->fp;
        p->idx = n = file_num;
        file_opened[file_num].cnt = 0;
        file_opened[file_num].hdr = 0;
      }
    }
    else { /* file handle as argument */
      n = (int)*p->fname;
      if (n>file_num || (p->fp = file_opened[n].file) == NULL)
        die(Str(X_1466,"fout: invalid file handle"));
    }
 done:
    p->nargs = p->INOCOUNT-2;
    switch((int) (*p->iflag+FL(0.5))) {
    case 0:
      p->outfilep = (SUBR)outfile_float;
      break;
    case 1:
      p->outfilep = (SUBR)outfile_int;
      break;
    case 2:
      p->outfilep = (SUBR)outfile_int_head;
      p->cnt = 0;
      file_opened[n].hdr = 1;
      writeheader(              /* Write header at start of file.  */
                   fileno(p->fp), /* Called after open, before data writes*/
                   file_opened[n].name);
      break;
    default:
      p->outfilep = (SUBR)outfile_int;
    }
    return OK;
}

int outfile (OUTFILE *p)
{
    p->outfilep(p);
    return OK;
}

static int koutfile_float (KOUTFILE *p)
{
    int j, nargs = p->nargs;
    MYFLT **args = p->argums;
    for (j = 0;j< nargs;j++) {
      sf_write_MYFLT(p->fp, args[j],1);
    }
    return OK;
}


static int koutfile_int (KOUTFILE *p)
{
    int j,nargs = p->nargs;
    MYFLT **args = p->argums;
    short tmp;
    for (j = 0;j< nargs;j++) {
      tmp = (short) *(args[j]);
      fwrite(&tmp, sizeof(short), 1, p->fp);
    }
    return OK;
}

int koutfile_set(KOUTFILE *p)
{
    int n;
    if (*p->fname == SSTRCOD) {/*gab B1*/ /* if char string name given */
      int j;
      char fname[FILENAME_MAX];
      /*extern char *unquote(char *name); */
      if (p->STRARG == NULL) strcpy(fname,unquote(currevent->strarg)); /*gab B1*/
      else strcpy(fname, unquote(p->STRARG));
      for (j=0; j<file_num; j++) {
        if (!strcmp(file_opened[j].name,fname)) {
          p->fp = file_opened[j].file;
          p->idx = n = j;
          goto done;
        }
      }
                                /* *** NON ANSI CODE *** */
      if ((p->fp = fopen(fname,"wb")) == NULL)
        dies(Str(X_1467,"foutk: cannot open outfile %s"),fname);
      else { /* put the file in the opened stack */
        file_num++;
        if (file_num>=file_max) {
          if (file_max==0) atexit(close_files);
          file_max += 4;
          file_opened = (struct fileinTag*)
            mrealloc(file_opened, sizeof(struct fileinTag)*file_max);
        }
        file_opened[file_num].name = (char*)mmalloc(strlen(fname)+1);
        strcpy(file_opened[file_num].name, fname);
        file_opened[file_num].file=p->fp;
        p->idx = n = file_num;
        file_opened[file_num].cnt = file_opened[file_num].hdr = 0;
      }
    }
    else { /* file handle argument */
      n = (int)*p->fname;
      if (n>file_num || (p->fp = file_opened[n].file) == NULL)
        die(Str(X_1466,"fout: invalid file handle"));
    }
 done:
    switch((int) (*p->iflag+FL(0.5))) {
    case 0:
      p->koutfilep = (SUBR)koutfile_float;
      break;
    case 1:
      p->koutfilep = (SUBR)koutfile_int;
      break;
    default:
      p->koutfilep = (SUBR)koutfile_int;
    }
    p->nargs = p->INOCOUNT-2;
    p->cnt = 0;
    return OK;
}

int koutfile(KOUTFILE *p)
{
    p->koutfilep(p);
    return OK;
}


/*--------------*/


/* syntax:
        ihandle fiopen "filename" [, iascii]
*/
int fiopen(FIOPEN *p)          /* open a file and return its handle  */
{                              /* the handle is simply a stack index */
    char fname[FILENAME_MAX];
    char *omodes[] = {"w", "r", "wb", "rb"};
    FILE *fp;
    int idx = (int)*p->iascii;
    strcpy(fname, unquote(p->STRARG));
    if (idx<0 || idx>3) idx=0;
    if (( fp = fopen(fname,omodes[idx])) == NULL)
      dies(Str(X_1468,"fout: cannot open outfile %s"),fname);
    if (idx>1) setbuf(fp, NULL);
    file_num++;
    if (file_num>=file_max) {
      if (file_max==0) atexit(close_files);
      file_max += 4;
      file_opened = (struct fileinTag*)
        mrealloc(file_opened, sizeof(struct fileinTag)*file_max);
    }
    file_opened[file_num].name = (char*)mmalloc(strlen(fname)+1);
    strcpy(file_opened[file_num].name, fname);
    file_opened[file_num].file=fp;
    *p->ihandle = (MYFLT) file_num;
    return OK;
}

/* syntax:
   fouti  ihandle, iascii, iflag, iarg1 [,iarg2,....,iargN]
*/

long kreset=0;
int ioutfile_set(IOUTFILE *p)
{
    int j;
    MYFLT **args=p->argums;
    FILE *fil;
    int n = (int) *p->ihandle;
    if (n<0 || n>file_num)
      die(Str(X_1469,"fouti: invalid file handle"));
    fil = file_opened[n].file;
    if (fil == NULL) die(Str(X_1469,"fouti: invalid file handle"));
    if (*p->iascii == 0) { /* ascii format */
      switch ((int) *p->iflag) {
      case 1: { /* whith prefix (i-statement, p1, p2 and p3) */
        int p1 = (int) p->h.insdshead->insno;
        double p2 =   (double) kcounter * onedkr;
        double p3 = p->h.insdshead->p3;
        if (p3 > FL(0.0))
          fprintf(fil, "i %i %f %f ", p1, p2, p3);
        else
          fprintf(fil, "i %i %f . ", p1, p2);
      }
      break;
      case 2: /* whith prefix (start at 0 time) */
        if (kreset == 0) kreset = kcounter;
        {
          int p1 = (int) p->h.insdshead->insno;
          double p2= (double) (kcounter - kreset) * onedkr;
          double p3 = p->h.insdshead->p3;
          if (p3 > FL(0.0))
            fprintf(fil, "i %i %f %f ", p1, p2, p3);
          else
            fprintf(fil, "i %i %f . ", p1, p2);
        }
        break;
      case 3: /* reset */
        kreset=0;
        return OK;
      }
      for (j=0; j < p->INOCOUNT - 3;j++) {
        fprintf( fil, " %f",(double) *args[j]);
      }
      putc('\n',fil);
    }
    else { /* binary format */
      for (j=0; j < p->INOCOUNT - 3;j++) {
        fwrite(args[j], sizeof(MYFLT),1, fil );
      }
    }
    return OK;
}


int ioutfile_set_r(IOUTFILE_R *p)
{
    int *xtra;
    if (*(xtra = &(p->h.insdshead->xtratim)) < 1 )  /* gab-a5 revised */
      *xtra = 1;
    p->counter =  kcounter;
    p->done = 1;
    if (*p->iflag==2)
      if (kreset == 0) kreset = kcounter;
    return OK;
}


int ioutfile_r(IOUTFILE_R *p)
{
    if (p->h.insdshead->relesing) {
      if (p->done) {
        int j;
        MYFLT **args=p->argums;
        FILE *fil;
        int n = (int) *p->ihandle;
        if (n<0 || n>file_num) die(Str(X_1469,"fouti: invalid file handle"));
        fil = file_opened[n].file;
        if (fil == NULL) die(Str(X_1469,"fouti: invalid file handle"));
        if (*p->iascii == 0) { /* ascii format */
          switch ((int) *p->iflag) {
          case 1:       {       /* whith prefix (i-statement, p1, p2 and p3) */
            int p1 = (int) p->h.insdshead->insno;
            double p2 = p->counter * onedkr;
            double p3 = (double) (kcounter-p->counter) * onedkr;
            fprintf(fil, "i %i %f %f ", p1, p2, p3);
          }
          break;
          case 2: /* whith prefix (start at 0 time) */
            {
              int p1 = (int) p->h.insdshead->insno;
              double p2 = (p->counter - kreset) *onedkr;
              double p3 = (double) (kcounter-p->counter) * onedkr;
              fprintf(fil, "i %i %f %f ", p1, p2, p3);
            }
            break;
          case 3: /* reset */
            kreset=0;
            return OK;
          }
          for (j=0; j < p->INOCOUNT - 3;j++) {
            fprintf( fil, " %f",(double) *args[j]);
          }
          putc('\n',fil);
        }
        else { /* binary format */
          for (j=0; j < p->INOCOUNT - 3;j++) {
            fwrite(args[j], sizeof(MYFLT),1, fil );
          }
        }
        p->done = 0;
      }
    }
    return OK;
}

/*----------------------------------*/

static int infile_float(INFILE *p)
{
    int nsmps= ksmps, j, nargs = p->nargs,k=0;
    MYFLT **args = p->argums;
    if (p->flag) {
      fseek(p->fp, p->currpos*sizeof(MYFLT)*nargs ,SEEK_SET);
      p->currpos+=nsmps;
      do {
        for (j = 0;j< nargs;j++) {
          if (fread(&(args[j][k]), sizeof(MYFLT), 1, p->fp));
          else {
            p->flag = 0;
            args[j][k] = FL(0.0);
          }
        }
        k++;
      } while (--nsmps);
    }
    else { /* after end of file */
      do {
        for (j = 0;j< nargs;j++)
          args[j][k] = FL(0.0);
        k++;
      } while (--nsmps);
    }
    return OK;
}


int infile_int(INFILE *p)
{
    int nsmps= ksmps, j,nargs = p->nargs,k=0;
    MYFLT **args = p->argums;
    short tmp;
    if (p->flag) {
      fseek(p->fp, p->currpos*sizeof(short)*nargs ,SEEK_SET);
      p->currpos+=nsmps;
      do {
        for (j = 0;j< nargs;j++) {
          if (fread( &tmp, sizeof(short),1,p->fp))
            args[j][k] = (MYFLT) tmp;
          else {
            p->flag = 0;
            args[j][k] = FL(0.0);
          }
        }
        k++;
      } while (--nsmps);
    }
    else {  /* after end of file */
      do {
        for (j = 0;j< nargs;j++)
          args[j][k] = FL(0.0);
        k++;
      } while (--nsmps);
    }
    return OK;
}

int infile_set(INFILE *p)
{
    if (*p->fname == SSTRCOD) { /* if char string name given */
      int j;
      /*extern char *unquote(char *name); */
      char fname[FILENAME_MAX];
      if (p->STRARG == NULL) strcpy(fname,unquote(currevent->strarg));
      else strcpy(fname, unquote(p->STRARG));
      for (j=0; j<file_num; j++) {
        if (!strcmp(file_opened[j].name,fname)) {
          p->fp = file_opened[j].file;
          goto done;
        }
      }
      if (( p->fp = fopen(fname,"rb")) == NULL)
        dies(Str(X_1470,"fin: cannot open infile %s"),fname);
      else { /* put the file in the opened stack */
        file_num++;
        if (file_num>=file_max) {
          if (file_max==0) atexit(close_files);
          file_max += 4;
          file_opened = (struct fileinTag*)
            mrealloc(file_opened, sizeof(struct fileinTag)*file_max);
        }
        file_opened[file_num].name = (char*)mmalloc(strlen(fname)+1);
        strcpy(file_opened[file_num].name, fname);
        file_opened[file_num].file=p->fp;
      }
    }
    else { /* file handle argument */
      int n = (int) *p->fname;
      if (n<0 || n> file_num || (p->fp = file_opened[n].file) == NULL)
        die(Str(X_1471,"fin: invalid file handle"));
    }
 done:
    switch((int) (*p->iflag+FL(0.5))) {
    case 0:
      p->infilep = (SUBR)infile_float;
      break;
    case 1:
      p->infilep = (SUBR)infile_int;
      break;
    default:
      p->infilep = (SUBR)infile_int;
    }
    p->nargs = p->INOCOUNT-3;
    p->currpos = (long) *p->iskpfrms;
    p->flag=1;
    return OK;
}

int infile_act(INFILE *p)
{
    p->infilep(p);
    return OK;
}


/*----------------------------*/

static int kinfile_float(KINFILE *p)
{
    int j, nargs = p->nargs;
    MYFLT **args = p->argums;
    if (p->flag) {
      fseek(p->fp, p->currpos*sizeof(MYFLT)*nargs ,SEEK_SET);
      p->currpos++;
      for (j = 0;j< nargs;j++) {
        if (fread(args[j], sizeof(MYFLT),1,p->fp));
        else {
          p->flag = 0;
          *(args[j]) = FL(0.0);
        }
      }
    }
    else { /* after end of file */
      for (j = 0; j < nargs; j++)
        *(args[j]) = FL(0.0);
    }
    return OK;
}


int kinfile_int(KINFILE *p)
{
    int j,nargs = p->nargs;
    MYFLT **args = p->argums;
    short tmp;
    if (p->flag) {
      fseek(p->fp, p->currpos*sizeof(short)*nargs ,SEEK_SET);
      p->currpos++;
      for (j = 0;j< nargs;j++) {
        if (fread( &tmp, sizeof(short),1,p->fp))
          *(args[j]) = (MYFLT) tmp;
        else {
          p->flag = 0;
          *(args[j]) = FL(0.0);
        }
      }
    }
    else {  /* after end of file */
      for (j = 0;j< nargs;j++)
        *(args[j]) = FL(0.0);
    }
    return OK;
}


int kinfile_set(KINFILE *p)
{
    if (*p->fname == SSTRCOD) { /* if char string name given */
      int j;
      /*extern char *unquote(char *name); */
      char fname[FILENAME_MAX];
      if (p->STRARG == NULL) strcpy(fname,unquote(currevent->strarg)); /*gab B1*/
      else strcpy(fname, unquote(p->STRARG));
      for (j=0; j<file_num || file_opened[j].name == NULL; j++) {
        if (!strcmp(file_opened[j].name,fname)) {
          p->fp = file_opened[j].file;
          goto done;
        }
      }
      if (( p->fp = fopen(fname,"rb")) == NULL)
        dies(Str(X_1470,"fin: cannot open infile %s"),fname);
      else { /* put the file in the opened stack */
        file_num++;
        if (file_num>=file_max) {
          if (file_max==0) atexit(close_files);
          file_max += 4;
          file_opened = (struct fileinTag*)
            mrealloc(file_opened, sizeof(struct fileinTag)*file_max);
        }
        file_opened[file_num].name = (char*)mmalloc(strlen(fname)+1);
        strcpy(file_opened[file_num].name, fname);
        file_opened[file_num].file=p->fp;
      }
    }
    else {/* file handle argument */
      int n = (int) *p->fname;
      if (n<0 || n>file_num || (p->fp = file_opened[n].file) == NULL)
        die(Str(X_1472,"fink: invalid file handle"));
    }
 done:
    switch((int) (*p->iflag+FL(0.5))) {
    case 0:
      p->kinfilep = (SUBR)kinfile_float;
      break;
    case 1:
      p->kinfilep = (SUBR)kinfile_int;
      break;
    default:
      p->kinfilep = (SUBR)kinfile_int;
    }
    p->nargs = p->INOCOUNT-3;
    p->currpos = (long) *p->iskpfrms;
    p->flag=1;
    return OK;
}


int kinfile(KINFILE *p)
{
    p->kinfilep(p);
    return OK;
}



int i_infile(I_INFILE *p)
{
    int j, nargs;
    FILE *fp = NULL;
    MYFLT **args = p->argums;
    if (*p->fname == SSTRCOD) {/* if char string name given */
      char fname[FILENAME_MAX];
      char *omodes[] = {"r", "r", "rb"};
      int idx;
      /*extern char *unquote(char *name); */

      if (p->STRARG == NULL) strcpy(fname,unquote(currevent->strarg));
      else strcpy(fname, unquote(p->STRARG));
      for (j=0; j<file_num || file_opened[j].name == NULL; j++) {
        if (!strcmp(file_opened[j].name,fname)) {
          fp = file_opened[j].file;
          goto done;
        }
      }
      idx = (int) (*p->iflag+FL(0.5));
      if (idx<0 || idx>2) idx = 0;
      if (( fp = fopen(fname,omodes[idx])) == NULL)
        dies(Str(X_1470,"fin: cannot open infile %s"),fname);
      else { /* put the file in the opened stack */
        file_num++;
        if (file_num>=file_max) {
          if (file_max==0) atexit(close_files);
          file_max += 4;
          file_opened = (struct fileinTag*)
            mrealloc(file_opened, sizeof(struct fileinTag)*file_max);
        }
        file_opened[file_num].name = (char*)mmalloc(strlen(fname)+1);
        strcpy(file_opened[file_num].name, fname);
        file_opened[file_num].file=fp;
      }
    }
    else {/* file handle argument */
      int n = (int) *p->fname;
      if (n<0 || n>file_num || (fp = file_opened[n].file) == NULL)
        die(Str(X_1472,"fink: invalid file handle"));
    }
 done:
    nargs = p->INOCOUNT-3;
    switch((int) (*p->iflag+FL(0.5))) {
    case 0: /* ascii file with loop */
      {
        char cf[20], *cfp;
        int cc;
      newcycle:
        for (j = 0;j< nargs;j++) {
          cfp = cf;
          while ((*cfp=cc=getc(fp)) == 'i'
                 || isspace(*cfp));
          if (cc == EOF) {
            fseek(fp, 0 ,SEEK_SET);
            goto newcycle;
          }
          while (isdigit(*cfp) || *cfp == '.')  {
            *(++cfp) = cc = getc(fp);
          }
          *++cfp = '\0';        /* Must terminate string */
          *(args[j]) = (MYFLT) atof (cf);
          if (cc == EOF) {
            fseek(fp, 0 ,SEEK_SET);
            break;
          }
        }
      }
      break;
    case 1: /* ascii file without loop */
      {
        char cf[20], *cfp;
        int cc;
        for (j = 0;j< nargs;j++) {
          cfp = cf;
          while ((*cfp=cc=getc(fp)) == 'i'
                 || isspace(*cfp));
          if (cc == EOF) {
            *(args[j]) = FL(0.0);
            break;
          }
          while (isdigit(*cfp) || *cfp == '.') {
            *(++cfp) = cc = getc(fp);
          }
          *++cfp = '\0';        /* Must terminate */
          *(args[j]) = (MYFLT) atof (cf);
          if (cc == EOF) {
            *(args[j]) = FL(0.0);
            break;
          }
        }
      }
      break;
    case 2: /* binary floats without loop */
      fseek(fp, p->currpos*sizeof(float)*nargs ,SEEK_SET);
      p->currpos++;
      for (j = 0;j< nargs;j++) {
        if (fread(args[j], sizeof(float),1,fp));
        else {
          p->flag = 0;
          *(args[j]) = FL(0.0);
        }
      }
      break;
    }
    return OK;
}

/*---------------------------*/

int incr(INCR *p)
{
    MYFLT *avar = p->avar, *aincr = p->aincr;
    int nsmps= ksmps;
    do  *(avar++) += *(aincr++);
    while (--nsmps);
    return OK;
}


int clear(CLEARS *p)
{
    int nsmps= ksmps,j;
    MYFLT *avar;
    for (j=0;j< p->INOCOUNT;j++) {
      avar = p->argums[j];
      nsmps= ksmps;
      do        *(avar++) = FL(0.0);
      while (--nsmps);
    }
    return OK;
}



/*---------------------------*/
/* formatted output to a text file */

int fprintf_set(FPRINTF *p)
{
    int n;
    char *sarg = p->STRARG2;
    char *sdest = p->txtstring;

    memset(p->txtstring, 0, 8192); /* *** Nasty to have exposed constant in code */

    if (*p->fname == SSTRCOD) { /* if char string name given */
      int j;
      char fname[FILENAME_MAX];
      /*extern char *unquote(char *name); */
      if (p->STRARG == NULL) strcpy(fname,unquote(currevent->strarg));
      else strcpy(fname, unquote(p->STRARG));
      for (j=0; j<= file_num; j++) {
        if (!strcmp(file_opened[j].name,fname)) {
          p->fp = file_opened[j].file;
          p->idx = n = j;
          goto done;
        }
      }

      if ((p->fp = fopen(fname,"wb")) == NULL)
        dies(Str(X_1465,"fprint: cannot open outfile %s"),fname);
      else { /* put the file in the opened stack */
        file_num++;
        if (file_num>=file_max) {
          if (file_max==0) atexit(close_files);
          file_max += 4;
          file_opened = (struct fileinTag*)
            mrealloc(file_opened, sizeof(struct fileinTag)*file_max);
        }
        file_opened[file_num].name = (char*) mmalloc(strlen(fname)+1);
        strcpy(file_opened[file_num].name, fname);
        file_opened[file_num].file = p->fp;
        p->idx = n = file_num;
        file_opened[file_num].cnt = 0;
        file_opened[file_num].hdr = 0;
      }
    }
    else { /* file handle as argument */
      n = (int)*p->fname;
      if (n>file_num || (p->fp = file_opened[n].file) == NULL)
        die(Str(X_1466,"fout: invalid file handle"));
    }

 done:
    /* Copy the string to the storage place in PRINTKS.
     *
     * We will look out for certain special codes and write special
     * bytes directly to the string.
     *
     * There is probably a more elegant way of doing this, then using
     * the look flag.  I could use goto - but I would rather not.      */
    /* This is really a if then else if...
     * construct and is currently grotty -- JPff */
    do {
      char temp  = *sarg++;
      char tempn = *sarg--;
      /* Look for a single caret and insert an escape char.  */
      if ((temp  == '^') && (tempn != '^')) {
        *sdest++ = 0x1B; /* ESC */
      }
      /* Look for a double caret and insert a single caret - stepping forward  one */
      else if ((temp  == '^') && (tempn == '^')) {
        *sdest++ = '^';
        sarg++;
      }
      /* Look for a single tilde and insert an escape followed by a '['.
       * ESC[ is the escape sequence for ANSI consoles */
      else if ((temp  == '~') && (tempn != '~')) {
        *sdest++ = 0x1B; /* ESC */
        *sdest++ = '[';
      }
      /* Look for a double tilde and insert a tilde caret - stepping forward one.  */
      else if ((temp  == '~') && (tempn == '~')) {
        *sdest++ = '~';
        sarg++;
      }
      /* Look for \n, \N etc */
      else if (temp == '\\') {
        switch (tempn) {
        case 'r': case 'R':
          *sdest++ = '\r';
          sarg++;
          break;
        case 'n': case 'N':
          *sdest++ = '\n';
          sarg++;
          break;
        case 't': case 'T':
          *sdest++ = '\t';
          sarg++;
          break;
        case 'a': case 'A':
          *sdest++ = '\a';
          sarg++;
          break;
        case 'b': case 'B':
          *sdest++ = '\b';
          sarg++;
          break;
        case '\\':
          *sdest++ = '\\';
          sarg++;
          break;
        default:
          *sdest++ = tempn;
          sarg++;
          break;
        }
      }
      else if (temp == '%') { /* an extra option to specify tab and return as %t and %r*/
        switch (tempn) {
        case 'r': case 'R':
          *sdest++ = '\r';
          sarg++;
          break;
        case 'n': case 'N':
          *sdest++ = '\n';
          sarg++;
          break;
        case 't': case 'T':
          *sdest++ = '\t';
          sarg++;
          break;
        case '!':       /* and a ';' */
          *sdest++ = ';';
          sarg++;
          break;
        default:
          *sdest++ = temp;
          break;
        }
      }
      else {
        /* If none of these match, then copy the character directly
         * and try again.      */
        *sdest++ = temp;
      }
      /* Increment pointer and process next character until end of string.  */
    } while (*++sarg != 0);
    return OK;
}

