/*
    fout.c:

    Copyright (C) 1999 Gabriel Maldonado, John ffitch, Matt Ingalls
              (C) 2005 Istvan Varga

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

#include "csdl.h"
#include <sndfile.h>
#include "fout.h"
#include "soundio.h"
#include <ctype.h>

static int fout_open_file(FOUT_GLOBALS *pp, FILE **f,
                                            const char *name, const char *mode)
{
    CSOUND            *csound = pp->csound;
    struct fileinTag  *p = (struct fileinTag*) pp->file_opened;
    void              *fd;
    int               i;

    *f = (FILE*) NULL;
    if (name == NULL || name[0] == '\0')
      return -1;
    /* is this file already open ? */
    for (i = 0; i <= pp->file_num; i++) {
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
    if (!((++pp->file_num) & 3)) {
      /* Expand by 4 each time */
      pp->file_max = pp->file_num + 4;
      p = (struct fileinTag *) csound->ReAlloc(csound, pp->file_opened,
                                                       sizeof(struct fileinTag)
                                                       * pp->file_max);
      pp->file_opened = (void*) p;
      for (i = pp->file_num; i < pp->file_max; i++)
        memset(&(p[i]), 0, sizeof(struct fileinTag));
    }
    /* store file information */
    i = pp->file_num;
    p[i].file = (SNDFILE*) NULL;
    p[i].raw = *f;
    p[i].fd = fd;
    p[i].fullName = csound->GetFileName(fd);
    p[i].name = (char*) csound->Malloc(csound, strlen(name) + 1);
    strcpy(p[i].name, name);
    /* return with file handle */
    return i;
}

static int fout_open_sndfile(FOUT_GLOBALS *pp, SNDFILE **sf, const char *name,
                                               int write_mode, SF_INFO *sfinfo)
{
    CSOUND            *csound = pp->csound;
    struct fileinTag  *p = pp->file_opened;
    void              *fd;
    int               i, buf_reqd, do_scale = 0;

    *sf = (SNDFILE*) NULL;
    if (name == NULL || name[0] == '\0')
      return -1;
    /* is this file already open ? */
    for (i = 0; i <= pp->file_num; i++) {
      if (p[i].file != NULL && strcmp(p[i].name, name) == 0) {
        *sf = p[i].file;
        return i;
      }
    }
    buf_reqd = (int) sfinfo->channels;
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
    /* reallocate buffer if necessary */
    if ((int) sfinfo->channels > buf_reqd)
      buf_reqd = (int) sfinfo->channels;
    buf_reqd *= csound->ksmps;
    if (buf_reqd > pp->buf_size) {
      pp->buf_size = buf_reqd;
      pp->buf = (MYFLT*) csound->ReAlloc(csound, pp->buf, sizeof(MYFLT)
                                                          * buf_reqd);
    }
    /* allocate new entry */
    if (!((++pp->file_num) & 3)) {
      /* Expand by 4 each time */
      pp->file_max = pp->file_num + 4;
      p = (struct fileinTag *) csound->ReAlloc(csound, pp->file_opened,
                                                       sizeof(struct fileinTag)
                                                       * pp->file_max);
      pp->file_opened = (void*) p;
      for (i = pp->file_num; i < pp->file_max; i++)
        memset(&(p[i]), 0, sizeof(struct fileinTag));
    }
    /* store file information */
    i = pp->file_num;
    p[i].file = *sf;
    p[i].raw = (FILE*) NULL;
    p[i].fd = fd;
    p[i].fullName = csound->GetFileName(fd);
    p[i].do_scale = do_scale;
    p[i].name = (char*) csound->Malloc(csound, strlen(name) + 1);
    strcpy(p[i].name, name);
    /* return with file handle */
    return i;
}

static int outfile(CSOUND *csound, OUTFILE *p)
{
    FOUT_GLOBALS  *pp = fout_get_globals(csound, &(p->p));
    int   i, j, k;

    if (p->fp == NULL) {
      FILE *fp = ((struct fileinTag*) pp->file_opened)[p->idx].raw;
      for (k = 0; k < csound->ksmps; k++) {
        for (j = 0; j < p->nargs; j++)
          fprintf(fp, "%g ", p->argums[j][k]);
        fprintf(fp, "\n");
      }
    }
    else {
      for (j = k = 0; j < csound->ksmps; j++)
        for (i = 0; i < p->nargs; i++)
          pp->buf[k++] = p->argums[i][j] * p->scaleFac;
#ifndef USE_DOUBLE
      sf_writef_float(p->fp, (float*) pp->buf, csound->ksmps);
#else
      sf_writef_double(p->fp, (double*) pp->buf, csound->ksmps);
#endif
    }
    return OK;
}

