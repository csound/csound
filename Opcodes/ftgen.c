/*
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
  Foundation, Inc., 31 Milk Street, #960789, Boston, MA, 02196, USA
*/

#include "stdopcod.h"
#include <ctype.h>
#include <stdarg.h>
#include "soundio.h"
#include <math.h>
#include <errno.h>

typedef struct ftgentmp_table {
  int32_t fno;
  struct ftgentmp_table *next;
} FTGENTMP_TABLE;

typedef struct {
  OPDS    h;
  MYFLT   *ifno, *p1, *p2, *p3, *p4, *p5, *argums[VARGMAX-5];
  FTGENTMP_TABLE *tables;
} FTGEN;

typedef struct {
  OPDS    h;
  MYFLT   *ifilno, *iflag, *argums[VARGMAX-2];
} FTLOAD;  /* gab 30 jul 2002 */

typedef struct {
  OPDS    h;
  MYFLT   *ifilno, *ktrig, *iflag, *argums[VARGMAX-2];
  FTLOAD  p;
} FTLOAD_K; /* gab 30 jul 2002 */

typedef struct {
  OPDS    h;
  MYFLT   *iftno, *ifreeTime;
  int32_t fno, deinit;
} FTFREE;


typedef struct namedgen {
  char    *name;
  int32_t genum;
  struct  namedgen *next;
} NAMEDGEN;

static int32_t ftable_delete(CSOUND *csound, FTGEN *p)
{
  int32_t result = OK;
  while (p->tables != NULL) {
    FTGENTMP_TABLE *table = p->tables;
    int32_t err;
    p->tables = table->next;
    err = csound->FTDelete(csound, table->fno);
    if (UNLIKELY(err != OK)) {
      csound->ErrorMsg(csound, Str("Error deleting ftable %d"), table->fno);
      result = err;
    }
    csound->Free(csound, table);
  }
  return result;
}

/* set up and call any GEN routine */
static int32_t ftgen_(CSOUND *csound, FTGEN *p, int32_t istring1, int32_t istring2)
{
  MYFLT   *fp;
  FUNC    *ftp;
  EVTBLK  *ftevt;
  int32_t     n;

  *p->ifno = FL(0.0);
  ftevt =(EVTBLK*) csound->Malloc(csound, sizeof(EVTBLK));
  n = GetInputArgCnt((OPDS *)p);
  ftevt->pcnt = (int16) n;
  n -= 5;
  ftevt->p = (MYFLT*) csound->Malloc(csound, sizeof(MYFLT)*(ftevt->pcnt+1));
  ftevt->opcod = 'f';
  ftevt->strarg = NULL;
  fp = &ftevt->p[0];
  fp[0] = FL(0.0);
  fp[1] = *p->p1;                                     /* copy p1 - p5 */
  fp[2] = ftevt->p2orig = FL(0.0);                    /* force time 0 */
  fp[3] = ftevt->p3orig = *p->p3;
  fp[4] = *p->p4;


  if (istring1) {              /* Named gen */
    NAMEDGEN *named = (NAMEDGEN*) csound->GetNamedGens(csound);
    while (named) {
      if (strcmp(named->name, ((STRINGDAT *) p->p4)->data) == 0) {
        /* Look up by name */
        fp[4] = named->genum;
        break;
      }
      named = named->next;                            /*  and round again   */
    }
    if (UNLIKELY(named == NULL)) {
      csound->Free(csound,ftevt);
      return csound->InitError(csound,
                               Str("Named gen \"%s\" not defined"),
                               (char *)p->p4);
    }
    // else fp[4] = named->genum;
  }

  if (istring2) {  /* string argument: */
    int32_t geno = (int32_t) fp[4];
    fp[5] = SSTRCOD;
    if (geno < 0)
      geno = -geno;
    switch (geno) {                      
    case 1:
    case 2:
    case 23:
    case 28:
    case 43:
    case 44:
    case 49:
      ftevt->strarg = ((STRINGDAT *) p->p5)->data;
      break;
    default:
      csound->Free(csound, ftevt);
      return csound->InitError(csound, "%s", Str("ftgen string arg not allowed"));
    }
  }
  else {
    fp[5] = *p->p5;                                   /* else no string */
  }

  if (n > 0) {
    MYFLT **argp = p->argums;
    fp += 6;
    do {
      *fp++ = **argp++;                               /* copy rem arglist */
    } while (--n);
  }
  n = csound->FTCreate(csound, &ftp, ftevt, 1);         /* call the fgen */
  csound->Free(csound,ftevt->p);
  csound->Free(csound, ftevt);
  if (UNLIKELY(n != 0))
    return csound->InitError(csound, "%s", Str("ftgen error"));
  if (ftp != NULL)
    *p->ifno = (MYFLT) ftp->fno;                      /* record the fno */
  return OK;
}

