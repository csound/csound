/*
    rdorch.c:

    Copyright (C) 1991-2002 Barry Vercoe, John ffitch, Istvan Varga

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

#include "cs.h"                 /*                      RDORCH.C        */
#include <ctype.h>
#include "namedins.h"   /* IV - Oct 31 2002 */

#ifdef sun
#define   SEEK_SET        0
#define   SEEK_CUR        1
#define   SEEK_END        2
#endif

#define LINMAX    1000
#define LENMAX    8192L
#define GRPMAX    VARGMAX
#define LBLMAX    100

typedef struct  {
        int     reqline;
        char    *label;
} LBLREQ;

#define MARGS   (3)

typedef struct MACRO {          /* To store active macros */
    char          *name;        /* Use is by name */
    int           acnt;         /* Count of arguments */
    char          *body;        /* The text of the macro */
    struct MACRO  *next;        /* Chain of active macros */
    int           margs;        /* ammount of space for args */
    char          *arg[MARGS];  /* With these arguments */
} MACRO;

typedef struct in_stack {
    short   string;
    short   args;
    char    *body;
    FILE    *file;
    void    *fd;
    MACRO   *mac;
    int     line;
    int     unget_cnt;
    char    unget_buf[128];
} IN_STACK;

typedef struct iflabel {            /* for if/else/endif */
    char    els[256];
    char    end[256];
    int     ithen;        /* "is an i-rate only" boolean */
    struct  iflabel *prv;
} IFLABEL;

typedef struct {
    MACRO   *macros;
    long    lenmax /* = LENMAX */;  /* Length of input line buffer  */
    char    **linadr;               /* adr of each line in text     */
#if 0   /* unused */
    int     *srclin;                /* text no. of expanded lines   */
#endif
    int     curline;                /* current line being examined  */
    char    *collectbuf;            /* splitline collect buffer     */
    char    **group;                /* splitline local storage      */
    char    **grpsav;               /* copy of above                */
    long    grpmax /* = GRPMAX */;  /* Size of group structure      */
    int     opgrpno;                /* grpno identified as opcode   */
    int     linopnum;               /* data for opcode in this line */
    char    *linopcod;
    int     linlabels;              /* count of labels this line    */
    LBLREQ  *lblreq;
    int     lblmax;
    int     lblcnt;
    int     lgprevdef;
    int     opnum;                  /* opcod data carriers          */
    char    *opcod;                 /*  (line or subline)           */
    ARGLST  *nxtarglist, *nullist;
    IN_STACK  *inputs, *str;
    FILE    *fp;
    void    *fd;
    int     input_size, input_cnt;
    int     pop;                    /* Number of macros to pop      */
    int     ingappop /* = 1 */;
    int     linepos /* = -1 */;
    long    *typemask_tabl;
    long    *typemask_tabl_in, *typemask_tabl_out;
    long    orchsiz;
    IFLABEL *iflabels;
    int     repeatingElseifLine;
    long    tempNum /* = 300L */;
    int     repeatingElseLine;
    short   grpcnt, nxtest /* = 1 */;
    short   xprtstno, polcnt;
    short   instrblk, instrcnt;
    short   opcodblk;               /* IV - Sep 8 2002 */
    TEXT    optext;                 /* struct to be passed back to caller */
} RDORCH_GLOBALS;

#define ST(x)   (((RDORCH_GLOBALS*) ((ENVIRON*) csound)->rdorchGlobals)->x)

static  void    intyperr(ENVIRON *, int, char, char);
static  void    printgroups(ENVIRON *, int);
static  int     isopcod(ENVIRON *, char *);
static  void    lblrequest(ENVIRON *, char *), lblfound(ENVIRON *, char *);
static  void    lblclear(ENVIRON *), lblchk(ENVIRON *);
static  void    lexerr(ENVIRON *, char *);
static  void    synterrp(ENVIRON *, const char *, char *);

#include "typetabl.h"                   /* IV - Oct 31 2002 */

void orchRESET(ENVIRON *csound)
{
    int     i;

    csound->synterrcnt = 0;
    strsav_destroy(csound);

    if (csound->rdorchGlobals == NULL)
      return;

    mfree(csound, ST(linadr));
#if 0   /* unused */
    mfree(csound, ST(srclin));
#endif
    if (ST(nxtarglist) != NULL)
      mfree(csound, ST(nxtarglist));
    if (ST(nullist) != NULL)
      mfree(csound, ST(nullist));
    while (ST(macros)) {
      mfree(csound, ST(macros)->body);
      mfree(csound, ST(macros)->name);
      for (i = 0; i < ST(macros)->acnt; i++)
        mfree(csound, ST(macros)->arg[i]);
      ST(macros) = ST(macros)->next;
    }
    /* IV - Oct 31 2002 */
    if (ST(typemask_tabl)) {
      mfree(csound, ST(typemask_tabl));
      mfree(csound, ST(typemask_tabl_in));
      mfree(csound, ST(typemask_tabl_out));
    }
    csound->Free(csound, csound->rdorchGlobals);
    csound->rdorchGlobals = NULL;
}

ARGLST* copy_arglist(ENVIRON *csound, ARGLST *old)
{
    size_t n = sizeof(ARGLST) + old->count * sizeof(char*) - sizeof(char*);
    ARGLST *nn = (ARGLST*)mmalloc(csound, n);
/*  csound->Message(csound, "copy_arglist: %d args\n", old->count); */
    memcpy(nn, old, n);
    memset(old, 0, n);
    return nn;
}

static inline int isNameChar(int c, int pos)
{
    c = (int) ((unsigned char) c);
    return (isalpha(c) || (pos && (c == '_' || isdigit(c))));
}

/* Functions to read/unread chracters from
 * a stack of file and macro inputs */

static inline void ungetorchar(void *csound, int c)
{
    if (ST(str)->unget_cnt < 128)
      ST(str)->unget_buf[ST(str)->unget_cnt++] = (char) c;
    else
      csoundDie(csound, "ungetorchar(): buffer overflow");
}

static void skiporchar(ENVIRON *csound)
{
    int c;
 top:
    if (ST(str)->unget_cnt) {
      c = (int) ((unsigned char) ST(str)->unget_buf[--ST(str)->unget_cnt]);
      if (c == '\n')
        return;
      goto top;
    }
    else if (ST(str)->string) {
      c = *ST(str)->body++;
      if (c == '\n') {
        ST(str)->line++; ST(linepos) = -1;
        return;
      }
      if (c == '\0') {
        ST(pop) += ST(str)->args;
        ST(str)--; ST(input_cnt)--;
        ST(linepos) = -1;
        return;
      }
    }
    else {
      c = getc(ST(str)->file);
      if (c == '\n' || c == '\r' || c == 26) {    /* MS-DOS spare ^Z */
        ST(str)->line++; ST(linepos) = -1;
        if (c == '\r') {
          if ((c = getc(ST(str)->file)) != '\n')
            ungetc(c, ST(str)->file);
        }
        return;
      }
      if (c == EOF) {
        if (ST(str) == &ST(inputs)[0]) {
          ST(linepos) = -1;
          return;
        }
        if (ST(str)->fd != NULL) {
          csound->FileClose(csound, ST(str)->fd); ST(str)->fd = NULL;
        }
        ST(str)--; ST(input_cnt)--;
        ST(str)->line++; ST(linepos) = -1;
        return;
      }
    }
    ST(linepos)++;
    goto top;
}

static int getorchar(ENVIRON *csound)
{
    int c;
 top:
    if (ST(str)->unget_cnt) {
      c = (int) ((unsigned char) ST(str)->unget_buf[--ST(str)->unget_cnt]);
      return c;
    }
    else if (ST(str)->string) {
      c = *ST(str)->body++;
      if (c == '\0') {
        ST(pop) += ST(str)->args;
        ST(str)--; ST(input_cnt)--;
        goto top;
      }
    }
    else {
      c = getc(ST(str)->file);
      if (c == 26) goto top;    /* MS-DOS spare ^Z */
      if (c == EOF) {
        if (ST(str) == &ST(inputs)[0]) return EOF;
        if (ST(str)->fd != NULL) {
          csound->FileClose(csound, ST(str)->fd); ST(str)->fd = NULL;
        }
        ST(str)--; ST(input_cnt)--; goto top;
      }
    }
    if (c == '\r') {
      int d;
      if ((d = getc(ST(str)->file) != '\n')) {
        ungetc(d, ST(str)->file);
      }
      c = '\n';
    }
    if (c == '\n') {
      ST(str)->line++; ST(linepos) = -1;
    }
    else ST(linepos)++;
    if (ST(ingappop) && ST(pop)) {
      do {
        MACRO *nn = ST(macros)->next;
        int i;
#ifdef MACDEBUG
        csound->Message(csound, "popping %s\n", ST(macros)->name);
#endif
        mfree(csound, ST(macros)->name); mfree(csound, ST(macros)->body);
        for (i=0; i<ST(macros)->acnt; i++)
          mfree(csound, ST(macros)->arg[i]);
        mfree(csound, ST(macros));
        ST(macros) = nn;
        ST(pop)--;
      } while (ST(pop));
    }
    return c;
}

void *fopen_path(ENVIRON *csound, FILE **fp, char *name, char *basename,
                                  char *env)
{
    void *fd;
                                /* First try to open name given */
    fd = csound->FileOpen(csound, fp, CSFILE_STD, name, "rb", NULL);
    if (fd != NULL)
      return fd;
                                /* if that fails try in base directory */
    if (basename != NULL) {
      char *p, name_full[1024];
      strcpy(name_full, basename);
      p = strrchr(name_full, DIRSEP);
      if (p == NULL) p = strrchr(name_full, '/');
      if (p == NULL) p = strrchr(name_full, '\\');
      if (p != NULL) {
        strcpy(p + 1, name);
        fd = csound->FileOpen(csound, fp, CSFILE_STD, name_full, "rb", NULL);
        if (fd != NULL)
          return fd;
      }
    }
                                /* or use env argument */
    fd = csound->FileOpen(csound, fp, CSFILE_STD, name, "rb", env);
    return fd;
}