static int outfile_set(CSOUND *csound, OUTFILE *p)
{
    FOUT_GLOBALS  *pp = fout_get_globals(csound, &(p->p));
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
      if ((p->idx = fout_open_sndfile(pp, &(p->fp), fname, 1, &sfinfo)) < 0)
        csound->Die(csound, Str("fout: cannot open outfile %s"), fname);
    }
    else { /* file handle as argument */
      n = (int) MYFLT2LRND(*p->fname);
      if (n < 0 || n > pp->file_num ||
          ((p->fp = ((struct fileinTag*) pp->file_opened)[n].file) == NULL &&
           ((struct fileinTag*) pp->file_opened)[n].raw == NULL))
        csound->Die(csound, Str("fout: invalid file handle"));
      p->idx = n;
    }
    if (((struct fileinTag*) pp->file_opened)[p->idx].do_scale)
      p->scaleFac = csound->dbfs_to_float;
    else
      p->scaleFac = FL(1.0);
    return OK;
}

static int koutfile(CSOUND *csound, KOUTFILE *p)
{
    FOUT_GLOBALS  *pp = fout_get_globals(csound, &(p->p));
    int   i;

    for (i = 0; i < p->nargs; i++)
      pp->buf[i] = p->argums[i][0] * p->scaleFac;
#ifndef USE_DOUBLE
    sf_writef_float(p->fp, (float*) pp->buf, 1);
#else
    sf_writef_double(p->fp, (double*) pp->buf, 1);
#endif
    return OK;
}

static int koutfile_set(CSOUND *csound, KOUTFILE *p)
{
    FOUT_GLOBALS  *pp = fout_get_globals(csound, &(p->p));
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
        /* according to manual, foutk always writes WAV format */
        sfinfo.format = SF_FORMAT_PCM_16 | TYPE2SF(TYP_WAV);
        break;
      default:
        sfinfo.format = SF_FORMAT_PCM_16 | SF_FORMAT_RAW;
      }
      if ((p->idx = fout_open_sndfile(pp, &(p->fp), fname, 1, &sfinfo)) < 0)
        csound->Die(csound, Str("foutk: cannot open outfile %s"), fname);
    }
    else { /* file handle argument */
      n = (int) MYFLT2LRND(*p->fname);
      if (n < 0 || n > pp->file_num ||
          (p->fp = ((struct fileinTag*) pp->file_opened)[n].file) == NULL)
        csound->Die(csound, Str("foutk: invalid file handle"));
      p->idx = n;
    }
    if (((struct fileinTag*) pp->file_opened)[p->idx].do_scale)
      p->scaleFac = csound->dbfs_to_float;
    else
      p->scaleFac = FL(1.0);
    return OK;
}

/*--------------*/

/* syntax:
        ihandle fiopen "filename" [, iascii]
*/
static int fiopen(CSOUND *csound, FIOPEN *p)
{                                       /* open a file and return its handle  */
    char    fname[FILENAME_MAX];        /* the handle is simply a stack index */
    FOUT_GLOBALS  *pp = fout_get_globals(csound, &(p->p));
    char    *omodes[] = {"w", "r", "wb", "rb"};
    FILE    *rfp;
    int     idx = (int) MYFLT2LRND(*p->iascii), n;

    strcpy(fname, (char*) p->fname);
    if (idx < 0 || idx > 3)
      idx = 0;
    if ((n = fout_open_file(pp, &rfp, fname, omodes[idx])) < 0)
      csound->Die(csound, Str("fout: cannot open outfile %s"), fname);
    if (idx > 1)
      setbuf(rfp, NULL);
    *p->ihandle = (MYFLT) n;
    return OK;
}

/* syntax:
   fouti  ihandle, iascii, iflag, iarg1 [, iarg2, ...., iargN]
*/