static int32_t ftgen(CSOUND *csound, FTGEN *p) {
  return ftgen_(csound,p,0,0);
}

static int32_t ftgen_S(CSOUND *csound, FTGEN *p) {
  return ftgen_(csound,p,1,0);
}

static int32_t ftgen_iS(CSOUND *csound, FTGEN *p) {
  return ftgen_(csound,p,0,1);
}

static int32_t ftgen_SS(CSOUND *csound, FTGEN *p) {
  return ftgen_(csound,p,1,1);
}

static int32_t ftgentmp_(CSOUND *csound, FTGEN *p,
                          int32_t istring1, int32_t istring2)
{
  if (UNLIKELY(ftgen_(csound, p, istring1, istring2) != OK))
    return NOTOK;
  if (MYFLT2LRND(*p->p1) == 0 && *p->ifno > FL(0.0)) {
    FTGENTMP_TABLE *table = csound->Malloc(csound, sizeof(FTGENTMP_TABLE));
    table->fno = (int32_t) MYFLT2LRND(*p->ifno);
    /* Reinit can create more tables; retain each until the note ends. */
    table->next = p->tables;
    p->tables = table;
  }
  return OK;
}

static int32_t ftgentmp(CSOUND *csound, FTGEN *p)
{
  return ftgentmp_(csound, p, 0, 0);
}

static int32_t ftgentmp_S(CSOUND *csound, FTGEN *p)
{
  return ftgentmp_(csound, p, 0, 1);
}

static int32_t ftgentmp_Si(CSOUND *csound, FTGEN *p)
{
  return ftgentmp_(csound, p, 1, 0);
}

static int32_t ftgentmp_SS(CSOUND *csound, FTGEN *p)
{
  return ftgentmp_(csound, p, 1, 1);
}

static int32_t ftfree_deinit(CSOUND *csound, FTFREE *p)
{
  if(p->deinit) {
    int32_t err = csound->FTDelete(csound, p->fno);
    if (UNLIKELY(err != OK))
      csound->ErrorMsg(csound, Str("Error deleting ftable %d"),
                       p->fno);
    return err;
  } else return OK;
}
static int32_t ftfree(CSOUND *csound, FTFREE *p)
{
  p->fno = (int32_t) MYFLT2LRND(*p->iftno);

  if (UNLIKELY(p->fno <= 0))
    return csound->InitError(csound, Str("Invalid table number: %d"), p->fno);
  if (*p->ifreeTime == FL(0.0)) {
    p->deinit = 0;
    if (UNLIKELY(csound->FTDelete(csound, p->fno) != 0))
      return csound->InitError(csound, Str("Error deleting ftable %d"), p->fno);
    return OK;
  }
  p->deinit = 1;
  return OK;
}

static int32_t myInitError(CSOUND *csound, OPDS *p, const char *str, ...)
{
  char message[1024];
  va_list args;
  IGN((OPDS *)p);
  va_start(args, str);
  vsnprintf(message, sizeof(message), str, args);
  va_end(args);
  return csound->InitError(csound, "%s", message);
}

static void ftload_copy_header(CSOUND *csound, FUNC *ftp,
                               const FUNC *header, size_t size)
{
  MYFLT *ftable = ftp->ftable;
  csound->Free(csound, ftp->args);
  memcpy(ftp, header, size);
  ftp->ftable = ftable;
  ftp->args = NULL;
  ftp->argcnt = 0;
}

static int32_t ftload_text_end(const char *text)
{
  while (isspace((unsigned char)*text)) text++;
  return *text == '\0';
}