static void init_omacros(ENVIRON *csound, NAMES *nn)
{
    while (nn) {
      char *s = nn->mac;
      char *p = strchr(s,'=');
      MACRO *mm = (MACRO*)mmalloc(csound, sizeof(MACRO));
      mm->margs = MARGS;  /* Initial size */
      if (p==NULL) p = s+strlen(s);
      if (csound->oparms->msglevel)
        csound->Message(csound,Str("Macro definition for %*s\n"), p-s, s);
      s = strchr(s,':')+1;                     /* skip arg bit */
      mm->name = mmalloc(csound, p-s+1);
      mm->name[p-s] = '\0';
      strncpy(mm->name, s, p-s);
      mm->acnt = 0;
      mm->body = (char*)mmalloc(csound, strlen(p+1)+1);
      strcpy(mm->body, p+1);
      mm->next = ST(macros);
      ST(macros) = mm;
      nn = nn->next;
    }
}

void rdorchfile(ENVIRON *csound)    /* read entire orch file into txt space */
{
    int     c, lincnt;
    int     srccnt;
    char    *cp, *endspace, *ortext;
    int     linmax = LINMAX;        /* Maximum number of lines      */
    int     heredoc = 0, openquote = 0;

    if (csound->rdorchGlobals == NULL) {
      csound->rdorchGlobals = csound->Calloc(csound, sizeof(RDORCH_GLOBALS));
      ST(lenmax)    = LENMAX;
      ST(grpmax)    = GRPMAX;
      ST(ingappop)  = 1;
      ST(linepos)   = -1;
      ST(tempNum)   = 300L;
      ST(nxtest)    = 1;
    }
    init_omacros(csound, csound->omacros);
    /* IV - Oct 31 2002: create tables for easier checking for common types */
    if (!ST(typemask_tabl)) {
      long *ptr = (long*) typetabl1;
      ST(typemask_tabl) = (long*) mcalloc(csound, sizeof(long) * 256);
      ST(typemask_tabl_in) = (long*) mcalloc(csound, sizeof(long) * 256);
      ST(typemask_tabl_out) = (long*) mcalloc(csound, sizeof(long) * 256);
      while (*ptr) {            /* basic types (both for input */
        long pos = *ptr++;      /* and output) */
        ST(typemask_tabl)[pos] = ST(typemask_tabl_in)[pos] =
                                 ST(typemask_tabl_out)[pos] = *ptr++;
      }
      ptr = (long*) typetabl2;
      while (*ptr) {            /* input types */
        long pos = *ptr++;
        ST(typemask_tabl_in)[pos] = *ptr++;
      }
      ptr = (long*) typetabl3;
      while (*ptr) {            /* output types */
        long pos = *ptr++;
        ST(typemask_tabl_out)[pos] = *ptr++;
      }
    }
    csound->Message(csound, Str("orch compiler:\n"));
    if ((ST(fd) = csound->FileOpen(csound, &ST(fp), CSFILE_STD,
                                   csound->orchname, "rb", NULL)) == NULL)
      csoundDie(csound, Str("cannot open orch file %s"), csound->orchname);
    if (fseek(ST(fp), 0L, SEEK_END) != 0)
      csoundDie(csound, Str("cannot find end of file %s"), csound->orchname);
    if ((ST(orchsiz) = ftell(ST(fp))) <= 0)
      csoundDie(csound, Str("ftell error on %s"), csound->orchname);
    rewind(ST(fp));
    ST(inputs) = (IN_STACK*) mmalloc(csound, 20 * sizeof(IN_STACK));
    ST(input_size) = 20;
    ST(input_cnt) = 0;
    ST(str) = ST(inputs);
    ST(str)->string = 0;
    ST(str)->file = ST(fp);
    ST(str)->fd = ST(fd);
    ST(str)->body = csound->orchname;
    ST(str)->line = 1;
    ST(str)->unget_cnt = 0;
    ortext = mmalloc(csound, ST(orchsiz) + 1);          /* alloc mem spaces */
    ST(linadr) = (char **) mmalloc(csound, (LINMAX + 1) * sizeof(char *));
#if 0   /* unused */
    ST(srclin) = (int *) mmalloc(csound, (LINMAX + 1) * sizeof(int));
    ST(srclin)[1] = 1;
#endif
    strsav_create(csound);
    lincnt = srccnt = 1;
    cp = ST(linadr)[1] = ortext;
    endspace = ortext + ST(orchsiz) + 1;
    strsav_string(csound, "sr");
    ST(group) = (char **)mcalloc(csound, (GRPMAX+1)*sizeof(char*));
    ST(grpsav)= (char **)mcalloc(csound, (GRPMAX+1)*sizeof(char*));
    ST(lblreq) = (LBLREQ*)mcalloc(csound, LBLMAX*sizeof(LBLREQ));
    ST(lblmax) = LBLMAX;

    while ((c = getorchar(csound)) != EOF) {    /* read entire orch file  */
      if (cp == endspace-1) {                   /* Must extend */
        char * orold = ortext;
        int i;
        ST(orchsiz) = ST(orchsiz) + (ST(orchsiz) >> 4) + 1L;
        ST(orchsiz) = (ST(orchsiz) + 511L) & (~511L);
        ortext = mrealloc(csound, ortext, ST(orchsiz));
        endspace = ortext + ST(orchsiz) + 1;
        if (ortext != orold) {
          int adj = ortext - orold;
          for (i=1; i<=lincnt; i++)
            ST(linadr)[i] += adj; /* Relocate */
          cp += adj;
        }
      }
      *cp++ = c;
      if (c == '{' && !openquote) {
        char  c2 = getorchar(csound);
        if (c2 == '{') {
          heredoc = 1;
          *cp++ = c;
        }
        else
          ungetorchar(csound, c2);
      }
      else if (c == '}' && heredoc) {
        char  c2 = getorchar(csound);
        if (c2 == '}') {
          heredoc = 0;
          *cp++ = c;
        }
        else
          ungetorchar(csound, c2);
      }
      if (c == ';' && !heredoc) {
        skiporchar(csound);
        c = '\n';
      }
      if (c == '"' && !heredoc) {
        openquote = !openquote;
      }
      if (c == '\\' && !heredoc) {  /* Continuation?? */
        while ((c = getorchar(csound))==' ' || c == '\t');  /* Ignore spaces */
        if (c==';') {                                /* Comments get skipped */
          skiporchar(csound);
          c = '\n';
        }
        if (c == '\n') {
          cp--;                                      /* Ignore newline */
          srccnt++;                                  /*    record a fakeline */
/*        ST(srclin)[++lincnt] = 0;     unused  */
/*        ST(linadr)[lincnt] = cp; */
        }
        else {
          *cp++ = c;
        }
      }
      else if (c == '\n') {                          /* at each new line */
        char *lp = ST(linadr)[lincnt];
        while ((c = *lp) == ' ' || c == '\t')
          lp++;
        if (*lp != '\n' && *lp != ';') {
          ST(curline) = lincnt - 1;
        }
        srccnt++;
        if (++lincnt >= linmax) {
          linmax += 100;
          ST(linadr) = (char**) mrealloc(csound, ST(linadr), (linmax + 1)
                                                             * sizeof(char*));
#if 0   /* unused */
          ST(srclin) = (int*) mrealloc(csound, ST(srclin), (linmax + 1)
                                                           * sizeof(int));
#endif
        }
  /*    ST(srclin)[lincnt] = srccnt;    unused  */
        ST(linadr)[lincnt] = cp;            /* record the adrs */
      }
      else if (c == '#' && ST(linepos) == 0 && !heredoc) {
        /* Start Macro definition */
        /* also deal with #include here */
        char  mname[100];
        int   i = 0;
        int   arg = 0;
        int   size = 100;
        MACRO *mm = (MACRO*)mmalloc(csound, sizeof(MACRO));
        mm->margs = MARGS;  /* Initial size */
        cp--;
        while (isspace(c = getorchar(csound)));
        if (c=='d') {
          if ((c = getorchar(csound))!='e' || (c = getorchar(csound))!='f' ||
              (c = getorchar(csound))!='i' || (c = getorchar(csound))!='n' ||
              (c = getorchar(csound))!='e')
            lexerr(csound, Str("Not #define"));
          while (isspace(c = getorchar(csound)));
          while (isNameChar(c, i)) {
            mname[i++] = c;
            c = getorchar(csound);
          }
          mname[i] = '\0';
          if (csound->oparms->msglevel)
            csound->Message(csound,Str("Macro definition for %s\n"), mname);
          mm->name = mmalloc(csound, i+1);
          strcpy(mm->name, mname);
          if (c == '(') {       /* arguments */
#ifdef MACDEBUG
            csound->Message(csound, "M-arguments: ");
#endif
            do {
              while (isspace(c = getorchar(csound)));
              i = 0;
              while (isNameChar(c, i)) {
                mname[i++] = c;
                c = getorchar(csound);
              }
              mname[i] = '\0';
#ifdef MACDEBUG
              csound->Message(csound, "%s\t", mname);
#endif
              mm->arg[arg] = mmalloc(csound, i+1);
              strcpy(mm->arg[arg++], mname);
              if (arg>=mm->margs) {
                mm = (MACRO*)mrealloc(csound, mm,
                                      sizeof(MACRO)+mm->margs*sizeof(char*));
                mm->margs += MARGS;
              }
              while (isspace(c)) c = getorchar(csound);
            } while (c=='\'' || c=='#');
            if (c!=')') csound->Message(csound, Str("macro error\n"));
          }
          mm->acnt = arg;
          i = 0;
          while ((c = getorchar(csound)) != '#'); /* Skip to next # */
          mm->body = (char*)mmalloc(csound, 100);
          while ((c = getorchar(csound)) != '#') {
            mm->body[i++] = c;
            if (i>= size) mm->body = mrealloc(csound, mm->body, size += 100);
            if (c=='\\') {      /* allow escaped # */
              mm->body[i++] = c = getorchar(csound);
              if (i>= size) mm->body = mrealloc(csound, mm->body, size += 100);
            }
            if (c == '\n') {
              srccnt++;
            }
          }
          mm->body[i]='\0';
          mm->next = ST(macros);
          ST(macros) = mm;
#ifdef MACDEBUG
          csound->Message(csound, "Macro %s with %d arguments defined\n",
                                  mm->name, mm->acnt);
#endif
          c = ' ';
        }
        else if (c=='i') {
          int delim;
          c = getorchar(csound);
          if (c=='n') {         /* #include */
            if ((c = getorchar(csound))!='c' || (c = getorchar(csound))!='l' ||
                (c = getorchar(csound))!='u' || (c = getorchar(csound))!='d' ||
                (c = getorchar(csound))!='e')
              lexerr(csound, Str("Not #include"));
            while (isspace(c = getorchar(csound)));
            delim = c;
            i = 0;
            while ((c=getorchar(csound))!=delim) mname[i++] = c;
            mname[i]='\0';
            while ((c=getorchar(csound))!='\n');
#ifdef MACDEBUG
            csound->Message(csound, "#include \"%s\"\n", mname);
#endif
            ST(input_cnt)++;
            if (ST(input_cnt) >= ST(input_size)) {
              ST(input_size) += 20;
              ST(inputs) = mrealloc(csound, ST(inputs), ST(input_size)
                                                        * sizeof(IN_STACK));
            }
            ST(str) = (IN_STACK*) ST(inputs) + (int) ST(input_cnt);
            ST(str)->string = 0;
            ST(str)->fd = fopen_path(csound, &(ST(str)->file),
                                             mname, csound->orchname, "INCDIR");
            if (ST(str)->fd == NULL) {
              csound->Message(csound,
                              Str("Cannot open #include'd file %s\n"), mname);
              /* Should this stop things?? */
              ST(str)--; ST(input_cnt)--;
            }
            else {
              ST(str)->body = csound->GetFileName(ST(str)->fd);
              ST(str)->line = 1;
              ST(str)->unget_cnt = 0;
              ST(linepos) = -1;
            }
          }
          else {
            if (c !='f' || (c = getorchar(csound))!='d' ||
                (c = getorchar(csound))!='e' || (c = getorchar(csound))!='f')
              lexerr(csound, "Not #ifdef");
            /* #ifdef XXX */
            while (isspace(c = getorchar(csound)));
            while (isNameChar(c, i)) {
              mname[i++] = c;
              c = getorchar(csound);
            }
            mname[i] = '\0';
            mm = ST(macros);
            while (mm != NULL) {  /* Find the definition */
              if (!(strcmp (mname, mm->name))) break;
              mm = mm->next;
            }
            if (mm==NULL) csound->Message(csound,"...not defined\n");
            else csound->Message(csound,"...defined\n");
            /* Problem is what to do about it!! */
            lexerr(csound, "#ifdef not complete");
          }
        }
        else if (c=='u') {
          if ((c = getorchar(csound))!='n' || (c = getorchar(csound))!='d' ||
              (c = getorchar(csound))!='e' || (c = getorchar(csound))!='f')
            lexerr(csound, Str("Not #undef"));
          while (isspace(c = getorchar(csound)));
          while (isNameChar(c, i)) {
            mname[i++] = c;
            c = getorchar(csound);
          }
          mname[i] = '\0';
          if(csound->oparms->msglevel)
            csound->Message(csound,Str("macro %s undefined\n"), mname);
          if (strcmp(mname, ST(macros)->name)==0) {
            MACRO *mm=ST(macros)->next;
            mfree(csound, ST(macros)->name); mfree(csound, ST(macros)->body);
            for (i=0; i<ST(macros)->acnt; i++)
              mfree(csound, ST(macros)->arg[i]);
            mfree(csound, ST(macros)); ST(macros) = mm;
          }
          else {
            MACRO *mm = ST(macros);
            MACRO *nn = mm->next;
            while (strcmp(mname, nn->name)!=0) {
              mm = nn; nn = nn->next;
              if (nn == NULL)
                lexerr(csound, Str("Undefining undefined macro"));
            }
            mfree(csound, nn->name); mfree(csound, nn->body);
            for (i=0; i<nn->acnt; i++)
              mfree(csound, nn->arg[i]);
            mm->next = nn->next; mfree(csound, nn);
          }
          while (c!='\n') c = getorchar(csound);  /* ignore rest of line */
        }
        else {
          csound->Message(csound, Str("Warning: Unknown # option"));
          ungetorchar(csound, c);
          c = '#';
        }
      }
      else if (c == '$' && !heredoc) {
        char      name[100];
        int       i = 0;
        int       j;
        MACRO     *mm, *mm_save = NULL;
        ST(ingappop) = 0;
        while (isNameChar((c = getorchar(csound)), i)) {
          name[i++] = c; name[i] = '\0';
          mm = ST(macros);
          while (mm != NULL) {  /* Find the definition */
            if (!(strcmp(name, mm->name))) {
              mm_save = mm;     /* found a match, save it */
              break;
            }
            mm = mm->next;
          }
        }
        mm = mm_save;
        if (mm == NULL) {
          lexerr(csound, Str("Undefined macro"));
          continue;
        }
        if ((int) strlen(mm->name) != i) {
          int cnt = i - (int) strlen(mm->name);
          csound->Warning(csound, Str("$%s matches macro name $%s"),
                                  name, mm->name);
          do {
            ungetorchar(csound, c);
            c = name[--i];
          } while (cnt--);
        }
        else if (c != '.')
          ungetorchar(csound, c);
#ifdef MACDEBUG
        csound->Message(csound, "Found macro %s required %d arguments\n",
                                mm->name, mm->acnt);
#endif
        /* Should bind arguments here */
        /* How do I recognise entities?? */
        if (mm->acnt) {
          if ((c = getorchar(csound)) != '(')
            lexerr(csound, Str("Syntax error in macro call"));
          for (j=0; j<mm->acnt; j++) {
            char term = (j==mm->acnt-1 ? ')' : '\'');
            char trm1 = (j==mm->acnt-1 ? ')' : '#'); /* Compatability */
            MACRO* nn = (MACRO*) mmalloc(csound, sizeof(MACRO));
            int size = 100;
            nn->name = mmalloc(csound, strlen(mm->arg[j])+1);
            strcpy(nn->name, mm->arg[j]);
#ifdef MACDEBUG
            csound->Message(csound, "defining argument %s ", nn->name);
#endif
            i = 0;
            nn->body = (char*) mmalloc(csound, 100);
            while ((c = getorchar(csound))!= term && c!=trm1) {
              if (i>98) {
                csound->Die(csound, Str("Missing argument terminator\n%.98s"),
                                    nn->body);
              }
              nn->body[i++] = c;
              if (i>= size) nn->body = mrealloc(csound, nn->body, size += 100);
              if (c == '\n') {
                srccnt++;
              }
            }
            nn->body[i]='\0';
#ifdef MACDEBUG
            csound->Message(csound, "as...#%s#\n", nn->body);
#endif
            nn->acnt = 0;       /* No arguments for arguments */
            nn->next = ST(macros);
            ST(macros) = nn;
          }
        }
        cp--;                   /* Ignore $ sign */
        ST(input_cnt)++;
        if (ST(input_cnt) >= ST(input_size)) {
          ST(input_size) += 20;
          ST(inputs) = (IN_STACK*) mrealloc(csound, ST(inputs),
                                            ST(input_size) * sizeof(IN_STACK));
        }
        ST(str) = (IN_STACK*) ST(inputs) + (int) ST(input_cnt);
        ST(str)->string = 1; ST(str)->body = mm->body; ST(str)->args = mm->acnt;
        ST(str)->mac = mm;
        ST(str)->line = 1;
        ST(str)->unget_cnt = 0;
        ST(ingappop) = 1;
      }
    }
    if (cp >= endspace) {                   /* Ought to extend */
      csoundDie(csound, Str("file too large for ortext space"));
    }
    if (*(cp-1) != '\n')                    /* if no final NL,      */
      *cp++ = '\n';                         /*    add one           */
    else --lincnt;
    ST(linadr)[lincnt+1] = NULL;            /* terminate the adrs list */
    csound->Message(csound,Str("%d lines read\n"),lincnt);
    if (ST(fd) != NULL) {
      csound->FileClose(csound, ST(fd));    /* close the file       */
      ST(fd) = NULL;
    }
    ST(curline) = 0;                        /*   & reset to line 1  */
    while (ST(macros)) {                    /* Clear all macros */
      int i;
      mfree(csound, ST(macros)->body);
      mfree(csound, ST(macros)->name);
      for (i=0; i<ST(macros)->acnt; i++) mfree(csound, ST(macros)->arg[i]);
      ST(macros) = ST(macros)->next;
    }                                       /* nullist is a count only */
    ST(nullist) = (ARGLST *) mmalloc(csound, sizeof(ARGLST));
    ST(nullist)->count = 0;
    ST(nxtarglist) = (ARGLST*) mmalloc(csound, sizeof(ARGLST)
                                               + 200 * sizeof(char*));
}