static int ioutfile_set(CSOUND *csound, IOUTFILE *p)
{
    FOUT_GLOBALS  *pp = fout_get_globals(csound, &(p->p));
    MYFLT   **args = p->argums;
    FILE    *rfil;
    int     j, n = (int) MYFLT2LRND(*p->ihandle);

    if (n < 0 || n > pp->file_num)
      csound->Die(csound, Str("fouti: invalid file handle"));
    rfil = ((struct fileinTag*) pp->file_opened)[n].raw;
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
        if (pp->fout_kreset == 0)
          pp->fout_kreset = csound->kcounter;
        {
          int p1 = (int) p->h.insdshead->p1;
          double p2 = (double) (csound->kcounter - pp->fout_kreset)
                      * csound->onedkr;
          double p3 = p->h.insdshead->p3;
          if (p3 > FL(0.0))
            fprintf(rfil, "i %i %f %f ", p1, p2, p3);
          else
            fprintf(rfil, "i %i %f . ", p1, p2);
        }
        break;
      case 3: /* reset */
        pp->fout_kreset = 0;
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

static int ioutfile_set_r(CSOUND *csound, IOUTFILE_R *p)
{
    FOUT_GLOBALS  *pp = fout_get_globals(csound, &(p->p));
    if (p->h.insdshead->xtratim < 1)
      p->h.insdshead->xtratim = 1;
    p->counter =  csound->kcounter;
    p->done = 1;
    if (*p->iflag == 2 && pp->fout_kreset == 0)
      pp->fout_kreset = csound->kcounter;
    return OK;
}

static int ioutfile_r(CSOUND *csound, IOUTFILE_R *p)
{
    FOUT_GLOBALS  *pp;
    MYFLT **args;
    FILE  *rfil;
    int   j, n;

    if (!p->h.insdshead->relesing || !p->done)
      return OK;

    pp = fout_get_globals(csound, &(p->p));
    args = p->argums;
    n = (int) MYFLT2LRND(*p->ihandle);
    if (n < 0 || n > pp->file_num)
      csound->Die(csound, Str("fouti: invalid file handle"));
    rfil = ((struct fileinTag*) pp->file_opened)[n].raw;
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
          double p2 = (p->counter - pp->fout_kreset) * csound->onedkr;
          double p3 = (double) (csound->kcounter - p->counter)
                      * csound->onedkr;
          fprintf(rfil, "i %i %f %f ", p1, p2, p3);
        }
        break;
      case 3: /* reset */
        pp->fout_kreset = 0;
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
    p->done = 0;

    return OK;
}

/*----------------------------------*/

static int infile_set(CSOUND *csound, INFILE *p)
{
    FOUT_GLOBALS  *pp = fout_get_globals(csound, &(p->p));
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
      if ((n = fout_open_sndfile(pp, &(p->fp), fname, 0, &sfinfo)) < 0)
        csound->Die(csound, Str("fin: cannot open infile %s"), fname);
    }
    else { /* file handle argument */
      n = (int) MYFLT2LRND(*p->fname);
      if (n < 0 || n > pp->file_num ||
          (p->fp = ((struct fileinTag*) pp->file_opened)[n].file) == NULL)
        csound->Die(csound, Str("fin: invalid file handle"));
    }
    if (((struct fileinTag*) pp->file_opened)[n].do_scale)
      p->scaleFac = csound->e0dbfs;
    else
      p->scaleFac = FL(1.0);
    p->nargs = p->INOCOUNT - 3;
    p->currpos = MYFLT2LRND(*p->iskpfrms);
    p->flag = 1;
    return OK;
}

static int infile_act(CSOUND *csound, INFILE *p)
{
    FOUT_GLOBALS  *pp = fout_get_globals(csound, &(p->p));
    int   i, j = 0, k = 0, n;

    if (p->flag) {
      sf_seek(p->fp, p->currpos, SEEK_SET);
      p->currpos += csound->ksmps;
#ifndef USE_DOUBLE
      n = (int) sf_readf_float(p->fp, (float*) pp->buf, csound->ksmps);
#else
      n = (int) sf_readf_double(p->fp, (double*) pp->buf, csound->ksmps);
#endif
      for ( ; j < n; j++)
        for (i = 0; i < p->nargs; i++)
          p->argums[i][j] = pp->buf[k++] * p->scaleFac;
      if (n >= csound->ksmps)
        return OK;
      p->flag = 0;
    }
    for ( ; j < csound->ksmps; j++)
      for (i = 0; i < p->nargs; i++)
        p->argums[i][j] = FL(0.0);
    return OK;
}

/*----------------------------*/