static char *ftload_text_field(FILE *file, char *line, size_t size,
                               const char *name)
{
  size_t length = strlen(name);
  if (fgets(line, size, file) == NULL ||
      strncmp(line, name, length) != 0 || line[length] != ':' ||
      (strchr(line, '\n') == NULL && !feof(file)))
    return NULL;
  return line + length + 1;
}

static int32_t ftload_(CSOUND *csound, FTLOAD *p, int32_t istring)
{
  MYFLT **argp = p->argums;
  FUNC  *ftp;
  char  filename[MAXNAME];
  int32_t   nargs = GetInputArgCnt((OPDS *)p) - 2;
  FILE  *file = NULL;
  int32_t   (*err_func)(CSOUND *, OPDS *, const char *, ...);
  FUNC  *(*ft_func)(CSOUND *, MYFLT *);
  void  *fd;

  if (strncmp(GetOpcodeName((OPDS *)p), "ftloadk", 7) == 0) {
    nargs--;
    ft_func = csound->FTFind;
    err_func = csound->PerfError;
  }
  else {
    ft_func = csound->FTFind;
    err_func = myInitError;
  }

  if (UNLIKELY(nargs <= 0))
    goto err2;

  if (!istring) {
    if (IsStringCode(*p->ifilno))
      csound->StringArg2Name(csound, filename, p->ifilno, "ftsave.", 0);
    else strncpy(filename, csound->GetArgString(csound,*p->ifilno), MAXNAME);
  } else {
    strncpy(filename, ((STRINGDAT *)p->ifilno)->data, MAXNAME);
  }

  if (*p->iflag <= FL(0.0)) {
    fd = csound->FileOpen(csound, &file, CSFILE_STD, filename, "rb",
                          "", CSFTYPE_FTABLES_BINARY, 0);
    if (UNLIKELY(fd == NULL)) goto err3;
    while (nargs--) {
      FUNC  header;
      int32_t   fno = (int32_t) MYFLT2LRND(**argp);
      MYFLT fno_f = (MYFLT) fno;
      size_t   n;

      memset(&header, 0, sizeof(FUNC));
      /* ***** Need to do byte order here ***** */
      n = fread(&header, sizeof(FUNC) - sizeof(MYFLT) - SSTRSIZ, 1, file);
      if (UNLIKELY(n!=1)) goto err4;
      header.fno = (int32) fno;
      if (UNLIKELY(csound->FTAlloc(csound, fno, (int32_t) header.flen) != 0))
        goto err;
      ftp = ft_func(csound, &fno_f);
      // Do we need to check value of ftp->fflen? #27323
      if (ftp->flen > 0x40000000)
        return csound->InitError(csound,"%s", Str("table length too long"));
      ftload_copy_header(csound, ftp, &header,
                         sizeof(FUNC) - sizeof(MYFLT*) - SSTRSIZ);
      memset(ftp->ftable, 0, sizeof(MYFLT) * ((uint64_t) ftp->flen + 1));
      n = fread(ftp->ftable, sizeof(MYFLT), ftp->flen + 1l, file);
      if (UNLIKELY(n!=ftp->flen + 1)) goto err4;
      /* ***** Need to do byte order here ***** */
      argp++;
    }
  }
  else {
    fd = csound->FileOpen(csound, &file, CSFILE_STD, filename, "r",
                          "", CSFTYPE_FTABLES_TEXT, 0);
    if (UNLIKELY(fd == NULL)) goto err3;
    while (nargs--) {
      FUNC  header;
      char  s[64], *s1;
      int32_t   fno = (int32_t) MYFLT2LRND(**argp);
      MYFLT fno_f = (MYFLT) fno;
      uint32_t  j;
      char *endptr;

      memset(&header, 0, sizeof(FUNC));
      /* IMPORTANT!! If FUNC structure and/or GEN01ARGS structure
         will be modified, the following code has to be modified too */
      if (UNLIKELY(NULL == fgets(s, sizeof(s), file))) goto err4;
#define FTLOAD_READ_INT(NAME, FIELD, MINIMUM, MAXIMUM) do {           \
        long long value;                                             \
        s1 = ftload_text_field(file, s, sizeof(s), NAME);              \
        if (UNLIKELY(s1 == NULL)) goto err4;                           \
        errno = 0;                                                   \
        value = strtoll(s1, &endptr, 10);                             \
        if (UNLIKELY(errno == ERANGE || endptr == s1 ||               \
                     !ftload_text_end(endptr) ||                      \
                     value < (MINIMUM) || value > (MAXIMUM)))         \
          goto err4;                                                 \
        header.FIELD = value;                                        \
      } while (0)
#define FTLOAD_READ_FLOAT(FIELD) do {                                 \
        s1 = ftload_text_field(file, s, sizeof(s), #FIELD);            \
        if (UNLIKELY(s1 == NULL)) goto err4;                           \
        header.FIELD = (MYFLT)csound->Strtod(s1, &endptr);              \
        if (UNLIKELY(endptr == s1 || !ftload_text_end(endptr)))         \
          goto err4;                                                 \
      } while (0)

      FTLOAD_READ_INT("flen", flen, 1, MAXLEN);
      FTLOAD_READ_INT("lenmask", lenmask, INT32_MIN, INT32_MAX);
      FTLOAD_READ_INT("lobits", lobits, 0, 31);
      FTLOAD_READ_INT("lomask", lomask, INT32_MIN, INT32_MAX);
      FTLOAD_READ_FLOAT(lodiv);
      FTLOAD_READ_FLOAT(cvtbas);
      FTLOAD_READ_FLOAT(cpscvt);
      FTLOAD_READ_INT("loopmode1", loopmode1, INT16_MIN, INT16_MAX);
      FTLOAD_READ_INT("loopmode2", loopmode2, INT16_MIN, INT16_MAX);
      FTLOAD_READ_INT("begin1", begin1, INT32_MIN, INT32_MAX);
      FTLOAD_READ_INT("end1", end1, INT32_MIN, INT32_MAX);
      FTLOAD_READ_INT("begin2", begin2, INT32_MIN, INT32_MAX);
      FTLOAD_READ_INT("end2", end2, INT32_MIN, INT32_MAX);
      FTLOAD_READ_INT("soundend", soundend, INT32_MIN, INT32_MAX);
      FTLOAD_READ_INT("flenfrms", flenfrms, INT32_MIN, INT32_MAX);
      FTLOAD_READ_INT("nchnls", nchanls, INT32_MIN, INT32_MAX);
      FTLOAD_READ_INT("fno", fno, INT32_MIN, INT32_MAX);
      FTLOAD_READ_FLOAT(gen01args.gen01);
      FTLOAD_READ_FLOAT(gen01args.ifilno);
      FTLOAD_READ_FLOAT(gen01args.iskptim);
      FTLOAD_READ_FLOAT(gen01args.iformat);
      FTLOAD_READ_FLOAT(gen01args.channel);
      FTLOAD_READ_FLOAT(gen01args.sample_rate);
#undef FTLOAD_READ_INT
#undef FTLOAD_READ_FLOAT
      if (UNLIKELY(NULL == fgets(s, sizeof(s), file))) goto err4;
      header.fno = fno;
      ftp = ft_func(csound, &fno_f);
      if (UNLIKELY(ftp == NULL)) goto err;
      if (ftp->flen < header.flen) {
        if (UNLIKELY(csound->FTAlloc(csound, fno, (int32_t)header.flen) != 0))
          goto err;
        ftp = ft_func(csound, &fno_f);
      }
      if (UNLIKELY(ftp == NULL || ftp->ftable == NULL)) goto err;
      ftload_copy_header(csound, ftp, &header, sizeof(FUNC) - sizeof(MYFLT));
      memset(ftp->ftable, 0, sizeof(MYFLT) * ((size_t)ftp->flen + 1));

      for (j = 0; j <= ftp->flen; j++) {
        if (UNLIKELY(NULL == fgets(s, sizeof(s), file))) goto err4;
        ftp->ftable[j] = (MYFLT)csound->Strtod(s, &endptr);
        if (UNLIKELY(endptr == s || !ftload_text_end(endptr))) goto err4;
      }
      if (UNLIKELY(NULL == fgets(s, sizeof(s), file))) goto err4;
      argp++;
    }
  }
  csound->FileClose(csound, fd, CSFILE_CLOSE_SYNC);
  return OK;
 err:
  csound->FileClose(csound, fd, CSFILE_CLOSE_SYNC);
  return err_func(csound, &(p->h),
                  "%s", Str("ftload: error allocating ftable"));
 err2:
  return err_func(csound, &(p->h), "%s", Str("ftload: no table numbers"));
 err3:
  return err_func(csound, &(p->h), "%s", Str("ftload: unable to open file"));
 err4:
  csound->FileClose(csound, fd, CSFILE_CLOSE_SYNC);
  return err_func(csound, &(p->h), "%s", Str("ftload: incorrect file"));
}