static int splitline(ENVIRON *csound)
{                          /* split next orch line into atomic groups, count */
                           /*  labels this line, and set opgrpno where found */
    int     grpcnt, prvif, prvelsif, logical, condassgn, parens;
    int     c, collecting;
    char    *cp, *lp, *grpp = NULL;

    if (ST(collectbuf) == NULL)
      ST(collectbuf) = mcalloc(csound, ST(lenmax));
 nxtlin:
    if ((lp = ST(linadr)[++ST(curline)]) == NULL)   /* point at next line   */
      return 0;
    csound->DebugMsg(csound, Str("LINE %d:"), ST(curline));
    ST(linlabels) = ST(opgrpno) = 0;
    grpcnt = prvif = prvelsif = logical = condassgn = parens = collecting = 0;
    cp = ST(collectbuf);
    while ((c = *lp++) != '\n') {         /* for all chars this line:  */
      if (cp - ST(collectbuf) >= ST(lenmax)) {
        char  *nn;
        int   i;
        ST(lenmax) += LENMAX;
        nn = mrealloc(csound, ST(collectbuf), ST(lenmax));
        cp += (nn - ST(collectbuf));      /* Adjust pointer */
        /* Need to correct grp vector */
        for (i = 0; i < grpcnt; i++)
          ST(group)[i] += (nn - ST(collectbuf));
        ST(collectbuf) = nn;
      }
      if (c == '"') {                     /* quoted string: */
        if (grpcnt >= ST(grpmax)) {
          ST(grpmax) += GRPMAX;
          ST(group) = (char**) mrealloc(csound, ST(group), (ST(grpmax) + 1)
                                                           * sizeof(char*));
          ST(grpsav) = (char**) mrealloc(csound, ST(grpsav), (ST(grpmax) + 1)
                                                             * sizeof(char*));
        }
        grpp = ST(group)[grpcnt++] = cp;
        *cp++ = c;                        /*  cpy to nxt quote */
        while ((*cp++ = c = *lp++) != '"' && c != '\n');
        if (c == '\n')
          synterrp(csound, lp - 1, Str("unmatched quotes"));
        collecting = 1;                   /*   & resume chking */
        continue;
      }
      if (c == '{' && *lp == '{') {       /* multiline quoted string:    */
        if (grpcnt >= ST(grpmax)) {
          ST(grpmax) += GRPMAX;
          ST(group) = (char **) mrealloc(csound, ST(group), (ST(grpmax) + 1)
                                                            * sizeof(char*));
          ST(grpsav) = (char **) mrealloc(csound, ST(grpsav), (ST(grpmax) + 1)
                                                              * sizeof(char*));
        }
        grpp = ST(group)[grpcnt++] = cp;
        c = '"';                          /*  cpy to nxt quote */
        do {
          *cp++ = c;
          if (cp - ST(collectbuf) >= ST(lenmax)) {
            char  *nn;
            int   i;
            ST(lenmax) += LENMAX;
            nn = mrealloc(csound, ST(collectbuf), ST(lenmax));
            cp += (nn - ST(collectbuf));  /* Adjust pointer */
            /* Need to correct grp vector */
            for (i = 0; i < grpcnt; i++)
              ST(group)[i] += (nn - ST(collectbuf));
            ST(collectbuf) = nn;
          }
          c = *(++lp);
          if (c == '\n')
            ++ST(curline);
        } while (!(c == '}' && lp[1] == '}'));
        lp += 2;
        *cp++ = '"';
        collecting = 1;                   /*   & resume chking */
        continue;
      }
      if (c == ';') {
        while ((c = *lp++) != '\n');    /* comments:  gobble */
        break;                          /*    & exit linloop */
      }
      if (c == '/' && *lp == '*') { /* C Style comments */
        char *ll, *eol;
        ll= strstr(lp++, "*/");
      nxtl:
        eol = strchr(lp, '\n');
        if (eol!=NULL && eol<ll) {
          lp = ST(linadr)[++ST(curline)];
          ll= strstr(lp, "*/");
          goto nxtl;
        }
        if (ll == NULL) {
          synterrp(csound, lp - 2, Str("Unmatched comment"));
          lp = eol+1; break;
        }
        lp = ll+2;
        continue;
      }
      if (c == ' ' || c == '\t') {          /* spaces, tabs:      */
        if (!ST(opgrpno) && collecting) {   /*  those before args */
          *cp++ = '\0';                     /*  can be delimitrs  */
          collecting = 0;
          if (strcmp(grpp, "if") == 0) {    /* of if opcod */
            strcpy(grpp, "cggoto");         /* (replace) */
            cp = grpp + 7;
            prvif++;
          }
          else if (strcmp(grpp, "elseif") == 0) {       /* of elseif opcod */
            if (!ST(iflabels)) {    /* check to see we had an 'if' before  */
              synterr(csound, Str("invalid 'elseif' statement.  "
                                  "must have a corresponding 'if'"));
              goto nxtlin;
            }
            /* check to see we did not have an 'else' before */
            if (!ST(iflabels)->els[0]) {
              synterr(csound,
                      Str("'elseif' statement cannot occur after an 'else'"));
              goto nxtlin;
            }
            /* 'elseif' requires 2 additional lines */
            if (ST(repeatingElseifLine)) {
              /* add the 'elselabel' */
              ST(linlabels)++;
              strcpy(grpp, ST(iflabels)->els);
              cp = grpp + strlen(ST(iflabels)->els) + 1;
              /* finally replace the 'elseif' with a 'goto' */
              grpp = ST(group)[grpcnt++] = cp;
              strcpy(grpp, "cggoto");
              cp = grpp + 7;
              prvif++;
              prvelsif++;
              ST(repeatingElseifLine) = 0;
            }
            else {
              /* first add a 'goto endif' for the previous if */
              if (ST(iflabels)->ithen)
                strcpy(grpp, "goto");
              else
                strcpy(grpp, "kgoto");
              if (isopcod(csound, grpp))
                ST(opgrpno) = grpcnt;
              cp = grpp + 5;
              grpp = ST(group)[grpcnt++] = cp;
              strcpy(grpp, ST(iflabels)->end);
              cp += strlen(ST(iflabels)->end);
              ST(curline)--; /* roll back one and parse this line again */
              ST(repeatingElseifLine)++;
              ST(linopnum) = ST(opnum);     /* else save full line ops */
              ST(linopcod) = ST(opcod);
              return(grpcnt);
            }
          }
          if (isopcod(csound, grpp))        /*   or maybe others */
            ST(opgrpno) = grpcnt;
        }
        continue;                           /* now discard blanks*/
      }
      if (c == ':' && collecting && grpcnt == ST(linlabels)+1) {
        ST(linlabels)++;                    /* colon in 1st grps */
        *cp++ = '\0';                       /*  is also delimitr */
        collecting = 0;                     /*  (do not copy it) */
        continue;
      }
      if (c == '=' && !ST(opgrpno)) {       /* assign befor args */
        if (collecting)                     /* can be a delimitr */
          *cp++ = '\0';
        grpp = ST(group)[grpcnt++] = cp;    /* is itslf an opcod */
        *cp++ = c;
        *cp++ = '\0';
        isopcod(csound, grpp);
        ST(opgrpno) = grpcnt;
        collecting = 0;                     /* & self-delimiting */
        continue;
      }
      if (c == ',') {                       /* comma:            */
        if (!collecting)
          synterrp(csound, lp - 1, Str("misplaced comma"));
        if (parens) {
          synterrp(csound, lp - 2, Str("unbalanced parens"));
          parens = 0;
        }
        *cp++ = '\0';                       /*  terminate strng  */
        collecting = logical = condassgn = 0;
        continue;
      }
      if (prvif && collecting) {            /* for prev "if":    */
        if (strncmp(lp-1,"goto",4) == 0) {  /* if found "goto"   */
          *cp++ = '\0';                     /*      delimit cond */
          lp += 3;                          /*      & step over  */
          prvif = collecting = 0;
          continue;
        }
        else if ((c == 'i' || c == 'k') &&          /*  if preced i or k */
                 strncmp(lp, "goto", 4) == 0) {     /*  before "goto"    */
          *(ST(group)[ST(opgrpno) - 1] + 1) = c;    /*     modify cggoto */
          isopcod(csound, ST(group)[ST(opgrpno) - 1]);
          *cp++ = '\0';                             /*     then delimit  */
          lp += 4;                                  /*      etc          */
          prvif = collecting = 0;
          continue;
        }
        else if (strncmp(lp - 1, "then", 4) == 0) {
          struct iflabel *prv = ST(iflabels);
          /* modify cggoto */
          *(ST(group)[ST(opgrpno) - 1] + 1) = 'n';
          isopcod(csound, ST(group)[ST(opgrpno) - 1]);
          *cp++ = '\0';
          lp += 3;
          prvif = collecting = 0;
          grpp = ST(group)[grpcnt++] = cp;
          /* synthesize labels to represent an else and endif */
          if (prvelsif) { /* elseif, so we just need a new elselabel */
            sprintf(ST(iflabels)->els, "__else_%ld", ST(tempNum)++);
            prvelsif = 0;
          }
          else {
            /* this is a new if, so put a whole new label struct on the stack */
            ST(iflabels) = (struct iflabel *) mmalloc(csound,
                                                      sizeof(struct iflabel));
            ST(iflabels)->prv = prv;
            sprintf(ST(iflabels)->end, "__endif_%ld",ST(tempNum)++);
            sprintf(ST(iflabels)->els, "__else_%ld", ST(tempNum)++);
          }
          /* we set the 'goto' label to the 'else' label */
          strcpy(grpp, ST(iflabels)->els);
          /* set ithen flag */
          ST(iflabels)->ithen = 0;
          continue;
        }
        else if (strncmp(lp - 1, "ithen", 5) == 0) {
          struct iflabel *prv = ST(iflabels);
          /* modify cggoto */
          *(ST(group)[ST(opgrpno) - 1] + 1) = 'o';
          isopcod(csound, ST(group)[ST(opgrpno) - 1]);
          *cp++ = '\0';
          lp += 4;
          prvif = collecting = 0;
          grpp = ST(group)[grpcnt++] = cp;
          /* synthesize labels to represent an else and endif */
          if (prvelsif) { /* elseif, so we just need a new elselabel */
            sprintf(ST(iflabels)->els, "__else_%ld",ST(tempNum)++);
            prvelsif = 0;
          }
          else {
            /* this is a new if, so put a whole new label struct on the stack */
            ST(iflabels) = (struct iflabel *)mmalloc(csound,
                                                     sizeof(struct iflabel));
            ST(iflabels)->prv = prv;
            sprintf(ST(iflabels)->end, "__endif_%ld",ST(tempNum)++);
            sprintf(ST(iflabels)->els, "__else_%ld", ST(tempNum)++);
          }
          /* we set the 'goto' label to the 'else' label */
          strcpy(grpp, ST(iflabels)->els);
          /* set ithen flag */
          ST(iflabels)->ithen = 1;
          continue;
        }
      }
      if (!collecting++) {              /* remainder are     */
        if (grpcnt >= ST(grpmax)) {     /* collectable chars */
          ST(grpmax) += GRPMAX;
          ST(group) = (char**)  mcalloc(csound, (ST(grpmax)+1) * sizeof(char*));
          ST(grpsav) = (char**) mcalloc(csound, (ST(grpmax)+1) * sizeof(char*));
        }
        grpp = ST(group)[grpcnt++] = cp;
      }
      if (*lp == c && (c == '<' || c == '>')) {         /* <<, >> */
        lp++; *cp++ = c;
      }
      else if ((prvif || parens) && *lp == c && (c == '&' || c == '|')) {
        logical++; lp++; *cp++ = c;                     /* &&, || */
      }
      else if ((prvif || parens) &&
               (c == '!' || c == '<' || c == '=' || c == '>')) {
        logical++;                                      /* ==, !=, <=, >= */
      }
      else if (isalnum(c) ||            /* establish validity */
               c == '+' || c == '-' ||
               c == '*' || c == '/' ||
               c == '%' || c == '^' ||
               c == '&' || c == '|' || c == '#' ||      /* Bit operations */
               c == '\254' || c == '~' ||
               c == '.' || c == '_'
              )         /* allow uppercases and underscore in variables */
        ;
      else if (c == '(')
        parens++;                       /* and monitor function */
      else if (c == ')')
        --parens;
      else if (c == '?' && logical)
        condassgn++;
      else if (c == ':' && condassgn)
        ;
      else {
        sprintf(csound->errmsg, Str("illegal character %c"), c);
        synterrp(csound, lp - 1, csound->errmsg);
      }
      *cp++ = c;                        /* then collect the char   */
    }                                   /*  and loop for next      */
    if (grpp && grpcnt <= 1) {
      /* convert an 'else' statement into 2 lines
         goto <endiflabel>
         <elselabel>
         to do this, we parse the current twice */
      if (strncmp(grpp, "else", 4) == 0 &&
          ((int) ((char*) cp - (char*) grpp) == 4 ||
           strchr(" \t\r\n", grpp[4]) != NULL)) {
        if (!ST(iflabels)) {    /* 'else': check to see we had an 'if' before */
          synterr(csound, Str("invalid 'else' statement.  "
                              "must have a corresponding 'if'"));
          goto nxtlin;
        }
        if (ST(repeatingElseLine)) {        /* add the elselabel */
          if (!ST(iflabels)->els[0]) {
            /* check to see we had not another 'else' */
            synterr(csound, Str("duplicate 'else' statement"));
            goto nxtlin;
          }
          ST(linlabels)++;
          strcpy(grpp, ST(iflabels)->els);
          cp = grpp + strlen(ST(iflabels)->els);
          ST(iflabels)->els[0] = '\0';
          ST(repeatingElseLine) = 0;
        }
        else {                              /* add the goto statement */
          if (ST(iflabels)->ithen)
            strcpy(grpp, "goto");
          else
            strcpy(grpp, "kgoto");
          if (isopcod(csound, grpp))
            ST(opgrpno) = grpcnt;
          cp = grpp + 5;
          grpp = ST(group)[grpcnt++] = cp;
          strcpy(grpp, ST(iflabels)->end);
          cp += strlen(ST(iflabels)->end);
          ST(curline)--; /* roll back one and parse this line again */
          ST(repeatingElseLine) = 1;
        }
      }
      else if (strncmp(grpp, "endif", 5) == 0 &&
               ((int) ((char*) cp - (char*) grpp) == 5 ||
                strchr(" \t\r\n", grpp[5]) != NULL)) {
        /* replace 'endif' with the synthesized label */
        struct iflabel *prv;
        if (!ST(iflabels)) { /* check to see we had an 'if' before  */
          synterr(csound, Str("invalid 'endif' statement.  "
                              "must have a corresponding 'if'"));
          goto nxtlin;
        }
        if (ST(iflabels)->els[0]) {
          /* we had no 'else' statement, so we need to insert the elselabel */
          ST(linlabels)++;
          strcpy(grpp, ST(iflabels)->els);
          cp = grpp + strlen(ST(iflabels)->els);
          ST(iflabels)->els[0] = '\0';
          ST(curline)--; /* roll back one and parse this line again */
        }
        else {
          prv = ST(iflabels)->prv;
          ST(linlabels)++;
          strcpy(grpp, ST(iflabels)->end);
          cp = grpp + strlen(ST(iflabels)->end);
          mfree(csound, ST(iflabels));
          ST(iflabels) = prv;
        }
      }
    }
    if (!grpcnt)                        /* if line was trivial,    */
      goto nxtlin;                      /*      try another        */
    if (collecting) {                   /* if still collecting,    */
      *cp = '\0';                       /*      terminate          */
      if (!ST(opgrpno))                 /*      & chk for opcod    */
        if (isopcod(csound, grpp))
          ST(opgrpno) = grpcnt;
    }
    if (parens)                                   /* check balanced parens   */
      synterrp(csound, lp - 1, Str("unbalanced parens"));
    if (grpcnt > ST(linlabels) && !ST(opgrpno)) { /* if no full line opcod,  */
      synterr(csound, Str("no legal opcode"));    /*      complain &         */
      goto nxtlin;                                /*      try another        */
    }
    ST(linopnum) = ST(opnum);                     /* else save full line ops */
    ST(linopcod) = ST(opcod);
    if (csound->oparms->odebug) printgroups(csound, grpcnt);
    return grpcnt;
}

