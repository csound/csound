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

#include "csoundCore.h"
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
    SNDFILE     *file;          /* Used in audio cases */
    FILE        *raw;           /* Only used if text file */
    void        *fd;
    char        *name;
    char        *fullName;
    int         do_scale;
};

static int fout_open_file(ENVIRON *csound, FILE **f,
                                           const char *name, const char *mode)
{
    struct fileinTag  *p = (struct fileinTag*) csound->file_opened;
    void              *fd;
    int               i;

    *f = (FILE*) NULL;
    if (name == NULL || name[0] == '\0')
      return -1;
    /* is this file already open ? */
    for (i = 0; i <= csound->file_num; i++) {
      if (p[i].raw != NULL && strcmp(p[i].name, name) == 0) {
        *f = p[i].raw;
        return i;
      }
    }
    /* attempt to open file */
    fd = csound->FileOpen(csound, f, CSFILE_STD, name, (void*) mode, "");
    if (fd == NULL)
      return -1;
    /* allocate new entry */
    if (!((++csound->file_num) & 3)) {
      /* Expand by 4 each time */
      csound->file_max = csound->file_num + 4;
      if (!csound->file_num)
        p = (struct fileinTag *) mmalloc(csound, sizeof(struct fileinTag)
                                                 * csound->file_max);
      else
        p = (struct fileinTag *) mrealloc(csound, csound->file_opened,
                                                  sizeof(struct fileinTag)
                                                  * csound->file_max);
      csound->file_opened = (void*) p;
      for (i = csound->file_num; i < csound->file_max; i++)
        memset(&(p[i]), 0, sizeof(struct fileinTag));
    }
    /* store file information */
    i = csound->file_num;
    p[i].file = (SNDFILE*) NULL;
    p[i].raw = *f;
    p[i].fd = fd;
    p[i].fullName = csound->GetFileName(fd);
    p[i].name = (char*) mmalloc(csound, strlen(name) + 1);
    strcpy(p[i].name, name);
    /* return with file handle */
    return i;
}

static int fout_open_sndfile(ENVIRON *csound, SNDFILE **sf, const char *name,
                                              int write_mode, SF_INFO *sfinfo)
{
    struct fileinTag  *p = (struct fileinTag*) csound->file_opened;
    void              *fd;
    int               i, do_scale = 0;

    *sf = (SNDFILE*) NULL;
    if (name == NULL || name[0] == '\0')
      return -1;
    /* is this file already open ? */
    for (i = 0; i <= csound->file_num; i++) {
      if (p[i].file != NULL && strcmp(p[i].name, name) == 0) {
        *sf = p[i].file;
        return i;
      }
    }
    /* attempt to open file */
    if (write_mode) {
      do_scale = (SF2TYPE(sfinfo->format) == TYP_RAW ? 0 : 1);
      fd = csound->FileOpen(csound, sf, CSFILE_SND_W, name, sfinfo,
                                    "SFDIR");
    }
    else {
      fd = csound->FileOpen(csound, sf, CSFILE_SND_R, name, sfinfo,
                                    "SFDIR;SSDIR");
      do_scale = (SF2TYPE(sfinfo->format) == TYP_RAW ? 0 : 1);
    }
    if (fd == NULL)
      return -1;
    if (!do_scale) {
#ifdef USE_DOUBLE
      sf_command(*sf, SFC_SET_NORM_DOUBLE, NULL, SF_FALSE);
#else
      sf_command(*sf, SFC_SET_NORM_FLOAT, NULL, SF_FALSE);
#endif
    }
    /* allocate new entry */
    if (!((++csound->file_num) & 3)) {
      /* Expand by 4 each time */
      csound->file_max = csound->file_num + 4;
      if (!csound->file_num)
        p = (struct fileinTag *) mmalloc(csound, sizeof(struct fileinTag)
                                                 * csound->file_max);
      else
        p = (struct fileinTag *) mrealloc(csound, csound->file_opened,
                                                  sizeof(struct fileinTag)
                                                  * csound->file_max);
      csound->file_opened = (void*) p;
      for (i = csound->file_num; i < csound->file_max; i++)
        memset(&(p[i]), 0, sizeof(struct fileinTag));
    }
    /* store file information */
    i = csound->file_num;
    p[i].file = *sf;
    p[i].raw = (FILE*) NULL;
    p[i].fd = fd;
    p[i].fullName = csound->GetFileName(fd);
    p[i].do_scale = do_scale;
    p[i].name = (char*) mmalloc(csound, strlen(name) + 1);
    strcpy(p[i].name, name);
    /* return with file handle */
    return i;
}