static int32_t ftload(CSOUND *csound, FTLOAD *p)
{
  return ftload_(csound, p, 0);
}

static int32_t ftload_S(CSOUND *csound, FTLOAD *p)
{
  return ftload_(csound, p, 1);
}


static int32_t ftload_k(CSOUND *csound, FTLOAD_K *p)
{
  if (*p->ktrig != FL(0.0))
    return ftload_(csound, &(p->p),0);
  return OK;
}

static int32_t ftload_kS(CSOUND *csound, FTLOAD_K *p)
{
  if (*p->ktrig != FL(0.0))
    return ftload_(csound, &(p->p), 1);
  return OK;
}

static int32_t ftsave_(CSOUND *csound, FTLOAD *p, int32_t istring)
{
  MYFLT **argp = p->argums;
  char  filename[MAXNAME];
  int32_t   nargs = GetInputArgCnt((OPDS *)p) - 3;
  FILE  *file = NULL;
  int32_t   (*err_func)(CSOUND *, OPDS *, const char *, ...);
  void  *fd;

  if (strncmp(GetOpcodeName((OPDS *)p), "ftsave.", 7) != 0) {
    err_func = csound->PerfError;
  }
  else {
    nargs = GetInputArgCnt((OPDS *)p) - 2;
    err_func = myInitError;
  }

  if (UNLIKELY(nargs <= 0))
    goto err2;

  if (!istring) {
    if (IsStringCode(*p->ifilno))
      csound->StringArg2Name(csound, filename, p->ifilno, "ftsave.", 0);
    else strncpy(filename, csound->GetArgString(csound,*p->ifilno), MAXNAME);
  } else {
    strncpy(filename, ((STRINGDAT *)p->ifilno)->data, MAXNAME);
  }

  if (*p->iflag <= FL(0.0)) {
    fd = csound->FileOpen(csound, &file, CSFILE_STD, filename, "wb",
                          "", CSFTYPE_FTABLES_BINARY, 0);
    if (UNLIKELY(fd == NULL)) goto err3;
    while (nargs--) {
      FUNC *ftp;
      //csound->Message(csound, "saving table %f \n", **argp);
      if ( *argp && (ftp = csound->FTFind(csound, *argp)) != NULL) {
        FUNC header = *ftp;
        MYFLT *table = ftp->ftable;
        int32 flen = ftp->flen;
        int32_t n;
        header.args = NULL;
        header.argcnt = 0;
        header.ftable = NULL;
        n =  (int32_t) fwrite(&header, sizeof(FUNC) - sizeof(MYFLT) - SSTRSIZ, 1, file);
        if (UNLIKELY(n!=1)) goto err4;
        n =  (int32_t) fwrite(table, sizeof(MYFLT), flen + 1, file);
        if (UNLIKELY(n!=flen + 1)) goto err4;
      }
      else goto err;
      argp++;
    }
  }
  else {
    fd = csound->FileOpen(csound, &file, CSFILE_STD, filename, "w",
                          "", CSFTYPE_FTABLES_TEXT, 0);
    if (UNLIKELY(fd == NULL)) goto err3;
    while (nargs--) {
      FUNC *ftp;

      if ((ftp = csound->FTFind(csound, *argp)) != NULL) {
        int32 flen = ftp->flen;
        int32 j;
        MYFLT *table = ftp->ftable;
        /* IMPORTANT!! If FUNC structure and/or GEN01ARGS structure
           will be modified, the following code has to be modified too */
        fprintf(file,"======= TABLE %d size: %d values ======\n",
                ftp->fno, ftp->flen);
        fprintf(file,"flen: %d\n", ftp->flen);
        fprintf(file,"lenmask: %d\n", ftp->lenmask);
        fprintf(file,"lobits: %d\n",ftp->lobits);
        fprintf(file,"lomask: %d\n",ftp->lomask);
        /* Keep enough digits to round-trip double-precision MYFLT values. */
        fprintf(file,"lodiv: %.17g\n",ftp->lodiv);
        fprintf(file,"cvtbas: %.17g\n",ftp->cvtbas);
        fprintf(file,"cpscvt: %.17g\n",ftp->cpscvt);
        fprintf(file,"loopmode1: %d\n", (int32_t) ftp->loopmode1);
        fprintf(file,"loopmode2: %d\n", (int32_t) ftp->loopmode2);
        fprintf(file,"begin1: %d\n",ftp->begin1);
        fprintf(file,"end1: %d\n",ftp->end1);
        fprintf(file,"begin2: %d\n",ftp->begin2);
        fprintf(file,"end2: %d\n",ftp->end2);
        fprintf(file,"soundend: %d\n",ftp->soundend);
        fprintf(file,"flenfrms: %d\n",ftp->flenfrms);
        fprintf(file,"nchnls: %d\n",ftp->nchanls);
        fprintf(file,"fno: %d\n",ftp->fno);

        fprintf(file,"gen01args.gen01: %.17g\n",ftp->gen01args.gen01);
        fprintf(file,"gen01args.ifilno: %.17g\n",ftp->gen01args.ifilno);
        fprintf(file,"gen01args.iskptim: %.17g\n",ftp->gen01args.iskptim);
        fprintf(file,"gen01args.iformat: %.17g\n",ftp->gen01args.iformat);
        fprintf(file,"gen01args.channel: %.17g\n",ftp->gen01args.channel);
        fprintf(file,"gen01args.sample_rate: %.17g\n",
                ftp->gen01args.sample_rate);
        /* WARNING! skips ftp->gen01args.strarg from saving/loading in
           text format */
        fprintf(file,"---------END OF HEADER--------------\n");

        for (j = 0; j <= flen; j++) {
          MYFLT val = table[j];
          fprintf(file,"%.17g\n",val);
        }
        fprintf(file,"---------END OF TABLE---------------\n");
      }
      else goto err;
      argp++;
    }
  }
  csound->FileClose(csound, fd, CSFILE_CLOSE_SYNC);
  return OK;
 err:
  csound->FileClose(csound, fd, CSFILE_CLOSE_SYNC);
  return err_func(csound, &(p->h),
                  "%s", Str("ftsave: Bad table number. Saving is possible "
                            "only for existing tables."));
 err2:
  return err_func(csound, &(p->h), "%s", Str("ftsave: no table numbers"));
 err3:
  return err_func(csound, &(p->h), "%s", Str("ftsave: unable to open file"));
 err4:
  csound->FileClose(csound, fd, CSFILE_CLOSE_SYNC);
  return err_func(csound, &(p->h), "%s", Str("ftsave: failed to write file"));
}