TEXT *getoptxt(ENVIRON *csound, int *init)
{                               /* get opcod and args from current line */
                                /*      returns pntr to a TEXT struct   */
    TEXT        *tp;
    char        c, d, str[32], *s;
    int         nn, incnt, outcnt;

    if (*init) {
      ST(grpcnt)   = 0;
      ST(nxtest)   = 1;
      ST(xprtstno) = 0;
      ST(polcnt)   = 0;
      ST(instrblk) = 0;
      ST(opcodblk) = 0;     /* IV - Sep 8 2002 */
      ST(instrcnt) = 0;
      *init    = 0;
      memset(&ST(optext),0,sizeof(TEXT));
    }

 tstnxt:
    tp = &ST(optext);
    if (ST(nxtest) >= ST(grpcnt)) {             /* if done with prevline, */
      csound->argcnt_offs = 0;          /* reset temporary variable index */
      if (!(ST(grpcnt) = splitline(csound)))    /*    attack next line    */
        return((TEXT *)0);                      /*    (else we're done)   */
      for (nn=0; nn<ST(grpcnt); nn++)           /*    save the group pntrs */
        ST(grpsav)[nn] = ST(group)[nn];
      ST(xprtstno) = ST(grpcnt) - 1;            /*    and reinit indices  */
      ST(nxtest) = 0;
      tp->linenum = ST(curline);
      /* IV - Jan 27 2005 */
      if (csound->oparms->expr_opt) {
        int i = (int) ST(linlabels) + 1;
        if (((int) ST(grpcnt) - i) > 0 && ST(group)[i][0] == '=' &&
            ST(group)[i][1] == '\0') {
          /* if opcode is '=', save outarg and type for expression optimiser */
          csound->opcode_is_assign = 1;
          csound->assign_type = (int) argtyp(csound, ST(group)[ST(linlabels)]);
          csound->assign_outarg = strsav_string(csound,
                                                ST(group)[ST(linlabels)]);
        }
        else {
          csound->opcode_is_assign = csound->assign_type = 0;
          csound->assign_outarg = NULL;
        }
      }
    }
    if (ST(linlabels)) {
      s = strsav_string(csound, ST(group)[ST(nxtest)]);
      lblfound(csound, s);
      tp->opnum = LABEL;
      tp->opcod = s;
      tp->inlist = tp->outlist = ST(nullist);
      ST(linlabels)--;
      ST(nxtest)++;
      return(tp);
    }
    if (!ST(instrcnt)) {                          /* send initial "instr 0"  */
      tp->opnum = INSTR;
      tp->opcod = strsav_string(csound, "instr"); /*  to hold global assigns */
      tp->outlist = ST(nullist);
      ST(nxtarglist)->count = 1;
      ST(nxtarglist)->arg[0] = strsav_string(csound, "0");
      tp->inlist = copy_arglist(csound, ST(nxtarglist));
      ST(instrcnt) = ST(instrblk) = 1;
      return(tp);
    }                                             /* then at 1st real INSTR, */
                                                  /*               or OPCODE, */
    if (ST(instrcnt) == 1 && ST(instrblk) &&
        (ST(opnum) == INSTR || ST(opnum) == OPCODE)) {
      tp->opnum = ENDIN;                          /*  send an endin to */
      tp->opcod = strsav_string(csound, "endin"); /*  term instr 0 blk */
      tp->outlist = tp->inlist = ST(nullist);
      ST(instrblk) = 0;
      ST(instrcnt) = 2;
      return(tp);
    }
    while (ST(xprtstno) >= 0) {             /* for each arg (last 1st):  */
      if (!ST(polcnt))
        /* if not midst of expressn: tst nxtarg */
        ST(polcnt) = express(csound, ST(group)[ST(xprtstno)--]);
      if (ST(polcnt) < 0) {
        /* polish but arg only: redo ptr & contin */
        ST(group)[ST(xprtstno)+1] = strsav_string(csound, csound->tokenstring);
        ST(polcnt) = 0;
      }
      else if (ST(polcnt)) {
        POLISH  *pol;                           /* for real polish ops, */
        int n;
        pol = &(csound->polish[--ST(polcnt)]);  /*    grab top one      */
        if (isopcod(csound, pol->opcod) == 0) { /* and check it out     */
          synterr(csound, Str("illegal opcod from expr anal"));
          goto tstnxt;
        }
        tp->opnum = ST(opnum);                  /* ok to send subop     */
        tp->opcod = strsav_string(csound, ST(opcod));
        ST(nxtarglist)->count = outcnt = 1;
        ST(nxtarglist)->arg[0] = strsav_string(csound, pol->arg[0]);
        tp->outlist = copy_arglist(csound, ST(nxtarglist));
        n = ST(nxtarglist)->count = incnt = pol->incount;
        do  ST(nxtarglist)->arg[n-1] = strsav_string(csound, pol->arg[n]);
        while (--n);
        tp->inlist = copy_arglist(csound, ST(nxtarglist));
        if (!ST(polcnt))                    /* last op? hit the grp ptr */
          ST(group)[ST(xprtstno)+1] = tp->outlist->arg[0];
        goto spctst;
      }
    }
    if (!strcmp(ST(linopcod), "=")) {       /* IV - Jan 08 2003: '=' opcode */
      if (csound->oparms->expr_opt && csound->opcode_is_assign < 0) {
        /* if optimised away, skip line */
        ST(nxtest) = ST(grpcnt); goto tstnxt;
      }
      if (ST(nxtest) <= ST(opgrpno) - 1) {
        c = argtyp(csound, ST(group)[ST(nxtest)]);
        switch (c) {
          case 'S': strcpy(str, "strcpy"); break;
          case 'a': c = argtyp(csound, ST(group)[ST(opgrpno)]);
                    strcpy(str, (c == 'a' ? "=.a" : "upsamp")); break;
          case 'p': c = 'i';
          default:  sprintf(str, "=.%c", c);
        }
        if (!(isopcod(csound, str))) {
          synterr(csound,
                  Str("failed to find %s, output arg '%s' illegal type"),
                  str, ST(group)[ST(nxtest)]);  /* report syntax error     */
          ST(nxtest) = 100;                     /* step way over this line */
          goto tstnxt;                          /* & go to next            */
        }
        if (strcmp(ST(group)[ST(nxtest)], ST(group)[ST(opgrpno)]) == 0) {
          /* outarg same as inarg, skip line */
          ST(nxtest) = ST(grpcnt); goto tstnxt;
        }
        ST(linopnum) = ST(opnum);
        ST(linopcod) = ST(opcod);
        csound->DebugMsg(csound, Str("modified opcod: %s"), ST(opcod));
      }
    }
    else if (ST(nxtest) <= ST(opgrpno) - 1) {
      /* Some aopcodes do not have ans!    */
      /* use outype to modify some opcodes */
      c = argtyp(csound, ST(group)[ST(nxtest)]);    /* Flagged as translating */
      if (csound->opcodlst[ST(linopnum)].dsblksiz == 0xffff ||
          (( strcmp(ST(linopcod), "table") == 0 ||  /*    with prefix   */
             strcmp(ST(linopcod), "tablei") == 0 ||
             strcmp(ST(linopcod), "table3") == 0 ||
             strcmp(ST(linopcod), "wrap") == 0 ||
             strcmp(ST(linopcod), "mirror") == 0) && (c == 'i' || c == 'p'))) {
        if (c == 'p')   c = 'i';
        if (c == '?')   c = 'a';                /* tmp */
        sprintf(str, "%s.%c", ST(linopcod), c);
        if (!(isopcod(csound, str))) {
          synterr(csound,
                  Str("failed to find %s, output arg '%s' illegal type"),
                  str, ST(group)[ST(nxtest)]);  /* report syntax error     */
          ST(nxtest) = 100;                     /* step way over this line */
          goto tstnxt;                          /* & go to next            */
        }
        ST(linopnum) = ST(opnum);
        ST(linopcod) = ST(opcod);
        csound->DebugMsg(csound, Str("modified opcod: %s"), ST(opcod));
      }
      else if (csound->opcodlst[ST(linopnum)].dsblksiz == 0xfffd) {
        if ((c = argtyp(csound, ST(group)[ST(opgrpno) ] )) != 'a') c = 'k';
        sprintf(str, "%s.%c", ST(linopcod), c);
        if (!(isopcod(csound, str))) {
          synterr(csound,
                  Str("failed to find %s, input arg '%s' illegal type"),
                  str, ST(group)[ST(opgrpno)]); /* report syntax error     */
          ST(nxtest) = 100;                     /* step way over this line */
          goto tstnxt;                          /* & go to next            */
        }
        ST(linopnum) = ST(opnum);
        ST(linopcod) = ST(opcod);
        csound->DebugMsg(csound, Str("modified opcod: %s"), ST(opcod));
      }
      else if (csound->opcodlst[ST(linopnum)].dsblksiz == 0xfffe) {
                                                /* Two tags for OSCIL's    */
        if ((c = argtyp(csound, ST(group)[ST(opgrpno) ] )) != 'a') c = 'k';
        if ((d = argtyp(csound, ST(group)[ST(opgrpno)+1])) != 'a') d = 'k';
        sprintf(str, "%s.%c%c", ST(linopcod), c, d);
        isopcod(csound, str);   /*  opcode with suffix */
        ST(linopnum) = ST(opnum);
        ST(linopcod) = ST(opcod);
        csound->DebugMsg(csound, Str("modified opcod: %s"), ST(opcod));
        c = argtyp(csound, ST(group)[ST(nxtest)]);  /* reset outype params */
      }                                 /* need we reset outype again here ? */
      else if (csound->opcodlst[ST(linopnum)].dsblksiz == 0xfffc) {
        /* For divz types */
        c = argtyp(csound, ST(group)[ST(opgrpno)  ]);
        d = argtyp(csound, ST(group)[ST(opgrpno)+1]);
        if ((c=='i' || c=='c') && (d=='i' || d=='c')) c='i',d = 'i';
        else {
          if (c != 'a') c = 'k';
          if (d != 'a') d = 'k';
        }
        sprintf(str,"divz.%c%c",c,d);
        isopcod(csound, str);   /*  opcode with suffix */
        ST(linopnum) = ST(opnum);
        ST(linopcod) = ST(opcod);
      }
    }
    tp->opnum = ST(linopnum);                         /* now use identified   */
    tp->opcod = strsav_string(csound, ST(linopcod));  /*   full line opcode   */
    /* IV - Oct 24 2002: check for invalid use of setksmps */
    if (!ST(opcodblk) && !strcmp(ST(linopcod), "setksmps"))
      synterr(csound, Str("setksmps is allowed only in user defined opcodes"));
    if (strncmp(ST(linopcod),"out",3) == 0 && /* but take case of MIDI ops */
        (ST(linopcod)[3] == '\0' || ST(linopcod)[3] == 's' ||
         ST(linopcod)[3] == 'q'  || ST(linopcod)[3] == 'h' ||
         ST(linopcod)[3] == 'o'  || ST(linopcod)[3] == 'x' ||
         ST(linopcod)[3] == '3'     ))
      if ((csound->tran_nchnls == 1  && strcmp(ST(linopcod),"out" ) != 0)    ||
          (csound->tran_nchnls == 2  && strncmp(ST(linopcod),"outs",4) != 0) ||
          (csound->tran_nchnls == 4  && strncmp(ST(linopcod),"outq",4) != 0) ||
          (csound->tran_nchnls == 6  && strncmp(ST(linopcod),"outh",4) != 0) ||
          (csound->tran_nchnls == 8  && strncmp(ST(linopcod),"outo",4) != 0) ||
          (csound->tran_nchnls == 16 && strncmp(ST(linopcod),"outx",4) != 0) ||
          (csound->tran_nchnls == 32 && strncmp(ST(linopcod),"out32",5) != 0)) {
        if      (csound->tran_nchnls == 1)  isopcod(csound, "out");
        else if (csound->tran_nchnls == 2)  isopcod(csound, "outs");
        else if (csound->tran_nchnls == 4)  isopcod(csound, "outq");
        else if (csound->tran_nchnls == 6)  isopcod(csound, "outh");
        else if (csound->tran_nchnls == 8)  isopcod(csound, "outo");
        else if (csound->tran_nchnls == 16) isopcod(csound, "outx");
        else if (csound->tran_nchnls == 32) isopcod(csound, "out32");
        csound->Message(csound, Str("%s inconsistent with global nchnls (%d); "
                                    "replaced with %s\n"),
                                ST(linopcod), csound->tran_nchnls, ST(opcod));
        tp->opnum = ST(linopnum) = ST(opnum);
        tp->opcod = strsav_string(csound, ST(linopcod) = ST(opcod));
      }
    incnt = outcnt = 0;
    while (ST(nxtest) < ST(opgrpno)-1)          /* create the out arglist  */
      ST(nxtarglist)->arg[outcnt++] =
        strsav_string(csound, ST(group)[ST(nxtest)++]);
    ST(nxtarglist)->count = outcnt;
    if (outcnt == 0)
      tp->outlist = ST(nullist);
    else {
      tp->outlist = copy_arglist(csound, ST(nxtarglist));   /* & prep ins */
    }
    ST(nxtest)++;
    while (ST(nxtest) < ST(grpcnt))             /*      & ensuing inargs  */
      ST(nxtarglist)->arg[incnt++] =
        strsav_string(csound, ST(group)[ST(nxtest)++]);
    ST(nxtarglist)->count = incnt;
    if (incnt==0)
      tp->inlist = ST(nullist);
    else tp->inlist = copy_arglist(csound, ST(nxtarglist));
    ST(grpcnt) = 0;                             /* all done w. these groups */

 spctst:
    tp->xincod_str = tp->xincod = 0;
    if (tp->opnum == OPCODE) {  /* IV - Sep 8 2002: added OPCODE and ENDOP */
      if (ST(opcodblk))
        synterr(csound, Str("opcode blks cannot be nested (missing 'endop'?)"));
      else if (ST(instrblk))
        synterr(csound, Str("opcode not allowed in instr block"));
      else ST(instrblk) = ST(opcodblk) = 1;
      resetouts(csound);                        /* reset #out counts */
      lblclear(csound);                         /* restart labelist  */
    }
    else if (tp->opnum == ENDOP) {      /* IV - Sep 8 2002:     ENDOP:  */
      lblchk(csound);                   /* chk missed labels */
      if (!ST(instrblk))
        synterr(csound, Str("unmatched endop"));
      else if (!ST(opcodblk))
        synterr(csound, Str("endop not allowed in instr block"));
      else ST(instrblk) = ST(opcodblk) = 0;
    }
    else if (tp->opnum == INSTR) {      /* IV - Sep 8 2002: for opcod INSTR  */
      if (ST(opcodblk))     /* IV - Sep 8 2002 */
        synterr(csound, Str("instr not allowed in opcode block"));
      else if (ST(instrblk))
        synterr(csound,
                Str("instr blocks cannot be nested (missing 'endin'?)"));
      else ST(instrblk) = 1;
      resetouts(csound);                        /* reset #out counts */
      lblclear(csound);                         /* restart labelist  */
    }
    else if (tp->opnum == ENDIN) {              /* ENDIN:       */
      lblchk(csound);                           /* chk missed labels */
      if (ST(opcodblk))
        synterr(csound, Str("endin not allowed in opcode blk"));
      else if (!ST(instrblk))
        synterr(csound, Str("unmatched endin"));
      else ST(instrblk) = 0;
    }
    else {                                      /* for all other opcodes:  */
      OENTRY    *ep = csound->opcodlst + tp->opnum;
      int       n, nreqd;
      char      tfound = '\0', treqd, *types = NULL;
      char      xtypes[OPCODENUMOUTS + 1];      /* IV - Oct 24 2002 */

      if (!ST(instrblk))
        synterr(csound, Str("misplaced opcode"));
      /* IV - Oct 24 2002: moved argument parsing for xout here */
      n = incnt;
      nreqd = -1;
      if (!strcmp(ep->opname, "xout")) {
        if (!ST(opcodblk))
          synterr(csound, Str("xout is allowed only in user defined opcodes"));
        else {
          /* IV - Oct 24 2002: opcodeInfo always points to the most recently */
          /* defined user opcode (or named instrument) structure; in this */
          /* case, it is the current opcode definition (not very elegant, */
          /* but works) */
          char *c = csound->opcodeInfo->outtypes;
          int i = 0;
          nreqd = csound->opcodeInfo->outchns;
          while (c[i]) {
            switch (c[i]) {
              case 'a':
              case 'k':
              case 'i': xtypes[i] = c[i]; break;
              case 'K': xtypes[i] = 'k';
            }
            i++;
          }
          xtypes[i] = '\0';
          types = &xtypes[0];
        }
      }
      if (nreqd < 0)    /* for other opcodes */
        nreqd = strlen(types = ep->intypes);
      if (n > nreqd) {                  /* IV - Oct 24 2002: end of new code */
        if ((treqd = types[nreqd-1]) == 'n') {  /* indef args: */
          if (!(incnt & 01))                    /* require odd */
            synterr(csound, Str("missing or extra arg"));
        }       /* IV - Sep 1 2002: added 'M' */
        else if (treqd != 'm' && treqd != 'z' && treqd != 'y' &&
                 treqd != 'Z' && treqd != 'M' && treqd != 'N') /* else any no */
          synterr(csound, Str("too many input args"));
      }
      else if (incnt < nreqd) {         /*  or set defaults: */
        do {
          switch (types[incnt]) {
          case 'k':             /* Will this work?  Doubtful code.... */
          case 'o': ST(nxtarglist)->arg[incnt++] = strsav_string(csound, "0");
            break;
          case 'p': ST(nxtarglist)->arg[incnt++] = strsav_string(csound, "1");
            break;
          case 'q': ST(nxtarglist)->arg[incnt++] = strsav_string(csound, "10");
            break;
          case 'v': ST(nxtarglist)->arg[incnt++] = strsav_string(csound, ".5");
            break;
          case 'h': ST(nxtarglist)->arg[incnt++] = strsav_string(csound, "127");
            break;
          case 'j': ST(nxtarglist)->arg[incnt++] = strsav_string(csound, "-1");
            break;
          case 'M':
          case 'N':
          case 'm': nreqd--;
            break;
          default:  synterr(csound, Str("insufficient required arguments"));
            goto chkin;
          }
        } while (incnt < nreqd);
        ST(nxtarglist)->count = n = incnt;          /*    in extra space */
        if (tp->inlist == ST(nullist) && incnt > 0) {
                                /*MWB 2/11/97 fixed bug that prevented an
                                  opcode with only optional arguments from
                                  properly loading defaults */
          tp->inlist = copy_arglist(csound, ST(nxtarglist));
        }
      }
    chkin:
      if (n>tp->inlist->count) {
        int i;
        size_t m = sizeof(ARGLST) + (n - 1) * sizeof(char*);
        tp->inlist = (ARGLST*) mrealloc(csound, tp->inlist, m);
        for (i=tp->inlist->count; i<n; i++) {
          tp->inlist->arg[i] = ST(nxtarglist)->arg[i];
        }
        tp->inlist->count = n;
      }
      while (n--) {                     /* inargs:   */
        long    tfound_m, treqd_m = 0L;
        s = tp->inlist->arg[n];
        if (n >= nreqd) {               /* det type required */
          switch (types[nreqd-1]) {
            case 'M':
            case 'N':
            case 'Z':
            case 'y':
            case 'z':   treqd = types[nreqd-1]; break;
            default:    treqd = 'i';    /*   (indef in-type) */
          }
        }
        else treqd = types[n];          /*       or given)   */
        if (treqd == 'l') {             /* if arg takes lbl  */
          csound->DebugMsg(csound, "treqd = l");
          lblrequest(csound, s);        /*      req a search */
          continue;                     /*      chk it later */
        }
        tfound = argtyp(csound, s);     /* else get arg type */
        /* IV - Oct 31 2002 */
        tfound_m = ST(typemask_tabl)[(unsigned char) tfound];
        if (!(tfound_m & (ARGTYP_c|ARGTYP_p)) && !ST(lgprevdef) && *s != '"') {
          synterr(csound, Str("input arg '%s' used before defined"), s);
        }
        csound->DebugMsg(csound, "treqd %c, tfound %c", treqd, tfound);
        if (tfound == 'a' && n < 31)    /* JMC added for FOG */
          /* 4 for FOF, 8 for FOG; expanded to 15  */
          tp->xincod |= (1 << n);
        if (tfound == 'S' && n < 31)
          tp->xincod_str |= (1 << n);
        /* IV - Oct 31 2002: simplified code */
        if (!(tfound_m & ST(typemask_tabl_in)[(unsigned char) treqd])) {
          /* check for exceptional types */
          switch (treqd) {
          case 'Z':                             /* indef kakaka ... */
            if (!(tfound_m & (n & 1 ? ARGTYP_a : ARGTYP_ipcrk)))
              intyperr(csound, n, tfound, treqd);
            break;
          case 'x':
            treqd_m = ARGTYP_ipcr;              /* also allows i-rate */
          case 's':                             /* a- or k-rate */
            treqd_m |= ARGTYP_a | ARGTYP_k;
            if (tfound_m & treqd_m) {
              if (tfound == 'a' && tp->outlist != ST(nullist)) {
                long outyp_m =                  /* ??? */
                  ST(typemask_tabl)[(unsigned char) argtyp(csound,
                                                       tp->outlist->arg[0])];
                if (outyp_m & (ARGTYP_a | ARGTYP_d | ARGTYP_w)) break;
              }
              else
                break;
            }
          default:
            intyperr(csound, n, tfound, treqd);
            break;
          }
        }
      }
      csound->DebugMsg(csound, "xincod = %d", tp->xincod);
      /* IV - Sep 1 2002: added 'X' type, and xoutcod */
      tp->xoutcod_str = tp->xoutcod = 0;
      /* IV - Oct 24 2002: moved argument parsing for xin here */
      n = outcnt;
      nreqd = -1;
      if (!strcmp(ep->opname, "xin")) {
        if (!ST(opcodblk))
          synterr(csound, Str("xin is allowed only in user opcodes"));
        else {
          /* IV - Oct 24 2002: opcodeInfo always points to the most recently */
          /* defined user opcode (or named instrument) structure; in this */
          /* case, it is the current opcode definition (not very elegant, */
          /* but works) */
          char *c = csound->opcodeInfo->intypes;
          int i = 0;
          nreqd = csound->opcodeInfo->inchns;
          while (c[i]) {
            switch (c[i]) {
              case 'a': xtypes[i] = c[i]; break;
              case 'k':
              case 'K': xtypes[i] = 'k'; break;
              default:  xtypes[i] = 'i';
            }
            i++;
          }
          xtypes[i] = '\0';
          types = &xtypes[0];
        }
      }
      if (nreqd < 0)    /* for other opcodes */
        nreqd = strlen(types = ep->outypes);
      if ((n != nreqd)                  /* IV - Oct 24 2002: end of new code */
          && ((*types != (char)'m' && *types != (char)'z' && *types != 'X')
              || !n || n > MAXCHNLS))
        synterr(csound, Str("illegal no of output args"));
      if (n > nreqd) n = nreqd; /* IV - Oct 24 2002: need this check */
      while (n--) {                                     /* outargs:  */
        long    tfound_m;       /* IV - Oct 31 2002 */
        s = tp->outlist->arg[n];
        treqd = types[n];
        tfound = argtyp(csound, s);                     /*  found    */
        /* IV - Oct 31 2002 */
        tfound_m = ST(typemask_tabl)[(unsigned char) tfound];
        /* IV - Sep 1 2002: xoutcod is the same as xincod for input */
        if (tfound == 'a' && n < 31)
          tp->xoutcod |= (1 << n);
        if (tfound == 'S' && n < 31)
          tp->xoutcod_str |= (1 << n);
        csound->DebugMsg(csound, "treqd %c, tfound %c", treqd, tfound);
        if (tfound_m & (ARGTYP_d | ARGTYP_w))
          if (ST(lgprevdef)) {
            synterr(csound, Str("output name previously used, "
                                "type '%c' must be uniquely defined"), tfound);
          }
        /* IV - Oct 31 2002: simplified code */
        if (!(tfound_m & ST(typemask_tabl_out)[(unsigned char) treqd])) {
          synterr(csound, Str("output arg '%s' illegal type"), s);
        }
      }
      if (incnt) {
        if (ep->intypes[0] != 'l')      /* intype defined by 1st inarg */
          tp->intype = argtyp(csound, tp->inlist->arg[0]);
        else tp->intype = 'l';          /*   (unless label)  */
      }
      if (outcnt)                       /* pftype defined by outarg */
        tp->pftype = tfound;
      else tp->pftype = tp->intype;     /*    else by 1st inarg     */
    }
    return(tp);                         /* return the text blk */
}