static int kinfile_set(CSOUND *csound, KINFILE *p)
{
    FOUT_GLOBALS  *pp = fout_get_globals(csound, &(p->p));
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
      if ((n = fout_open_sndfile(pp, &(p->fp), fname, 0, &sfinfo)) < 0)
        csound->Die(csound, Str("fink: cannot open infile %s"), fname);
    }
    else {                      /* file handle argument */
      n = (int) MYFLT2LRND(*p->fname);
      if (n < 0 || n > pp->file_num ||
          (p->fp = ((struct fileinTag*) pp->file_opened)[n].file) == NULL)
        csound->Die(csound, Str("fink: invalid file handle"));
    }
    if (((struct fileinTag*) pp->file_opened)[n].do_scale)
      p->scaleFac = csound->e0dbfs;
    else
      p->scaleFac = FL(1.0);
    p->nargs = p->INOCOUNT - 3;
    p->currpos = MYFLT2LRND(*p->iskpfrms);
    p->flag = 1;
    return OK;
}

static int kinfile(CSOUND *csound, KINFILE *p)
{
    FOUT_GLOBALS  *pp = fout_get_globals(csound, &(p->p));
    int   i, n;

    if (p->flag) {
      sf_seek(p->fp, p->currpos, SEEK_SET);
      p->currpos++;
#ifndef USE_DOUBLE
      n = (int) sf_readf_float(p->fp, (float*) pp->buf, 1);
#else
      n = (int) sf_readf_double(p->fp, (double*) pp->buf, 1);
#endif
      if (n > 0) {
        for (i = 0; i < p->nargs; i++)
          p->argums[i][0] = pp->buf[i] * p->scaleFac;
        return OK;
      }
      p->flag = 0;
    }
    for (i = 0; i < p->nargs; i++)
      p->argums[i][0] = FL(0.0);
    return OK;
}