static int32_t ftsave(CSOUND *csound, FTLOAD *p){
  return ftsave_(csound,p,0);
}

static int32_t ftsave_S(CSOUND *csound, FTLOAD *p){
  return ftsave_(csound,p,1);
}


static int32_t ftsave_k_set(CSOUND *csound, FTLOAD_K *p)
{
  memcpy(&(p->p.h), &(p->h), sizeof(OPDS));
  p->p.ifilno = p->ifilno;
  p->p.iflag = p->iflag;
  memcpy(p->p.argums, p->argums,
         sizeof(MYFLT*) * (GetInputArgCnt((OPDS *)p) - 3));
  return OK;
}

static int32_t ftsave_k(CSOUND *csound, FTLOAD_K *p)
{
  if (*p->ktrig != FL(0.0))
    return ftsave_(csound, &(p->p), 0);
  return OK;
}

static int32_t ftsave_kS(CSOUND *csound, FTLOAD_K *p)
{
  if (*p->ktrig != FL(0.0))
    return ftsave_(csound, &(p->p), 1);
  return OK;
}

static int32_t ftgen_list(CSOUND *csound, FTGEN *p, int32_t istring)
{
  MYFLT   *fp;
  FUNC    *ftp;
  EVTBLK  *ftevt;
  int32_t     n;
  ARRAYDAT *array = (ARRAYDAT*) (p->p5);

  *p->ifno = FL(0.0);
  ftevt =(EVTBLK*) csound->Malloc(csound, sizeof(EVTBLK));
  n = array->sizes[0];
  ftevt->pcnt =  n+4;
  ftevt->p = (MYFLT*) csound->Malloc(csound, sizeof(MYFLT)*(ftevt->pcnt+1));  
  ftevt->opcod = 'f';
  ftevt->strarg = NULL;
  fp = &ftevt->p[0];
  fp[0] = FL(0.0);
  fp[1] = *p->p1;                                     /* copy p1 - p5 */
  fp[2] = ftevt->p2orig = FL(0.0);                    /* force time 0 */
  fp[3] = ftevt->p3orig = *p->p3;
  if(fp[3] == 0) fp[3] = ftevt->p3orig = n;
  fp[4] = *p->p4;


  if (istring) {              /* Named gen */
    NAMEDGEN *named = (NAMEDGEN*) csound->GetNamedGens(csound);
    while (named) {
      if (strcmp(named->name, ((STRINGDAT *) p->p4)->data) == 0) {
        /* Look up by name */
        fp[4] = named->genum;
        break;
      }
      named = named->next;                            /*  and round again   */
    }
    if (UNLIKELY(named == NULL)) {
      csound->Free(csound,ftevt);
      return csound->InitError(csound,
                               Str("Named gen \"%s\" not defined"),
                               (char *)p->p4);
    }
  }

  memcpy(&fp[5], array->data, n*sizeof(MYFLT));
  n = csound->FTCreate(csound, &ftp, ftevt, 1);         /* call the fgen */
  csound->Free(csound,ftevt->p);
  csound->Free(csound,ftevt);
  if (UNLIKELY(n != 0))
    return csound->InitError(csound, "%s", Str("ftgen error"));
  if (ftp != NULL)
    *p->ifno = (MYFLT) ftp->fno;                      /* record the fno */
  return OK;
}

