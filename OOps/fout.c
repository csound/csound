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

#include <sndfile.h>
#include "fout.h"
#include "soundio.h"
#include <ctype.h>

#ifdef  USE_DOUBLE
#define sf_writef_MYFLT sf_writef_double
#define sf_readf_MYFLT  sf_readf_double
#else
#define sf_writef_MYFLT sf_writef_float
#define sf_readf_MYFLT  sf_readf_float
#endif

struct fileinTag {
    SNDFILE*    file;           /* Used in audio cases */
    FILE*       raw;            /* Only used if text file */
    char        *name;
    long        cnt;
};

#define file_opened ((struct fileinTag *)(csound->file_opened_))
#define file_max (csound->file_max_)
#define file_num (csound->file_num_)

static void close_files(void)
{
#ifdef ACCESS_TO_ENVIRON
    while (file_num>=0) {
      printf("%d (%s):", file_num, file_opened[file_num].name);
      if (file_opened[file_num].raw != NULL)
        fclose(file_opened[file_num].raw);
      else
        sf_close(file_opened[file_num].file);
      file_num--;
      printf(Str("\t... closed\n"));
    }
#endif
}

int outfile(ENVIRON *csound, OUTFILE *p)
{
    int j, nargs = p->nargs, k;
    MYFLT **args = p->argums;
    MYFLT vals[VARGMAX];
    if (p->fp==NULL) {
      FILE* fp = file_opened[p->idx].raw;
      for (k=0; k<ksmps; k++) {
        for (j=0;j<nargs;j++)
          fprintf(fp, "%g ", args[j][k]);
        fprintf(fp, "\n");
      }
    }
    else {
      for (k=0; k<ksmps; k++) {
        for (j = 0; j<nargs; j++)
          vals[j] = args[j][k];
        sf_writef_MYFLT(p->fp, vals, 1);
      }
    }
    return OK;
}


int outfile_set(ENVIRON *csound, OUTFILE *p)
{
    int n=0;
    SF_INFO sfinfo;

    p->nargs = p->INOCOUNT-2;
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
          return OK;
        }
      }
      /* Need to open file */
      switch((int) (*p->iflag+FL(0.5))) {
      case 0:
        sfinfo.format = SF_FORMAT_FLOAT | SF_FORMAT_RAW;
        break;
      case 1:
        sfinfo.format = SF_FORMAT_PCM_16 | SF_FORMAT_RAW;
        break;
      case 2:
        sfinfo.format = SF_FORMAT_PCM_16 | O.filetyp;
        p->cnt = 0;
        break;
      default:
        sfinfo.format = SF_FORMAT_PCM_16 | SF_FORMAT_RAW;
      }
      sfinfo.samplerate = (long)esr;
      sfinfo.channels = p->nargs;
      if ((p->fp = sf_open(fname,SFM_WRITE,&sfinfo)) == NULL)
        csound->Die(csound, Str("fout: cannot open outfile %s"),fname);
      else { /* put the file in the opened stack */
        file_num++;
        if (file_num>=file_max) {
          if (file_max==0) atexit(close_files);
          file_max += 4;        /* Expand by 4 each time */
          csound->file_opened_ = (void*)
            mrealloc(csound, file_opened, sizeof(struct fileinTag)*file_max);
        }
        file_opened[file_num].name = (char*) mmalloc(csound, strlen(fname)+1);
        strcpy(file_opened[file_num].name, fname);
        file_opened[file_num].file = p->fp;
        file_opened[file_num].raw = NULL;
        p->idx = n = file_num;
        file_opened[file_num].cnt = 0;
      }
    }
    else { /* file handle as argument */
      n = (int)*p->fname;
      if (n>file_num || ((p->fp = file_opened[n].file) == NULL &&
                         file_opened[n].raw == NULL))
        csound->Die(csound, Str("fout: invalid file handle"));
    }
    return OK;
}

int koutfile (ENVIRON *csound, KOUTFILE *p)
{
    int j, nargs = p->nargs;
    MYFLT **args = p->argums;
    MYFLT vals[VARGMAX];
    for (j = 0;j< nargs;j++) {
      vals[j] = *args[j];
    }
    sf_writef_MYFLT(p->fp, vals, 1);
    return OK;
}

