/*
    Csound is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with Csound; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
    02110-1301 USA
*/

#include "stdopcod.h"
#include <ctype.h>
#include <stdarg.h>
#include "soundio.h"
#include <math.h>

typedef struct {
    OPDS    h;
    MYFLT   *ifno, *p1, *p2, *p3, *p4, *p5, *argums[VARGMAX-5];
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
} FTFREE;

typedef struct {
    OPDS    h;
    int32_t fno;
} FTDELETE;

typedef struct namedgen {
    char    *name;
    int32_t genum;
    struct  namedgen *next;
} NAMEDGEN;

static int32_t ftable_delete(CSOUND *csound, void *p)
{
    int32_t err = csound->FTDelete(csound, ((FTDELETE*) p)->fno);
    if (UNLIKELY(err != OK))
      csound->ErrorMsg(csound, Str("Error deleting ftable %d"),
                               ((FTDELETE*) p)->fno);
    csound->Free(csound, p);
    return err;
}

static int32_t register_ftable_delete(CSOUND *csound, void *p, int32_t tableNum)
{
  FTDELETE  *op = (FTDELETE*) csound->Calloc(csound, sizeof(FTDELETE));
    if (UNLIKELY(op == NULL))
      return csound->InitError(csound, Str("memory allocation failure"));
    op->h.insdshead = ((OPDS*) p)->insdshead;
    op->fno = tableNum;
    return csound->RegisterDeinitCallback(csound, op, ftable_delete);
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
      n = (int32_t) fp[4];
      fp[5] = SSTRCOD;
      if (n < 0)
        n = -n;
      switch (n) {                      /*   must be Gen01, 23, 28, 43, 49 */
      case 1:
      case 23:
      case 28:
      case 43:
      case 49:
        ftevt->strarg = ((STRINGDAT *) p->p5)->data;
        break;
      default:
        csound->Free(csound, ftevt);
        return csound->InitError(csound, Str("ftgen string arg not allowed"));
      }
    }
    else {
      fp[5] = *p->p5;                                   /* else no string */
    }
    n = csound->GetInputArgCnt(p);
    ftevt->pcnt = (int16) n;
    n -= 5;
    if (n > 0) {
      MYFLT **argp = p->argums;
      fp += 6;
      do {
        *fp++ = **argp++;                               /* copy rem arglist */
      } while (--n);
    }
    n = csound->hfgens(csound, &ftp, ftevt, 1);         /* call the fgen */
    csound->Free(csound, ftevt);
    if (UNLIKELY(n != 0))
      return csound->InitError(csound, Str("ftgen error"));
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

static int32_t ftgentmp(CSOUND *csound, FTGEN *p)
{
    int32_t   p1, fno;

    if (UNLIKELY(ftgen(csound, p) != OK))
      return NOTOK;
    p1 = (int32_t) MYFLT2LRND(*p->p1);
    if (p1)
      return OK;
    fno = (int32_t) MYFLT2LRND(*p->ifno);
    return register_ftable_delete(csound, p, fno);
}


static int32_t ftgentmp_S(CSOUND *csound, FTGEN *p)
{
    int32_t   p1, fno;

    if (UNLIKELY(ftgen_(csound, p,0,1) != OK))
      return NOTOK;
    p1 = (int32_t) MYFLT2LRND(*p->p1);
    if (p1)
      return OK;
    fno = (int32_t) MYFLT2LRND(*p->ifno);
    return register_ftable_delete(csound, p, fno);
}

static int32_t ftgentmp_Si(CSOUND *csound, FTGEN *p)
{
    int32_t   p1, fno;

    if (UNLIKELY(ftgen_(csound, p,1,0) != OK))
      return NOTOK;
    p1 = (int32_t) MYFLT2LRND(*p->p1);
    if (p1)
      return OK;
    fno = (int32_t) MYFLT2LRND(*p->ifno);
    return register_ftable_delete(csound, p, fno);
}

static int32_t ftgentmp_SS(CSOUND *csound, FTGEN *p)
{
    int32_t   p1, fno;

    if (UNLIKELY(ftgen_(csound, p,1,1) != OK))
      return NOTOK;
    p1 = (int32_t) MYFLT2LRND(*p->p1);
    if (p1)
      return OK;
    fno = (int32_t) MYFLT2LRND(*p->ifno);
    return register_ftable_delete(csound, p, fno);
}