static int32_t ftgen_list_S(CSOUND *csound, FTGEN *p){
  return ftgen_list(csound,p,1);
}

static int32_t ftgen_list_i(CSOUND *csound, FTGEN *p){
  return ftgen_list(csound,p,0);
}

/*
  gtftargs (c) 2016 Guillermo Senna.
*/

typedef struct {
  OPDS      h;
  STRINGDAT *Scd;
  MYFLT     *ftable;
  MYFLT     *ktrig;
  MYFLT     prv_ktrig;
  int32_t       status;
} FTARGS;

static int32_t getftargs(CSOUND *, FTARGS *);

/*
  Inspiration for the implementation of this Opcode was taken
  from the following Csound Opcodes: "sprintf", "puts" and "pwd".
  Credit for that goes to their respective authors.
*/

static int32_t getftargs_init(CSOUND *csound, FTARGS *p)
{
  p->status = OK;
  if (*p->ktrig > FL(0.0))
    p->status = getftargs(csound, p);
  p->prv_ktrig = *p->ktrig;

  return p->status;
}

static int32_t getftargs_process(CSOUND *csound, FTARGS *p)
{
  if (*p->ktrig != p->prv_ktrig && *p->ktrig > FL(0.0)) {
    p->status = getftargs(csound, p);
  }
  p->prv_ktrig = *p->ktrig;

  return p->status;

}