int koutfile_set(ENVIRON *csound, KOUTFILE *p)
{
    int n;
    SF_INFO sfinfo;
    p->nargs = p->INOCOUNT-2;
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
      sfinfo.channels = p->nargs;
      sfinfo.samplerate = (long)esr;
      switch((int) (*p->iflag+FL(0.5))) {
      case 0:
        sfinfo.format = SF_FORMAT_FLOAT | SF_FORMAT_RAW;
        break;
      case 1:
        sfinfo.format = SF_FORMAT_PCM_16 | SF_FORMAT_RAW;
        break;
      case 2:
        sfinfo.format = SF_FORMAT_PCM_16 | O.filetyp;
        p->cnt = 0;
        break;
      default:
        sfinfo.format = SF_FORMAT_PCM_16 | SF_FORMAT_RAW;
      }
      p->nargs = p->INOCOUNT-2;
      if ((p->fp = sf_open(fname, SFM_WRITE, &sfinfo)) == NULL)
        csound->Die(csound, Str("foutk: cannot open outfile %s"),fname);
      else { /* put the file in the opened stack */
        file_num++;
        if (file_num>=file_max) {
          if (file_max==0) atexit(close_files);
          file_max += 4;
          csound->file_opened_ = (void*)
            mrealloc(csound, file_opened, sizeof(struct fileinTag)*file_max);
        }
        file_opened[file_num].name = (char*)mmalloc(csound, strlen(fname)+1);
        strcpy(file_opened[file_num].name, fname);
        file_opened[file_num].file=p->fp;
        p->idx = n = file_num;
        file_opened[file_num].cnt = 0;
      }
    }
    else { /* file handle argument */
      n = (int)*p->fname;
      if (n>file_num || (p->fp = file_opened[n].file) == NULL)
        csound->Die(csound, Str("fout: invalid file handle"));
    }
 done:
    p->cnt = 0;
    return OK;
}

/*--------------*/


/* syntax:
        ihandle fiopen "filename" [, iascii]
*/
int fiopen(ENVIRON *csound, FIOPEN *p)          /* open a file and return its handle  */
{                              /* the handle is simply a stack index */
    char fname[FILENAME_MAX];
    char *omodes[] = {"w", "r", "wb", "rb"};
    FILE *rfp = NULL;
    int idx = (int)*p->iascii;
    strcpy(fname, unquote(p->STRARG));
    if (idx<0 || idx>3) idx=0;
    if ((rfp = fopen(fname,omodes[idx])) == NULL)
      csound->Die(csound, Str("fout: cannot open outfile %s"),fname);
    if (idx>1) setbuf(rfp, NULL);
    file_num++;
    if (file_num>=file_max) {
      if (file_max==0) atexit(close_files);
      file_max += 4;
      csound->file_opened_ = (void*)
        mrealloc(csound, file_opened, sizeof(struct fileinTag)*file_max);
    }
    file_opened[file_num].name = (char*)mmalloc(csound, strlen(fname)+1);
    strcpy(file_opened[file_num].name, fname);
    file_opened[file_num].file=NULL;
    file_opened[file_num].raw=rfp;
    *p->ihandle = (MYFLT) file_num;
    return OK;
    }

/* syntax:
   fouti  ihandle, iascii, iflag, iarg1 [,iarg2,....,iargN]
*/

static long kreset=0;
int ioutfile_set(ENVIRON *csound, IOUTFILE *p)
{
    int j;
    MYFLT **args=p->argums;
    FILE* rfil;
    int n = (int) *p->ihandle;
    if (n<0 || n>file_num)
      csound->Die(csound, Str("fouti: invalid file handle"));
    rfil = file_opened[n].raw;
    if (rfil == NULL) csound->Die(csound, Str("fouti: invalid file handle"));
    if (*p->iascii == 0) { /* ascii format */
      switch ((int) *p->iflag) {
      case 1: { /* with prefix (i-statement, p1, p2 and p3) */
        int p1 = (int) p->h.insdshead->insno;
        double p2 =   (double) kcounter * onedkr;
        double p3 = p->h.insdshead->p3;
        if (p3 > FL(0.0))
          fprintf(rfil, "i %i %f %f ", p1, p2, p3);
        else
          fprintf(rfil, "i %i %f . ", p1, p2);
      }
      break;
      case 2: /* with prefix (start at 0 time) */
        if (kreset == 0) kreset = kcounter;
        {
          int p1 = (int) p->h.insdshead->insno;
          double p2= (double) (kcounter - kreset) * onedkr;
          double p3 = p->h.insdshead->p3;
          if (p3 > FL(0.0))
            fprintf(rfil, "i %i %f %f ", p1, p2, p3);
          else
            fprintf(rfil, "i %i %f . ", p1, p2);
        }
        break;
      case 3: /* reset */
        kreset=0;
        return OK;
      }
      for (j=0; j < p->INOCOUNT - 3;j++) {
        fprintf(rfil, " %f",(double) *args[j]);
      }
      putc('\n',rfil);
    }
    else { /* binary format */
      for (j=0; j < p->INOCOUNT - 3;j++) {
        fwrite(args[j], sizeof(MYFLT),1, rfil);
      }
    }
    return OK;
}