static void intyperr(ENVIRON *csound, int n, char tfound, char expect)
{
    char    *s = ST(grpsav)[ST(opgrpno) + n];
    char    t[10];

    switch (tfound) {
    case 'd':
    case 'w':
    case 'f':
    case 'a':
    case 'k':
    case 'i':
    case 'p': t[0] = tfound;
              t[1] = '\0';
              break;
    case 'r':
    case 'c': strcpy(t,"const");
              break;
    case 'S': strcpy(t,"string");
              break;
    case 'b':
    case 'B': strcpy(t,"boolean");
              break;
    case '?': strcpy(t,"?");
              break;
    }
    synterr(csound, Str("input arg '%s' of type %s "
                        "not allowed when expecting %c"), s, t, expect);
}

static int isopcod(ENVIRON *csound, char *s)
{                               /* tst a string against opcodlst  */
    int     n;                  /*   & set op carriers if matched */

    if (!(n = find_opcode(csound, s))) return (0);      /* IV - Oct 31 2002 */
    ST(opnum) = n;                          /* on corr match,   */
    ST(opcod) = csound->opcodlst[n].opname; /*  set op carriers */

    return(1);                              /*  & report success */
}

int getopnum(ENVIRON *csound, char *s)
{                               /* tst a string against opcodlst  */
    int     n;                  /*   & return with opnum          */

    if ((n = find_opcode(csound, s))) return n;  /* IV - Oct 31 2002 */
    csound->Message(csound,"opcode=%s\n", s);
    csound->Die(csound, Str("unknown opcode"));
    return NOTOK;
}