static int32_t getftargs(CSOUND *csound, FTARGS *p)
{
  FUNC *src;
  int32 argcnt, i, strlen = 0;

  if (UNLIKELY((src = csound->FTFind(csound, p->ftable)) == NULL)) {
    return csound->PerfError(csound, &(p->h),
                             Str("table: could not find ftable %d"),
                             (int32_t) *p->ftable);
  }

  argcnt = src->argcnt;

  if (argcnt <= 1 || src->args == NULL) {
    p->Scd->size = 1;
    if (p->Scd->data == NULL) {
      p->Scd->data = (char*) csound->Calloc(csound, 1);
    }
    else {
      p->Scd->data = (char*) csound->ReAlloc(csound, p->Scd->data, 1);
    }
    p->Scd->data[0] = '\0';
    return OK;
  }

  for (i = 1; i < argcnt; i++)
    strlen += snprintf(NULL, 0, "%g ", src->args[i]);

  p->Scd->size = strlen;

  if (p->Scd->data == NULL) {
    p->Scd->data = (char*) csound->Calloc(csound, strlen);
  }
  else
    p->Scd->data = (char*) csound->ReAlloc(csound, p->Scd->data, strlen);

  {
    char* curr = p->Scd->data, *const end = curr + strlen;
    for (i = 1; curr != end && i < argcnt; i++) {
      curr += snprintf(curr, end-curr, "%g ", src->args[i]);
    }
  }

  return OK;
}