void foutRESET(ENVIRON *csound)
{
    struct fileinTag *file_opened = (struct fileinTag*) csound->file_opened;

    while (csound->file_num >= 0) {
      if (csound->oparms->msglevel & 3)
        csound->Message(csound, "%d (%s):",
                                csound->file_num,
                                file_opened[csound->file_num].fullName);
      mfree(csound, file_opened[csound->file_num].name);
      if (file_opened[csound->file_num].fd != NULL)
        csound->FileClose(csound, file_opened[csound->file_num].fd);
      csound->file_num--;
      if (csound->oparms->msglevel & 3)
        csound->Message(csound, Str("\t... closed\n"));
    }
    mfree(csound, csound->file_opened);
    csound->file_opened = NULL;
    csound->file_max = 0;
}

int outfile(ENVIRON *csound, OUTFILE *p)
{
    int   j, nargs = p->nargs, k;
    MYFLT **args = p->argums;
    MYFLT vals[VARGMAX];

    if (p->fp == NULL) {
      FILE *fp = ((struct fileinTag*) csound->file_opened)[p->idx].raw;
      for (k = 0; k < csound->ksmps; k++) {
        for (j = 0; j < nargs; j++)
          fprintf(fp, "%g ", args[j][k]);
        fprintf(fp, "\n");
      }
    }
    else {
      for (k = 0; k < csound->ksmps; k++) {
        for (j = 0; j < nargs; j++)
          vals[j] = args[j][k] * p->scaleFac;
        sf_writef_MYFLT(p->fp, vals, 1);
      }
    }
    return OK;
}

int outfile_set(ENVIRON *csound, OUTFILE *p)
{
    int     n;
    SF_INFO sfinfo;

    p->nargs = p->INOCOUNT - 2;
    if (p->XSTRCODE || *p->fname == SSTRCOD) { /* if char string name given */
      char fname[FILENAME_MAX];
      csound->strarg2name(csound, fname, p->fname, "fout.", p->XSTRCODE);
      /* Need to open file */
      memset(&sfinfo, 0, sizeof(SF_INFO));
      p->flag = (int) MYFLT2LRND(*p->iflag);
      switch (p->flag) {
      case 0:
        sfinfo.format = SF_FORMAT_FLOAT | SF_FORMAT_RAW;
        break;
      case 1:
        sfinfo.format = SF_FORMAT_PCM_16 | SF_FORMAT_RAW;
        break;
      case 2:
        sfinfo.format = SF_FORMAT_PCM_16 | TYPE2SF(csound->oparms->filetyp);
        break;
      default:
        sfinfo.format = SF_FORMAT_PCM_16 | SF_FORMAT_RAW;
      }
      sfinfo.samplerate = (int) MYFLT2LRND(csound->esr);
      sfinfo.channels = p->nargs;
      if ((p->idx = fout_open_sndfile(csound, &(p->fp), fname, 1, &sfinfo)) < 0)
        csound->Die(csound, Str("fout: cannot open outfile %s"), fname);
    }
    else { /* file handle as argument */
      n = (int) MYFLT2LRND(*p->fname);
      if (n < 0 || n > csound->file_num ||
          ((p->fp=((struct fileinTag*) csound->file_opened)[n].file) == NULL &&
           ((struct fileinTag*) csound->file_opened)[n].raw == NULL))
        csound->Die(csound, Str("fout: invalid file handle"));
      p->idx = n;
    }
    if (((struct fileinTag*) csound->file_opened)[p->idx].do_scale)
      p->scaleFac = csound->dbfs_to_float;
    else
      p->scaleFac = FL(1.0);
    return OK;
}