static int pnum(char *s)        /* check a char string for pnum format  */
                                /*   and return the pnum ( >= 0 )       */
{                               /* else return -1                       */
    int n;

    if (*s == 'p' || *s == 'P')
      if (sscanf(++s, "%d", &n))
        return(n);
    return(-1);
}

char argtyp(ENVIRON *csound, char *s)
{                       /* find arg type:  d, w, a, k, i, c, p, r, S, B, b */
    char c = *s;        /*   also set lgprevdef if !c && !p && !S */

    /*trap this before parsing for a number! */
    /* two situations: defined at header level: 0dbfs = 1.0
     *  and returned as a value:  idb = 0dbfs
     */
    if ((c >= '1' && c <= '9') || c == '.' || c == '-' || c == '+' ||
        (c == '0' && strcmp(s, "0dbfs") != 0))
      return('c');                              /* const */
    if (pnum(s) >= 0)
      return('p');                              /* pnum */
    if (c == '"')
      return('S');                              /* quoted String */
    ST(lgprevdef) = lgexist(csound, s);               /* (lgprev) */
    if (strcmp(s,"sr") == 0    || strcmp(s,"kr") == 0 ||
        strcmp(s,"0dbfs") == 0 ||
        strcmp(s,"ksmps") == 0 || strcmp(s,"nchnls") == 0)
      return('r');                              /* rsvd */
    if (c == 'd' || c == 'w') /* N.B. d,w NOT YET #TYPE OR GLOBAL */
      return(c);
    if (c == '#')
      c = *(++s);
    if (c == 'g')
      c = *(++s);
    if (strchr("akiBbfS", c) != NULL)
      return(c);
    else return('?');
}