static int i_infile(CSOUND *csound, I_INFILE *p)
{
    FOUT_GLOBALS  *pp = fout_get_globals(csound, &(p->p));
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
      if ((n = fout_open_file(pp, &fp, fname, omodes[idx])) < 0)
        csound->Die(csound, Str("fini: cannot open infile %s"), fname);
    }
    else {/* file handle argument */
      n = (int) MYFLT2LRND(*p->fname);
      if (n < 0 || n > pp->file_num ||
          (fp = ((struct fileinTag*) pp->file_opened)[n].raw) == NULL)
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

static int incr(CSOUND *csound, INCR *p)
{
    MYFLT *avar = p->avar, *aincr = p->aincr;
    int   n;

    for (n = 0; n < csound->ksmps; n++)
      avar[n] += aincr[n];
    return OK;
}

static int clear(CSOUND *csound, CLEARS *p)
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

static int fprintf_set(CSOUND *csound, FPRINTF *p)
{
    FOUT_GLOBALS  *pp = fout_get_globals(csound, &(p->p));
    int   n;
    char  *sarg = (char*) p->fmt;
    char  *sdest = p->txtstring;

    memset(p->txtstring, 0, 8192); /* Nasty to have exposed constant in code */

    if ((p->XSTRCODE & 1) || *p->fname == SSTRCOD) {
      /* if char string name given */
      char fname[FILENAME_MAX];
      csound->strarg2name(csound, fname, p->fname, "fout.", p->XSTRCODE);
      if ((p->idx = fout_open_file(pp, &(p->fp), fname, "w")) < 0)
        csound->Die(csound, Str("fprint: cannot open outfile %s"), fname);
    }
    else { /* file handle as argument */
      n = (int) MYFLT2LRND(*p->fname);
      if (n < 0 || n > pp->file_num ||
          (p->fp = ((struct fileinTag*) pp->file_opened)[n].raw) == NULL)
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

/* perform a sprintf-style format  -- matt ingalls */
static void sprints(char *outstring, char *fmt, MYFLT **kvals, long numVals)
{
    char strseg[8192];
    int i = 0, j = 0;
    char *segwaiting = 0;

    while (*fmt) {
      if (*fmt == '%') {
        /* if already a segment waiting, then lets print it */
        if (segwaiting) {
          strseg[i] = '\0';

          switch (*segwaiting) {
          case 'd':
          case 'i':
          case 'o':
          case 'x':
          case 'X':
          case 'u':
          case 'c':
            sprintf(outstring, strseg, (int) MYFLT2LRND(*kvals[j]));
            break;
          case 'h':
            sprintf(outstring, strseg, (short) MYFLT2LRND(*kvals[j]));
            break;
          case 'l':
            sprintf(outstring, strseg, (long) MYFLT2LRND(*kvals[j]));
            break;

          default:
            sprintf(outstring, strseg, *kvals[j]);
            break;
          }
          outstring += strlen(outstring);

          i = 0;
          segwaiting = 0;

          /* prevent potential problems */
          /* if user didnt give enough input params */
          if (j < numVals-1)
            j++;
        }

        /* copy the '%' */
        strseg[i++] = *fmt++;

        /* find the format code */
        segwaiting = fmt;
        while (*segwaiting && !isalpha(*segwaiting))
          segwaiting++;
      }
      else
        strseg[i++] = *fmt++;
    }

    if (i) {
      strseg[i] = '\0';
      if (segwaiting) {
        switch (*segwaiting) {
        case 'd':
        case 'i':
        case 'o':
        case 'x':
        case 'X':
        case 'u':
        case 'c':
          sprintf(outstring, strseg, (int) MYFLT2LRND(*kvals[j]));
          break;
        case 'h':
          sprintf(outstring, strseg, (short) MYFLT2LRND(*kvals[j]));
          break;
        case 'l':
          sprintf(outstring, strseg, (long) MYFLT2LRND(*kvals[j]));
          break;

        default:
          sprintf(outstring, strseg, *kvals[j]);
          break;
        }
      }
      else
        sprintf(outstring, strseg);
    }
}

static int fprintf_k(CSOUND *csound, FPRINTF *p)
{
    char        string[8192];
    sprints(string, p->txtstring, p->argums, p->INOCOUNT-2);
    fprintf(p->fp, string);
    return OK;
}

/* i-rate fprints */
static int fprintf_i(CSOUND *csound, FPRINTF *p)
{
    char        string[8192];
    fprintf_set(csound,p);
    sprints(string, p->txtstring, p->argums, p->INOCOUNT-2);
    fprintf(p->fp, string);
    return OK;
}

#define S(x)    sizeof(x)

static OENTRY localops[] = {
    { "fprints",    S(FPRINTF),     1,  "",     "TSM",
        (SUBR) fprintf_i,       (SUBR) NULL,        (SUBR) NULL         },
    { "fprintks",   S(FPRINTF),     3,  "",     "TSM",
        (SUBR) fprintf_set,     (SUBR) fprintf_k,   (SUBR) NULL         },
    { "vincr",      S(INCR),        4,  "",     "aa",
        (SUBR) NULL,            (SUBR) NULL,        (SUBR) incr         },
    { "clear",      S(CLEARS),      4,  "",     "y",
        (SUBR) NULL,            (SUBR) NULL,        (SUBR) clear        },
    { "fout",       S(OUTFILE),     5,  "",     "Tiy",
        (SUBR) outfile_set,     (SUBR) NULL,        (SUBR) outfile      },
    { "foutk",      S(KOUTFILE),    3,  "",     "Tiz",
        (SUBR) koutfile_set,    (SUBR) koutfile,    (SUBR) NULL         },
    { "fouti",      S(IOUTFILE),    1,  "",     "iiim",
        (SUBR) ioutfile_set,    (SUBR) NULL,        (SUBR) NULL         },
    { "foutir",     S(IOUTFILE_R),  3,  "",     "iiim",
        (SUBR) ioutfile_set_r,  (SUBR) ioutfile_r,  (SUBR) NULL         },
    { "fiopen",     S(FIOPEN),      1,  "i",    "Si",
        (SUBR) fiopen,          (SUBR) NULL,        (SUBR) NULL         },
    { "fin",        S(INFILE),      5,  "",     "Tiiy",
        (SUBR) infile_set,      (SUBR) NULL,        (SUBR) infile_act   },
    { "fink",       S(KINFILE),     3,  "",     "Tiiz",
        (SUBR) kinfile_set,     (SUBR) kinfile,     (SUBR) NULL         },
    { "fini",       S(I_INFILE),    1,  "",     "Tiim",
        (SUBR) i_infile,        (SUBR) NULL,        (SUBR) NULL         }
};

PUBLIC long opcode_size(void)
{
    return (long) sizeof(localops);
}

PUBLIC OENTRY *opcode_init(CSOUND *csound)
{
    FOUT_GLOBALS  *p;

    if (csound->CreateGlobalVariable(csound, "_fout_globals",
                                             sizeof(FOUT_GLOBALS)) != 0)
      csound->Die(csound, Str("fout: could not create global structure"));
    p = csound->QueryGlobalVariableNoCheck(csound, "_fout_globals");
    p->csound = csound;
    p->file_opened = (struct fileinTag*) NULL;
    p->file_max = 0;
    p->file_num = -1;
    p->fout_kreset = 0L;
    p->buf = (MYFLT*) NULL;
    p->buf_size = 0;

    return localops;
}