int ioutfile_set_r(ENVIRON *csound, IOUTFILE_R *p)
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


int ioutfile_r(ENVIRON *csound, IOUTFILE_R *p)
{
    if (p->h.insdshead->relesing) {
      if (p->done) {
        int j;
        MYFLT **args=p->argums;
        FILE *rfil;
        int n = (int) *p->ihandle;
        if (n<0 || n>file_num)
          csound->Die(csound, Str("fouti: invalid file handle"));
        rfil = file_opened[n].raw;
        if (rfil == NULL)
          csound->Die(csound, Str("fouti: invalid file handle"));
        if (*p->iascii == 0) { /* ascii format */
          switch ((int) *p->iflag) {
          case 1:       {       /* whith prefix (i-statement, p1, p2 and p3) */
            int p1 = (int) p->h.insdshead->insno;
            double p2 = p->counter * onedkr;
            double p3 = (double) (kcounter-p->counter) * onedkr;
            fprintf(rfil, "i %i %f %f ", p1, p2, p3);
          }
            break;
          case 2: /* with prefix (start at 0 time) */
            {
              int p1 = (int) p->h.insdshead->insno;
              double p2 = (p->counter - kreset) *onedkr;
              double p3 = (double) (kcounter-p->counter) * onedkr;
              fprintf(rfil, "i %i %f %f ", p1, p2, p3);
            }
            break;
          case 3: /* reset */
            kreset=0;
            return OK;
          }
          for (j=0; j < p->INOCOUNT - 3;j++) {
            fprintf(rfil, " %f",(double) *args[j]);
          }
          putc('\n',rfil);
        }
        else { /* binary format */
          for (j=0; j < p->INOCOUNT - 3;j++) {
            fwrite(args[j], sizeof(MYFLT),1, rfil);
          }
        }
        p->done = 0;
      }
    }
    return OK;
}

/*----------------------------------*/

int infile_set(ENVIRON *csound, INFILE *p)
{
    SF_INFO sfinfo;
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
      if (( p->fp = sf_open(fname, SFM_READ, &sfinfo)) == NULL)
        csound->Die(csound, Str("fin: cannot open infile %s"),fname);
      else { /* put the file in the opened stack */
        file_num++;
        if (file_num>=file_max) {
          if (file_max==0) atexit(close_files);
          file_max += 4;
          csound->file_opened_ = (void*)
            mrealloc(csound, file_opened, sizeof(struct fileinTag)*file_max);
        }
        file_opened[file_num].name = (char*)mmalloc(csound, strlen(fname)+1);
        strcpy(file_opened[file_num].name, fname);
        file_opened[file_num].file=p->fp;
        file_opened[file_num].raw=NULL;
      }
    }
    else { /* file handle argument */
      int n = (int) *p->fname;
      if (n<0 || n> file_num || (p->fp = file_opened[n].file) == NULL)
        csound->Die(csound, Str("fin: invalid file handle"));
    }
 done:
    p->nargs = p->INOCOUNT-3;
    p->currpos = (long) *p->iskpfrms;
    p->flag=1;
    return OK;
}

int infile_act(ENVIRON *csound, INFILE *p)
{
    int j, nargs = p->nargs,k=0;
    MYFLT **args = p->argums;
    if (p->flag) {
      sf_seek(p->fp, p->currpos, SEEK_SET);
      p->currpos+=ksmps;
      for (k=0; k<ksmps; k++) {
        MYFLT vals[VARGMAX];
        if (sf_readf_MYFLT(p->fp, vals, 1)) {
          for (j=0; j< nargs; j++) args[j][k] = vals[j];
        }
        else {
          p->flag = 0;
          for (j=0; j< nargs; j++) args[j][k] = FL(0.0);
        }
      };
    }
    else { /* after end of file */
      for (k=0; k<ksmps; k++) {
        for (j=0; j< nargs; j++)
          args[j][k] = FL(0.0);
      }
    }
    return OK;
}