static void lblclear(ENVIRON *csound)
{
    ST(lblcnt) = 0;
}

static void lblrequest(ENVIRON *csound, char *s)
{
    int     req;

    for (req=0; req<ST(lblcnt); req++)
      if (strcmp(ST(lblreq)[req].label,s) == 0)
        return;
    if (++ST(lblcnt) >= ST(lblmax)) {
      LBLREQ *tmp;
      ST(lblmax) += LBLMAX;
      tmp = mrealloc(csound, ST(lblreq), ST(lblmax) * sizeof(LBLREQ));
      ST(lblreq) = tmp;
    }
    ST(lblreq)[req].reqline = ST(curline);
    ST(lblreq)[req].label =s;
}

static void lblfound(ENVIRON *csound, char *s)
{
    int     req;

    for (req=0; req<ST(lblcnt); req++ )
      if (strcmp(ST(lblreq)[req].label,s) == 0) {
        if (ST(lblreq)[req].reqline == 0)
          synterr(csound, Str("duplicate label"));
        goto noprob;
      }
    if (++ST(lblcnt) >= ST(lblmax)) {
      LBLREQ *tmp;
      ST(lblmax) += LBLMAX;
      tmp = mrealloc(csound, ST(lblreq), ST(lblmax) * sizeof(LBLREQ));
      ST(lblreq) = tmp;
    }
    ST(lblreq)[req].label = s;
noprob:
    ST(lblreq)[req].reqline = 0;
}