int koutfile(ENVIRON *csound, KOUTFILE *p)
{
    int   j, nargs = p->nargs;
    MYFLT **args = p->argums;
    MYFLT vals[VARGMAX];

    for (j = 0; j < nargs; j++) {
      vals[j] = *args[j] * p->scaleFac;
    }
    sf_writef_MYFLT(p->fp, vals, 1);
    return OK;
}

int koutfile_set(ENVIRON *csound, KOUTFILE *p)
{
    int     n;
    SF_INFO sfinfo;

    p->nargs = p->INOCOUNT - 2;
    if (p->XSTRCODE || *p->fname == SSTRCOD) { /* if char string name given */
      char fname[FILENAME_MAX];
      csound->strarg2name(csound, fname, p->fname, "fout.", p->XSTRCODE);
      memset(&sfinfo, 0, sizeof(SF_INFO));
      sfinfo.channels = p->nargs;
      sfinfo.samplerate = (int) MYFLT2LRND(csound->ekr);
      p->flag = (int) MYFLT2LRND(*p->iflag);
      switch (p->flag) {
      case 0:
        sfinfo.format = SF_FORMAT_FLOAT | SF_FORMAT_RAW;
        break;
      case 1:
        sfinfo.format = SF_FORMAT_PCM_16 | SF_FORMAT_RAW;
        break;
      case 2:
        sfinfo.format = SF_FORMAT_PCM_16 | TYPE2SF(csound->oparms->filetyp);
        break;
      default:
        sfinfo.format = SF_FORMAT_PCM_16 | SF_FORMAT_RAW;
      }
      if ((p->idx = fout_open_sndfile(csound, &(p->fp), fname, 1, &sfinfo)) < 0)
        csound->Die(csound, Str("foutk: cannot open outfile %s"), fname);
    }
    else { /* file handle argument */
      n = (int) MYFLT2LRND(*p->fname);
      if (n < 0 || n > csound->file_num ||
          (p->fp = ((struct fileinTag*) csound->file_opened)[n].file) == NULL)
        csound->Die(csound, Str("foutk: invalid file handle"));
      p->idx = n;
    }
    if (((struct fileinTag*) csound->file_opened)[p->idx].do_scale)
      p->scaleFac = csound->dbfs_to_float;
    else
      p->scaleFac = FL(1.0);
    return OK;
}

/*--------------*/

/* syntax:
        ihandle fiopen "filename" [, iascii]
*/
int fiopen(ENVIRON *csound, FIOPEN *p)  /* open a file and return its handle  */
{                                       /* the handle is simply a stack index */
    char    fname[FILENAME_MAX];
    char    *omodes[] = {"w", "r", "wb", "rb"};
    FILE    *rfp;
    int     idx = (int) MYFLT2LRND(*p->iascii), n;

    strcpy(fname, (char*) p->fname);
    if (idx < 0 || idx > 3)
      idx = 0;
    if ((n = fout_open_file(csound, &rfp, fname, omodes[idx])) < 0)
      csound->Die(csound, Str("fout: cannot open outfile %s"), fname);
    if (idx > 1)
      setbuf(rfp, NULL);
    *p->ihandle = (MYFLT) n;
    return OK;
}

/* syntax:
   fouti  ihandle, iascii, iflag, iarg1 [, iarg2, ...., iargN]
*/