#define S(x)    sizeof(x)

static OENTRY localops[] = {
  { "ftgen",    S(FTGEN),     TW,  "i",  "iiiiim", (SUBR) ftgen, NULL, NULL    },
  { "ftgen.S",    S(FTGEN),   TW,  "i",  "iiiSim", (SUBR) ftgen_S, NULL, NULL  },
  { "ftgen.iS",    S(FTGEN),  TW,  "i",  "iiiiSm", (SUBR) ftgen_iS, NULL, NULL },
  { "ftgen.SS",    S(FTGEN),  TW,  "i",  "iiiSSm", (SUBR) ftgen_SS, NULL, NULL },
  { "ftgen",    S(FTGEN),     TW,  "i",  "iiiii[]", (SUBR) ftgen_list_i, NULL  },
  { "ftgen",    S(FTGEN),     TW,  "i",  "iiiSi[]", (SUBR) ftgen_list_S, NULL  },
  { "ftgentmp.i", S(FTGEN),   TW,  "i",  "iiiiim", (SUBR) ftgentmp, NULL,
    (SUBR) ftable_delete },
  { "ftgentmp.iS", S(FTGEN),  TW,  "i",  "iiiiSm", (SUBR) ftgentmp_S, NULL,
    (SUBR) ftable_delete},
  { "ftgentmp.Si", S(FTGEN),  TW,  "i",  "iiiSim", (SUBR) ftgentmp_Si,NULL,
    (SUBR) ftable_delete},
  { "ftgentmp.SS", S(FTGEN),  TW,  "i",  "iiiSSm", (SUBR) ftgentmp_SS,NULL,
    (SUBR) ftable_delete},
  { "ftfree",   S(FTFREE),    TW,  "",   "ii",     (SUBR) ftfree, NULL,
    (SUBR) ftfree_deinit},
  { "ftsave",   S(FTLOAD),    TR,  "",   "iim",    (SUBR) ftsave, NULL, NULL   },
  { "ftsave.S",   S(FTLOAD),  TR,  "",   "Sim",    (SUBR) ftsave_S, NULL, NULL },
  { "ftload",   S(FTLOAD),    TW,  "",   "iim",    (SUBR) ftload, NULL, NULL   },
  { "ftload.S",  S(FTLOAD), TW,  "",   "Sim",    (SUBR) ftload_S, NULL, NULL },
  { "ftsavek",  S(FTLOAD_K),  TW,   "",   "ikim",   (SUBR) ftsave_k_set,
    (SUBR) ftsave_k, NULL       },
  { "ftsavek.S",  S(FTLOAD_K),  TW,   "",   "Skim",   (SUBR) ftsave_k_set,
    (SUBR) ftsave_kS, NULL       },
  { "ftloadk",  S(FTLOAD_K),  TW,   "",   "ikim",   (SUBR) ftsave_k_set,
    (SUBR) ftload_k, NULL       },
  { "ftloadk.S",  S(FTLOAD_K),  TW,   "",   "Skim",   (SUBR) ftsave_k_set,
    (SUBR) ftload_kS, NULL       },
  { "getftargs",   sizeof(FTARGS),  0,  "S", "ik",
    (SUBR)getftargs_init, (SUBR)getftargs_process }

};

int32_t ftgen_init_(CSOUND *csound)
{
  return csound->AppendOpcodes(csound, &(localops[0]),
                               (int32_t
                                ) (sizeof(localops) / sizeof(OENTRY)));
}