static int32_t ftfree(CSOUND *csound, FTFREE *p)
{
    int32_t fno = (int32_t) MYFLT2LRND(*p->iftno);

    if (UNLIKELY(fno <= 0))
      return csound->InitError(csound, Str("Invalid table number: %d"), fno);
    if (*p->ifreeTime == FL(0.0)) {
      if (UNLIKELY(csound->FTDelete(csound, fno) != 0))
        return csound->InitError(csound, Str("Error deleting ftable %d"), fno);
      return OK;
    }
    return register_ftable_delete(csound, p, fno);
}

static int32_t myInitError(CSOUND *csound, OPDS *p, const char *str, ...)
{
    IGN(p);
    return csound->InitError(csound, "%s",str);
}

static int32_t ftload_(CSOUND *csound, FTLOAD *p, int32_t istring)
{
    MYFLT **argp = p->argums;
    FUNC  *ftp;
    char  filename[MAXNAME];
    int32_t   nargs = csound->GetInputArgCnt(p) - 2;
    FILE  *file = NULL;
    int32_t   (*err_func)(CSOUND *, OPDS *, const char *, ...);
    FUNC  *(*ft_func)(CSOUND *, MYFLT *);
    void  *fd;

    if (strncmp(csound->GetOpcodeName(p), "ftloadk", 7) == 0) {
      nargs--;
      ft_func = csound->FTFindP;
      err_func = csound->PerfError;
    }
    else {
      ft_func = csound->FTnp2Finde;
      err_func = myInitError;
    }

    if (UNLIKELY(nargs <= 0))
      goto err2;

    if (!istring) {
      if (csound->ISSTRCOD(*p->ifilno))
        csound->strarg2name(csound, filename, p->ifilno, "ftsave.", 0);
      else strNcpy(filename, get_arg_string(csound,*p->ifilno), MAXNAME);
    } else {
      strNcpy(filename, ((STRINGDAT *)p->ifilno)->data, MAXNAME);
    }

    if (*p->iflag <= FL(0.0)) {
      fd = csound->FileOpen2(csound, &file, CSFILE_STD, filename, "rb",
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
          return csound->InitError(csound,Str("table length too long"));
        memcpy(ftp, &header, sizeof(FUNC) - sizeof(MYFLT*) - SSTRSIZ);
        memset(ftp->ftable, 0, sizeof(MYFLT) * ((uint64_t) ftp->flen + 1));
        n = fread(ftp->ftable, sizeof(MYFLT), ftp->flen + 1l, file);
        if (UNLIKELY(n!=ftp->flen + 1)) goto err4;
        /* ***** Need to do byte order here ***** */
        argp++;
      }
    }
    else {
      fd = csound->FileOpen2(csound, &file, CSFILE_STD, filename, "r",
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
        if (UNLIKELY(NULL==fgets(s, 64, file))) goto err4;
        if (UNLIKELY(NULL==fgets(s, 64, file))) goto err4;
        s1 = strchr(s, ' ')+1;
        header.flen = strtol(s1, &endptr, 10);
        if (UNLIKELY(endptr==NULL)) goto err4;
        if (UNLIKELY(NULL==fgets(s, 64, file))){ goto err4; }
        s1 = strchr(s, ' ')+1;
        header.lenmask = strtol(s1, &endptr, 10);
        if (UNLIKELY(endptr==NULL)) goto err4;
        if (UNLIKELY(NULL==fgets(s, 64, file))){ goto err4; }
        s1 = strchr(s, ' ')+1;
        header.lobits = strtol(s1, &endptr, 10);
        if (UNLIKELY(endptr==NULL)) goto err4;
        if (UNLIKELY(NULL==fgets(s, 64, file))){ goto err4;}
        s1 = strchr(s, ' ')+1;
        header.lomask = strtol(s1, &endptr, 10);
        if (UNLIKELY(endptr==NULL)) goto err4;
        if (UNLIKELY(NULL==fgets(s, 64, file))){ goto err4;}
        s1 = strchr(s, ' ')+1;
        header.lodiv = (MYFLT)strtol(s1, &endptr, 10);
        if (UNLIKELY(endptr==NULL)) goto err4;
        if (UNLIKELY(NULL==fgets(s, 64, file))) {goto err4;}
        s1 = strchr(s, ' ')+1;
        header.cvtbas = (MYFLT)strtol(s1, &endptr, 10);
        if (UNLIKELY(endptr==NULL)) goto err4;
        if (UNLIKELY(NULL==fgets(s, 64, file))){ goto err4;}
        s1 = strchr(s, ' ')+1;
        header.cpscvt = (MYFLT)strtol(s1, &endptr, 10);
        if (UNLIKELY(endptr==NULL)) goto err4;
        if (UNLIKELY(NULL==fgets(s, 64, file))) { goto err4; }
        s1 = strchr(s, ' ')+1;
        header.loopmode1 = (int16) strtol(s1, &endptr, 10);
        if (UNLIKELY(endptr==NULL)) goto err4;
        if (UNLIKELY(NULL==fgets(s, 64, file))) {goto err4;}
        s1 = strchr(s, ' ')+1;
        header.loopmode2 = (int16) strtol(s1, &endptr, 10);
        if (UNLIKELY(endptr==NULL)) goto err4;
        if (UNLIKELY(NULL==fgets(s, 64, file))) {goto err4;}
        s1 = strchr(s, ' ')+1;
        header.begin1 = strtol(s1, &endptr, 10);
        if (UNLIKELY(endptr==NULL)) goto err4;
        if (UNLIKELY(NULL==fgets(s, 64, file))){ goto err4;}
        s1 = strchr(s, ' ')+1;
        header.end1 = strtol(s1, &endptr, 10);
        if (UNLIKELY(endptr==NULL)) goto err4;
        if (UNLIKELY(NULL==fgets(s, 64, file))) {goto err4; }
        s1 = strchr(s, ' ')+1;
        header.begin2 = strtol(s1, &endptr, 10);
        if (UNLIKELY(endptr==NULL)) goto err4;
        if (UNLIKELY(NULL==fgets(s, 64, file))){ goto err4;}
        s1 = strchr(s, ' ')+1;
        header.end2 = strtol(s1, &endptr, 10);
        if (UNLIKELY(endptr==NULL)) goto err4;
        if (UNLIKELY(NULL==fgets(s, 64, file))){ goto err4;}
        s1 = strchr(s, ' ')+1;
        header.soundend = strtol(s1, &endptr, 10);
        if (UNLIKELY(endptr==NULL)) { goto err4; }
        if (UNLIKELY(NULL==fgets(s, 64, file))){goto err4;}
        s1 = strchr(s, ' ')+1;
        header.flenfrms = strtol(s1, &endptr, 10);
        if (UNLIKELY(endptr==NULL)) { goto err4; }
        if (UNLIKELY(NULL==fgets(s, 64, file))){ goto err4;}
        s1 = strchr(s, ' ')+1;
        header.nchanls = strtol(s1, &endptr, 10);
        if (UNLIKELY(endptr==NULL)) goto err4;
        if (UNLIKELY(NULL==fgets(s, 64, file))){ goto err4; }
        s1 = strchr(s, ' ')+1;
        header.fno = strtol(s1, &endptr, 10);
        if (UNLIKELY(endptr==NULL)) goto err4;
        if (UNLIKELY(NULL==fgets(s, 64, file))) {goto err4;}
        s1 = strchr(s, ' ')+1;
        header.gen01args.gen01 = (MYFLT)cs_strtod(s1, &endptr);
        if (UNLIKELY(endptr==NULL)) goto err4;
        if (UNLIKELY(NULL==fgets(s, 64, file))) { goto err4;}
        s1 = strchr(s, ' ')+1;
        header.gen01args.ifilno = (MYFLT)cs_strtod(s1, &endptr);
        if (UNLIKELY(endptr==NULL)) goto err4;
        if (UNLIKELY(NULL==fgets(s, 64, file))) {goto err4;}
        s1 = strchr(s, ' ')+1;
        header.gen01args.iskptim = (MYFLT)cs_strtod(s1, &endptr);
        if (UNLIKELY(endptr==NULL)) goto err4;
        if (UNLIKELY(NULL==fgets(s, 64, file))){ goto err4;}
        s1 = strchr(s, ' ')+1;
        header.gen01args.iformat = (MYFLT)cs_strtod(s1, &endptr);
        if (UNLIKELY(endptr==NULL)) goto err4;
        if (UNLIKELY(NULL==fgets(s, 64, file))){ goto err4;}
        s1 = strchr(s, ' ')+1;
        header.gen01args.channel = (MYFLT)cs_strtod(s1, &endptr);
        if (UNLIKELY(endptr==NULL)) goto err4;
        if (UNLIKELY(NULL==fgets(s, 64, file))) {goto err4;}
        s1 = strchr(s, ' ')+1;
        header.gen01args.sample_rate = (MYFLT)cs_strtod(s1, &endptr);
        if (UNLIKELY(endptr==NULL)) goto err4;
        if (UNLIKELY(NULL==fgets(s, 64, file))) {goto err4;}
        //s1 = strchr(s, ' ')+1;
        /* WARNING! skips header.gen01args.strarg from saving/loading
           in text format */
        header.fno = (int32) fno;
        if (fno_f == fno) {
          ftp = ft_func(csound, &fno_f);
          if (ftp->flen < header.flen){
             if (UNLIKELY(csound->FTAlloc(csound, fno, (int32_t) header.flen) != 0))
             goto err;
          }
        }
        else {
         if (UNLIKELY(csound->FTAlloc(csound, fno, (int32_t) header.flen) != 0))
          goto err;
         ftp = ft_func(csound, &fno_f);
        }
        memcpy(ftp, &header, sizeof(FUNC) - sizeof(MYFLT));
        memset(ftp->ftable, 0, sizeof(MYFLT) * (ftp->flen + 1));
        for (j = 0; j <= ftp->flen; j++) {
          if (UNLIKELY(NULL==fgets(s, 64, file))) goto err4;
          ftp->ftable[j] = (MYFLT) cs_strtod(s, &endptr);
          if (UNLIKELY(endptr==NULL)) goto err4;
        }
        if (UNLIKELY(NULL==fgets(s, 64, file))) goto err4;
        argp++;
      }
    }
    csound->FileClose(csound, fd);
    return OK;
 err:
    csound->FileClose(csound, fd);
    return err_func(csound, &(p->h),
                    Str("ftload: error allocating ftable"));
 err2:
    return err_func(csound, &(p->h), Str("ftload: no table numbers"));
 err3:
    return err_func(csound, &(p->h), Str("ftload: unable to open file"));
 err4:
    csound->FileClose(csound, fd);
    return err_func(csound, &(p->h), Str("ftload: incorrect file"));
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
    int32_t   nargs = csound->GetInputArgCnt(p) - 3;
    FILE  *file = NULL;
    int32_t   (*err_func)(CSOUND *, OPDS *, const char *, ...);
    FUNC  *(*ft_func)(CSOUND *, MYFLT *);
    void  *fd;

    if (strncmp(csound->GetOpcodeName(p), "ftsave.", 7) != 0) {
      ft_func = csound->FTFindP;
      err_func = csound->PerfError;
    }
    else {
      nargs = csound->GetInputArgCnt(p) - 2;
      ft_func = csound->FTnp2Finde;
      err_func = myInitError;
    }

    if (UNLIKELY(nargs <= 0))
      goto err2;

    if (!istring) {
      if (csound->ISSTRCOD(*p->ifilno))
        csound->strarg2name(csound, filename, p->ifilno, "ftsave.", 0);
      else strNcpy(filename, get_arg_string(csound,*p->ifilno), MAXNAME);
    } else {
      strNcpy(filename, ((STRINGDAT *)p->ifilno)->data, MAXNAME);
    }

    if (*p->iflag <= FL(0.0)) {
      fd = csound->FileOpen2(csound, &file, CSFILE_STD, filename, "wb",
                               "", CSFTYPE_FTABLES_BINARY, 0);
      if (UNLIKELY(fd == NULL)) goto err3;
      while (nargs--) {
        FUNC *ftp;
        //csound->Message(csound, "saving table %f \n", **argp);
        if ( *argp && (ftp = ft_func(csound, *argp)) != NULL) {
          MYFLT *table = ftp->ftable;
          int32 flen = ftp->flen;
          int32_t n;
          n = fwrite(ftp, sizeof(FUNC) - sizeof(MYFLT) - SSTRSIZ, 1, file);
          if (UNLIKELY(n!=1)) goto err4;
          n = fwrite(table, sizeof(MYFLT), flen + 1, file);
          if (UNLIKELY(n!=flen + 1)) goto err4;
        }
        else goto err;
        argp++;
      }
    }
    else {
      fd = csound->FileOpen2(csound, &file, CSFILE_STD, filename, "w",
                               "", CSFTYPE_FTABLES_TEXT, 0);
      if (UNLIKELY(fd == NULL)) goto err3;
      while (nargs--) {
        FUNC *ftp;

        if ((ftp = ft_func(csound, *argp)) != NULL) {
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
          fprintf(file,"lodiv: %f\n",ftp->lodiv);
          fprintf(file,"cvtbas: %f\n",ftp->cvtbas);
          fprintf(file,"cpscvt: %f\n",ftp->cpscvt);
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

          for (j = 0; j <= flen; j++) {
            MYFLT val = table[j];
            fprintf(file,"%f\n",val);
          }
          fprintf(file,"---------END OF TABLE---------------\n");
        }
        else goto err;
        argp++;
      }
    }
    csound->FileClose(csound, fd);
    return OK;
 err:
    csound->FileClose(csound, fd);
    return err_func(csound, &(p->h),
                    Str("ftsave: Bad table number. Saving is possible "
                        "only for existing tables."));
 err2:
    return err_func(csound, &(p->h), Str("ftsave: no table numbers"));
 err3:
    return err_func(csound, &(p->h), Str("ftsave: unable to open file"));
 err4:
    return err_func(csound, &(p->h), Str("ftsave: failed to write file"));
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
           sizeof(MYFLT*) * (csound->GetInputArgCnt(p) - 3));
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

    *p->ifno = FL(0.0);
    ftevt =(EVTBLK*) csound->Malloc(csound, sizeof(EVTBLK));
    ftevt->opcod = 'f';
    ftevt->strarg = NULL;
    fp = &ftevt->p[0];
    fp[0] = FL(0.0);
    fp[1] = *p->p1;                                     /* copy p1 - p5 */
    fp[2] = ftevt->p2orig = FL(0.0);                    /* force time 0 */
    fp[3] = ftevt->p3orig = *p->p3;
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

    ARRAYDAT *array = (ARRAYDAT*) (p->p5);
    n = array->sizes[0];
    ftevt->pcnt = (int16) n+4;
    memcpy(&fp[5], array->data, n*sizeof(MYFLT));
    n = csound->hfgens(csound, &ftp, ftevt, 1);         /* call the fgen */
    csound->Free(csound,ftevt);
    if (UNLIKELY(n != 0))
      return csound->InitError(csound, Str("ftgen error"));
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