int ioutfile_set(ENVIRON *csound, IOUTFILE *p)
{
    int     j;
    MYFLT   **args = p->argums;
    FILE    *rfil;
    int     n = (int) MYFLT2LRND(*p->ihandle);

    if (n < 0 || n > csound->file_num)
      csound->Die(csound, Str("fouti: invalid file handle"));
    rfil = ((struct fileinTag*) csound->file_opened)[n].raw;
    if (rfil == NULL) csound->Die(csound, Str("fouti: invalid file handle"));
    if (*p->iascii == 0) { /* ascii format */
      switch ((int) MYFLT2LRND(*p->iflag)) {
      case 1:
        {     /* with prefix (i-statement, p1, p2 and p3) */
          int     p1 = (int) p->h.insdshead->p1;
          double  p2 = (double) csound->kcounter * csound->onedkr;
          double  p3 = p->h.insdshead->p3;
          if (p3 > FL(0.0))
            fprintf(rfil, "i %i %f %f ", p1, p2, p3);
          else
            fprintf(rfil, "i %i %f . ", p1, p2);
        }
        break;
      case 2: /* with prefix (start at 0 time) */
        if (csound->fout_kreset == 0)
          csound->fout_kreset = csound->kcounter;
        {
          int p1 = (int) p->h.insdshead->p1;
          double p2 = (double) (csound->kcounter - csound->fout_kreset)
                      * csound->onedkr;
          double p3 = p->h.insdshead->p3;
          if (p3 > FL(0.0))
            fprintf(rfil, "i %i %f %f ", p1, p2, p3);
          else
            fprintf(rfil, "i %i %f . ", p1, p2);
        }
        break;
      case 3: /* reset */
        csound->fout_kreset = 0;
        return OK;
      }
      for (j = 0; j < p->INOCOUNT - 3; j++) {
        fprintf(rfil, " %f", (double) *args[j]);
      }
      putc('\n', rfil);
    }
    else { /* binary format */
      for (j = 0; j < p->INOCOUNT - 3; j++) {
        fwrite(args[j], sizeof(MYFLT), 1, rfil);
      }
    }
    return OK;
}

int ioutfile_set_r(ENVIRON *csound, IOUTFILE_R *p)
{
    if (p->h.insdshead->xtratim < 1)
      p->h.insdshead->xtratim = 1;
    p->counter =  csound->kcounter;
    p->done = 1;
    if (*p->iflag == 2 && csound->fout_kreset == 0)
      csound->fout_kreset = csound->kcounter;
    return OK;
}

int ioutfile_r(ENVIRON *csound, IOUTFILE_R *p)
{
    int   j;
    MYFLT **args;
    FILE  *rfil;
    int   n;

    if (!p->h.insdshead->relesing || !p->done)
      return OK;

    args = p->argums;
    n = (int) MYFLT2LRND(*p->ihandle);
    if (n < 0 || n > csound->file_num)
      csound->Die(csound, Str("fouti: invalid file handle"));
    rfil = ((struct fileinTag*) csound->file_opened)[n].raw;
    if (rfil == NULL)
      csound->Die(csound, Str("fouti: invalid file handle"));
    if (*p->iascii == 0) { /* ascii format */
      switch ((int) MYFLT2LRND(*p->iflag)) {
      case 1:
        {     /* whith prefix (i-statement, p1, p2 and p3) */
          int p1 = (int) p->h.insdshead->p1;
          double p2 = p->counter * csound->onedkr;
          double p3 = (double) (csound->kcounter - p->counter)
                      * csound->onedkr;
          fprintf(rfil, "i %i %f %f ", p1, p2, p3);
        }
        break;
      case 2: /* with prefix (start at 0 time) */
        {
          int p1 = (int) p->h.insdshead->p1;
          double p2 = (p->counter - csound->fout_kreset) *csound->onedkr;
          double p3 = (double) (csound->kcounter - p->counter)
                      * csound->onedkr;
          fprintf(rfil, "i %i %f %f ", p1, p2, p3);
        }
        break;
      case 3: /* reset */
        csound->fout_kreset = 0;
        return OK;
      }
      for (j = 0; j < p->INOCOUNT - 3;j++) {
        fprintf(rfil, " %f", (double) *args[j]);
      }
      putc('\n', rfil);
    }
    else { /* binary format */
      for (j = 0; j < p->INOCOUNT - 3; j++) {
        fwrite(args[j], sizeof(MYFLT), 1, rfil);
      }
    }
    p->done = 0;

    return OK;
}

/*----------------------------------*/