/*----------------------------*/


int kinfile_set(ENVIRON *csound, KINFILE *p)
{
    SF_INFO sfinfo;
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
      if (( p->fp = sf_open(fname,SFM_READ, &sfinfo)) == NULL)
        csound->Die(csound, Str("fin: cannot open infile %s"),fname);
      else { /* put the file in the opened stack */
        file_num++;
        if (file_num>=file_max) {
          if (file_max==0) atexit(close_files);
          file_max += 4;
          csound->file_opened_ = (void*)
            mrealloc(csound, file_opened, sizeof(struct fileinTag)*file_max);
        }
        file_opened[file_num].name = (char*)mmalloc(csound, strlen(fname)+1);
        strcpy(file_opened[file_num].name, fname);
        file_opened[file_num].file=p->fp;
      }
    }
    else {/* file handle argument */
      int n = (int) *p->fname;
      if (n<0 || n>file_num || (p->fp = file_opened[n].file) == NULL)
        csound->Die(csound, Str("fink: invalid file handle"));
    }
 done:
    p->nargs = p->INOCOUNT-3;
    p->currpos = (long) *p->iskpfrms;
    p->flag=1;
    return OK;
}


int kinfile(ENVIRON *csound, KINFILE *p)
{
    int j, nargs = p->nargs;
    MYFLT **args = p->argums;
    if (p->flag) {
      sf_seek(p->fp, p->currpos, SEEK_SET);
      p->currpos++;
      for (j = 0;j< nargs;j++) {
        if (sf_read_MYFLT(p->fp, args[j], 1));
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


int i_infile(ENVIRON *csound, I_INFILE *p)
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
          fp = file_opened[j].raw;
          goto done;
        }
      }
      idx = (int) (*p->iflag+FL(0.5));
      if (idx<0 || idx>2) idx = 0;
      if (( fp = fopen(fname,omodes[idx])) == NULL)
        csound->Die(csound, Str("fin: cannot open infile %s"),fname);
      else { /* put the file in the opened stack */
        file_num++;
        if (file_num>=file_max) {
          if (file_max==0) atexit(close_files);
          file_max += 4;
          csound->file_opened_ = (void*)
            mrealloc(csound, file_opened, sizeof(struct fileinTag)*file_max);
        }
        file_opened[file_num].name = (char*)mmalloc(csound, strlen(fname)+1);
        strcpy(file_opened[file_num].name, fname);
        file_opened[file_num].raw=fp;
        file_opened[file_num].file=NULL;
      }
    }
    else {/* file handle argument */
      int n = (int) *p->fname;
      if (n<0 || n>file_num || (fp = file_opened[n].raw) == NULL)
        csound->Die(csound, Str("fink: invalid file handle"));
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

int incr(ENVIRON *csound, INCR *p)
{
    MYFLT *avar = p->avar, *aincr = p->aincr;
    int n;
    for (n=0; n<ksmps; n++)
      avar[n] += aincr[n];
    return OK;
}


int clear(ENVIRON *csound, CLEARS *p)
{
    int n, j;
    MYFLT *avar;
    for (j=0;j< p->INOCOUNT;j++) {
      avar = p->argums[j];
      for (n=0; n<ksmps; n++)
        avar[n] = FL(0.0);
    }
    return OK;
}



/*---------------------------*/
/* formatted output to a text file */

int fprintf_set(ENVIRON *csound, FPRINTF *p)
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
          p->fp = file_opened[j].raw;
          p->idx = n = j;
          goto done;
        }
      }

      if ((p->fp = fopen(fname,"wb")) == NULL)
        csound->Die(csound, Str("fprint: cannot open outfile %s"),fname);
      else { /* put the file in the opened stack */
        file_num++;
        if (file_num>=file_max) {
          if (file_max==0) atexit(close_files);
          file_max += 4;
          csound->file_opened_ = (void*)
            mrealloc(csound, file_opened, sizeof(struct fileinTag)*file_max);
        }
        file_opened[file_num].name = (char*) mmalloc(csound, strlen(fname)+1);
        strcpy(file_opened[file_num].name, fname);
        file_opened[file_num].raw = p->fp;
        p->idx = n = file_num;
        file_opened[file_num].cnt = 0;
      }
    }
    else { /* file handle as argument */
      n = (int)*p->fname;
      if (n>file_num || (p->fp = file_opened[n].raw) == NULL)
        csound->Die(csound, Str("fout: invalid file handle"));
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