#define S(x)    sizeof(x)

static OENTRY localops[] = {
  { "ftgen",    S(FTGEN),     TW, 1,  "i",  "iiiiim", (SUBR) ftgen, NULL, NULL    },
  { "ftgen.S",    S(FTGEN),   TW, 1,  "i",  "iiiSim", (SUBR) ftgen_S, NULL, NULL  },
  { "ftgen.iS",    S(FTGEN),  TW, 1,  "i",  "iiiiSm", (SUBR) ftgen_iS, NULL, NULL },
  { "ftgen.SS",    S(FTGEN),  TW, 1,  "i",  "iiiSSm", (SUBR) ftgen_SS, NULL, NULL },
  { "ftgen",    S(FTGEN),     TW, 1,  "i",  "iiiii[]", (SUBR) ftgen_list_i, NULL  },
  { "ftgen",    S(FTGEN),     TW, 1,  "i",  "iiiSi[]", (SUBR) ftgen_list_S, NULL  },
  { "ftgentmp.i", S(FTGEN),   TW, 1,  "i",  "iiiiim", (SUBR) ftgentmp, NULL, NULL },
  { "ftgentmp.iS", S(FTGEN),  TW, 1,  "i",  "iiiiSm", (SUBR) ftgentmp_S, NULL,NULL},
  { "ftgentmp.Si", S(FTGEN),  TW, 1,  "i",  "iiiSim", (SUBR) ftgentmp_Si,NULL,NULL},
  { "ftgentmp.SS", S(FTGEN),  TW, 1,  "i",  "iiiSSm", (SUBR) ftgentmp_SS,NULL,NULL},
  { "ftfree",   S(FTFREE),    TW, 1,  "",   "ii",     (SUBR) ftfree, NULL, NULL   },
  { "ftsave",   S(FTLOAD),    TR, 1,  "",   "iim",    (SUBR) ftsave, NULL, NULL   },
  { "ftsave.S",   S(FTLOAD),  TR, 1,  "",   "Sim",    (SUBR) ftsave_S, NULL, NULL },
  { "ftload",   S(FTLOAD),    TW, 1,  "",   "iim",    (SUBR) ftload, NULL, NULL   },
  { "ftload.S",  S(FTLOAD), TW, 1,  "",   "Sim",    (SUBR) ftload_S, NULL, NULL },
  { "ftsavek",  S(FTLOAD_K),  TW, 3,  "",   "ikim",   (SUBR) ftsave_k_set,
                                                  (SUBR) ftsave_k, NULL       },
  { "ftsavek.S",  S(FTLOAD_K),  TW, 3,  "",   "Skim",   (SUBR) ftsave_k_set,
                                                  (SUBR) ftsave_kS, NULL       },
  { "ftloadk",  S(FTLOAD_K),  TW, 3,  "",   "ikim",   (SUBR) ftsave_k_set,
    (SUBR) ftload_k, NULL       },
  { "ftloadk.S",  S(FTLOAD_K),  TW, 3,  "",   "Skim",   (SUBR) ftsave_k_set,
                                                  (SUBR) ftload_kS, NULL       }
};

int32_t ftgen_init_(CSOUND *csound)
{
    return csound->AppendOpcodes(csound, &(localops[0]),
                                 (int32_t
                                  ) (sizeof(localops) / sizeof(OENTRY)));
}