int infile_set(ENVIRON *csound, INFILE *p)
{
    SF_INFO sfinfo;
    int     n = 0;

    if (p->XSTRCODE || *p->fname == SSTRCOD) { /* if char string name given */
      char fname[FILENAME_MAX];
      csound->strarg2name(csound, fname, p->fname, "fout.", p->XSTRCODE);
      memset(&sfinfo, 0, sizeof(SF_INFO));
      sfinfo.samplerate = (int) MYFLT2LRND(csound->esr);
      if ((int) MYFLT2LRND(*p->iflag) == 0)
        sfinfo.format = FORMAT2SF(AE_FLOAT) | TYPE2SF(TYP_RAW);
      else
        sfinfo.format = FORMAT2SF(AE_SHORT) | TYPE2SF(TYP_RAW);
      sfinfo.channels = p->INOCOUNT - 3;
      if ((n = fout_open_sndfile(csound, &(p->fp), fname, 0, &sfinfo)) < 0)
        csound->Die(csound, Str("fin: cannot open infile %s"), fname);
    }
    else { /* file handle argument */
      n = (int) MYFLT2LRND(*p->fname);
      if (n < 0 || n > csound->file_num ||
          (p->fp = ((struct fileinTag*) csound->file_opened)[n].file) == NULL)
        csound->Die(csound, Str("fin: invalid file handle"));
    }
    if (((struct fileinTag*) csound->file_opened)[n].do_scale)
      p->scaleFac = csound->e0dbfs;
    else
      p->scaleFac = FL(1.0);
    p->nargs = p->INOCOUNT - 3;
    p->currpos = MYFLT2LRND(*p->iskpfrms);
    p->flag = 1;
    return OK;
}

int infile_act(ENVIRON *csound, INFILE *p)
{
    int   j, nargs = p->nargs, k = 0;
    MYFLT **args = p->argums;

    if (p->flag) {
      sf_seek(p->fp, p->currpos, SEEK_SET);
      p->currpos += csound->ksmps;
      for (k = 0; k < csound->ksmps; k++) {
        MYFLT vals[VARGMAX];
        if (sf_readf_MYFLT(p->fp, vals, 1)) {
          for (j = 0; j < nargs; j++)
            args[j][k] = vals[j] * p->scaleFac;
        }
        else {
          p->flag = 0;
          for (j = 0; j < nargs; j++)
            args[j][k] = FL(0.0);
        }
      }
    }
    else { /* after end of file */
      for (k = 0; k < csound->ksmps; k++) {
        for (j = 0; j < nargs; j++)
          args[j][k] = FL(0.0);
      }
    }
    return OK;
}

/*----------------------------*/

int kinfile_set(ENVIRON *csound, KINFILE *p)
{
    SF_INFO sfinfo;
    int     n = 0;

    if (p->XSTRCODE || *p->fname == SSTRCOD) { /* if char string name given */
      char fname[FILENAME_MAX];
      csound->strarg2name(csound, fname, p->fname, "fout.", p->XSTRCODE);
      memset(&sfinfo, 0, sizeof(SF_INFO));
      sfinfo.samplerate = (int) MYFLT2LRND(csound->ekr);
      if ((int) MYFLT2LRND(*p->iflag) == 0)
        sfinfo.format = FORMAT2SF(AE_FLOAT) | TYPE2SF(TYP_RAW);
      else
        sfinfo.format = FORMAT2SF(AE_SHORT) | TYPE2SF(TYP_RAW);
      sfinfo.channels = p->INOCOUNT - 3;
      if ((n = fout_open_sndfile(csound, &(p->fp), fname, 0, &sfinfo)) < 0)
        csound->Die(csound, Str("fink: cannot open infile %s"), fname);
    }
    else {                      /* file handle argument */
      n = (int) MYFLT2LRND(*p->fname);
      if (n < 0 || n > csound->file_num ||
          (p->fp = ((struct fileinTag*) csound->file_opened)[n].file) == NULL)
        csound->Die(csound, Str("fink: invalid file handle"));
    }
    if (((struct fileinTag*) csound->file_opened)[n].do_scale)
      p->scaleFac = csound->e0dbfs;
    else
      p->scaleFac = FL(1.0);
    p->nargs = p->INOCOUNT - 3;
    p->currpos = MYFLT2LRND(*p->iskpfrms);
    p->flag = 1;
    return OK;
}