static void lblchk(ENVIRON *csound)
{
    int req;
    int n;

    for (req=0; req<ST(lblcnt); req++ )
      if ((n = ST(lblreq)[req].reqline)) {
        char    *s;
        csound->Message(csound, Str("error line %d.  unknown label:\n"), n);
        s = ST(linadr)[n];
        do {
          csound->Message(csound, "%c", *s);
        } while (*s++ != '\n');
        csound->synterrcnt++;
      }
}

void synterr(ENVIRON *csound, const char *s, ...)
{
    va_list args;
    char    *cp;
    int     c;

    csound->MessageS(csound, CSOUNDMSG_ERROR, Str("error:  "));
    va_start(args, s);
    csound->MessageV(csound, CSOUNDMSG_ERROR, s, args);
    va_end(args);
    if ((cp = ST(linadr)[ST(curline)]) != NULL) {
      csound->MessageS(csound, CSOUNDMSG_ERROR,
                               Str(", line %d:\n"), ST(curline));
      do {
        csound->MessageS(csound, CSOUNDMSG_ERROR, "%c", (c = *cp++));
      } while (c != '\n');
    }
    else {
      csound->MessageS(csound, CSOUNDMSG_ERROR, "\n");
    }
    csound->synterrcnt++;
}

static void synterrp(ENVIRON *csound, const char *errp, char *s)
{
    char    *cp;

    synterr(csound, s);
    cp = ST(linadr)[ST(curline)];
    while (cp < errp) {
      int ch = *cp++;
      if (ch != '\t') ch = ' ';
      csound->MessageS(csound, CSOUNDMSG_ERROR, "%c", ch);
    }
    csound->MessageS(csound, CSOUNDMSG_ERROR, "^\n");
}

static void lexerr(ENVIRON *csound, char *s)
{
    IN_STACK  *curr = ST(str);

    csound->MessageS(csound, CSOUNDMSG_ERROR, Str("error:  %s\n"), s);
    while (curr != ST(inputs)) {
      if (curr->string) {
        MACRO *mm = ST(macros);
        while (mm != curr->mac) mm = mm->next;
        csound->MessageS(csound, CSOUNDMSG_ERROR,
                                 Str("called from line %d of macro %s\n"),
                                 curr->line, mm->name);
      }
      else {
        csound->MessageS(csound, CSOUNDMSG_ERROR,
                                 Str("in line %d of file input %s\n"),
                                 curr->line, curr->body);
      }
      curr--;
    }
    longjmp(csound->exitjmp, 1);
}

static void printgroups(ENVIRON *csound, int grpcnt)
{                                       /*   debugging aid (onto stdout) */
    char    c, *cp = ST(group)[0];

    csound->Message(csound, "groups:\t");
    while (grpcnt--) {
      csound->Message(csound, "%s ", cp);
      while ((c = *cp++));
    }
    csound->Message(csound, "\n");
}