int kinfile(ENVIRON *csound, KINFILE *p)
{
    int   j, nargs = p->nargs;
    MYFLT **args = p->argums;

    if (p->flag) {
      sf_seek(p->fp, p->currpos, SEEK_SET);
      p->currpos++;
      for (j = 0; j < nargs; j++) {
        if (sf_read_MYFLT(p->fp, args[j], 1))
          *(args[j]) *= p->scaleFac;
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
    int   j, n, nargs;
    FILE  *fp = NULL;
    MYFLT **args = p->argums;

    if (p->XSTRCODE || *p->fname == SSTRCOD) {  /* if char string name given */
      char  fname[FILENAME_MAX];
      char  *omodes[] = {"r", "r", "rb"};
      int   idx = (int) MYFLT2LRND(*p->iflag);
      csound->strarg2name(csound, fname, p->fname, "fout.", p->XSTRCODE);
      if (idx < 0 || idx > 2)
        idx = 0;
      if ((n = fout_open_file(csound, &fp, fname, omodes[idx])) < 0)
        csound->Die(csound, Str("fini: cannot open infile %s"), fname);
    }
    else {/* file handle argument */
      n = (int) MYFLT2LRND(*p->fname);
      if (n < 0 || n > csound->file_num ||
          (fp = ((struct fileinTag*) csound->file_opened)[n].raw) == NULL)
        csound->Die(csound, Str("fini: invalid file handle"));
    }

    nargs = p->INOCOUNT - 3;
    switch ((int) MYFLT2LRND(*p->iflag)) {
    case 0: /* ascii file with loop */
      {
        char cf[20], *cfp;
        int cc;
      newcycle:
        for (j = 0; j < nargs; j++) {
          cfp = cf;
          while ((*cfp = cc = getc(fp)) == 'i' || isspace(*cfp));
          if (cc == EOF) {
            fseek(fp, 0, SEEK_SET);
            goto newcycle;
          }
          while (isdigit(*cfp) || *cfp == '.')  {
            *(++cfp) = cc = getc(fp);
          }
          *++cfp = '\0';        /* Must terminate string */
          *(args[j]) = (MYFLT) atof(cf);
          if (cc == EOF) {
            fseek(fp, 0, SEEK_SET);
            break;
          }
        }
      }
      break;
    case 1: /* ascii file without loop */
      {
        char cf[20], *cfp;
        int cc;
        for (j = 0; j < nargs; j++) {
          cfp = cf;
          while ((*cfp = cc = getc(fp)) == 'i' || isspace(*cfp));
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
      fseek(fp, p->currpos * sizeof(float) * nargs, SEEK_SET);
      p->currpos++;
      for (j = 0; j < nargs; j++) {
        if (fread(args[j], sizeof(float), 1, fp));
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
    int   n;

    for (n = 0; n < csound->ksmps; n++)
      avar[n] += aincr[n];
    return OK;
}

int clear(ENVIRON *csound, CLEARS *p)
{
    int   n, j;
    MYFLT *avar;

    for (j = 0; j < p->INOCOUNT; j++) {
      avar = p->argums[j];
      for (n = 0; n < csound->ksmps; n++)
        avar[n] = FL(0.0);
    }
    return OK;
}

/*---------------------------------*/
/* formatted output to a text file */

int fprintf_set(ENVIRON *csound, FPRINTF *p)
{
    int   n;
    char  *sarg = (char*) p->fmt;
    char  *sdest = p->txtstring;

    memset(p->txtstring, 0, 8192); /* Nasty to have exposed constant in code */

    if ((p->XSTRCODE & 1) || *p->fname == SSTRCOD) {
      /* if char string name given */
      char fname[FILENAME_MAX];
      csound->strarg2name(csound, fname, p->fname, "fout.", p->XSTRCODE);
      if ((p->idx = fout_open_file(csound, &(p->fp), fname, "w")) < 0)
        csound->Die(csound, Str("fprint: cannot open outfile %s"), fname);
    }
    else { /* file handle as argument */
      n = (int) MYFLT2LRND(*p->fname);
      if (n < 0 || n > csound->file_num ||
          (p->fp = ((struct fileinTag*) csound->file_opened)[n].raw) == NULL)
        csound->Die(csound, Str("fout: invalid file handle"));
      p->idx = n;
    }

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
      /* Look for a double caret and insert a single caret
         - stepping forward one */
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
      /* Look for a double tilde and insert a tilde caret
         - stepping forward one. */
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
      else if (temp == '%') {
        /* an extra option to specify tab and return as %t and %r */
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

