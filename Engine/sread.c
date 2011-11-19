/*
    sread.c:

    Copyright (C) 1991, 1997 Barry Vercoe, John ffitch

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

#include "csoundCore.h"                             /*   SREAD.C     */
#include <math.h>      /* for fabs() */
#include <ctype.h>
#include "namedins.h"           /* IV - Oct 31 2002 */
#include "corfile.h"

#define MEMSIZ  16384           /* size of memory requests from system  */
#define MARGIN  4096            /* minimum remaining before new request */
#define NAMELEN 40              /* array size of repeat macro names */
#define RPTDEPTH 40             /* size of repeat_n arrays (39 loop levels) */

#define MARGS   (3)

typedef struct MACRO {          /* To store active macros */
    char        *name;          /* Use is by name */
    int         acnt;           /* Count of arguments */
    char        *body;          /* The text of the macro */
    struct MACRO *next;         /* Chain of active macros */
    int         margs;          /* ammount of space for args */
    char        *arg[MARGS];    /* With these arguments */
} MACRO;

typedef struct in_stack_s {     /* Stack of active inputs */
    int16 string;               /* Flag to say if string or file */
    int16 is_marked_repeat;     /* 1 if this input created by 'n' statement */
    int16 args;                 /* Argument count for macro */
    char  *body;                /* String */
    FILE  *file;                /* File case only */
    void  *fd;
    MACRO *mac;
    int   line;
    int   unget_cnt;
    char  unget_buf[128];
} IN_STACK;

typedef struct marked_sections {
  char  *name;
  int32  posit;
  int   line;
  char  *file;
} MARKED_SECTIONS;

typedef struct {
    SRTBLK  *bp, *prvibp;           /* current srtblk,  prev w/same int(p1) */
    char    *sp, *nxp;              /* string pntrs into srtblk text        */
    int     op;                     /* opcode of current event              */
    int     warpin;                 /* input format sensor                  */
    int     linpos;                 /* line position sensor                 */
    int     lincnt;                 /* count of lines/section in scorefile  */
    MYFLT   prvp2 /* = -FL(1.0) */;     /* Last event time                  */
    MYFLT   clock_base /* = FL(0.0) */;
    MYFLT   warp_factor /* = FL(1.0) */;
    char    *curmem;
    char    *memend;                /* end of cur memblk                    */
    MACRO   *macros;
    int     next_name /* = -1 */;
    IN_STACK  *inputs, *str;
    int     input_size, input_cnt;
    int     pop;                    /* Number of macros to pop              */
    int     ingappop /* = 1 */;     /* Are we in a popable gap?             */
    int     linepos /* = -1 */;
    MARKED_SECTIONS names[30], *current_name;
    char    repeat_name_n[RPTDEPTH][NAMELEN];
    int     repeat_cnt_n[RPTDEPTH];
    int32    repeat_point_n[RPTDEPTH];
    int     repeat_inc_n /* = 1 */;
    MACRO   *repeat_mm_n[RPTDEPTH];
    int     repeat_index;
    /* Variable for repeat sections */
    char    repeat_name[NAMELEN];
    int     repeat_cnt;
    int32    repeat_point;
    int     repeat_inc /* = 1 */;
    MACRO   *repeat_mm;
} SREAD_GLOBALS;

static void print_input_backtrace(CSOUND *csound, int needLFs,
                                  void (*msgfunc)(CSOUND*, const char*, ...));
static  void    copylin(CSOUND *), copypflds(CSOUND *);
static  void    ifa(CSOUND *), setprv(CSOUND *);
static  void    carryerror(CSOUND *), pcopy(CSOUND *, int, int, SRTBLK*);
static  void    salcinit(CSOUND *);
static  void    salcblk(CSOUND *), flushlin(CSOUND *);
static  int     getop(CSOUND *), getpfld(CSOUND *);
        MYFLT   stof(CSOUND *, char *);
extern  void    *fopen_path(CSOUND *, FILE **, char *, char *, char *, int);

#define ST(x)   (((SREAD_GLOBALS*) csound->sreadGlobals)->x)

static void sread_alloc_globals(CSOUND *csound)
{
    if (LIKELY(csound->sreadGlobals != NULL))
      return;
    csound->sreadGlobals = (SREAD_GLOBALS*)
                                csound->Calloc(csound, sizeof(SREAD_GLOBALS));
    ST(prvp2) = -FL(1.0);
    ST(clock_base) = FL(0.0);
    ST(warp_factor) = FL(1.0);
    ST(next_name) = -1;
    ST(ingappop) = 1;
    ST(linepos) = -1;
    ST(repeat_inc_n) = 1;
    ST(repeat_inc) = 1;
}

static intptr_t expand_nxp(CSOUND *csound)
{
    char      *oldp;
    SRTBLK    *p;
    intptr_t  offs;
    size_t    nbytes;

    if (UNLIKELY(ST(nxp) >= (ST(memend) + MARGIN))) {
      csound->Die(csound, Str("sread:  text space overrun, increase MARGIN"));
      return 0;     /* not reached */
    }
    /* calculate the number of bytes to allocate */
    nbytes = (size_t) (ST(memend) - ST(curmem));
    nbytes = nbytes + (nbytes >> 3) + (size_t) (MEMSIZ - 1);
    nbytes &= ~((size_t) (MEMSIZ - 1));
    /* extend allocated memory */
    oldp = ST(curmem);
    ST(curmem) = (char*) csound->ReAlloc(csound, ST(curmem),
                                                 nbytes + (size_t) MARGIN);
    ST(memend) = (char*) ST(curmem) + (int32) nbytes;
    /* did the pointer change ? */
    if (ST(curmem) == oldp)
      return (intptr_t) 0;      /* no, nothing to do */
    /* correct all pointers for the change */
    offs = (intptr_t) ((uintptr_t) ST(curmem) - (uintptr_t) oldp);
    if (ST(bp) != NULL)
      ST(bp) = (SRTBLK*) ((uintptr_t) ST(bp) + (intptr_t) offs);
    if (ST(prvibp) != NULL)
      ST(prvibp) = (SRTBLK*) ((uintptr_t) ST(prvibp) + (intptr_t) offs);
    if (ST(sp) != NULL)
      ST(sp) = (char*) ((uintptr_t) ST(sp) + (intptr_t) offs);
    if (ST(nxp) != NULL)
      ST(nxp) = (char*) ((uintptr_t) ST(nxp) + (intptr_t) offs);
    if (csound->frstbp == NULL)
      return offs;
    p = csound->frstbp;
    csound->frstbp = p = (SRTBLK*) ((uintptr_t) p + (intptr_t) offs);
    do {
      if (p->prvblk != NULL)
        p->prvblk = (SRTBLK*) ((uintptr_t) p->prvblk + (intptr_t) offs);
      if (p->nxtblk != NULL)
        p->nxtblk = (SRTBLK*) ((uintptr_t) p->nxtblk + (intptr_t) offs);
      p = p->nxtblk;
    } while (p != NULL);
    /* return pointer change in bytes */
    return offs;
}

/* sreaderr() - for non-fatal "warnings" */
static void sreaderr(CSOUND *csound, const char *s, ...)
{
    va_list   args;

    csoundMessage(csound, Str("sread: "));
    va_start(args, s);
    csoundMessageV(csound, 0, s, args);
    va_end(args);
    csoundMessage(csound, "\n");
    print_input_backtrace(csound, 1, csoundMessage);
    return;
}

/* scorerr() - for fatal errors in score parsing */
static void scorerr(CSOUND *csound, const char *s, ...)
{
    va_list   args;

    va_start(args, s);
    csound->ErrMsgV(csound, Str("score error:  "), s, args);
    va_end(args);
    print_input_backtrace(csound, 0, csoundErrorMsg);
    csound->LongJmp(csound, 1);
}

static void print_input_backtrace(CSOUND *csound, int needLFs,
                                  void (*msgfunc)(CSOUND*, const char*, ...))
{
    IN_STACK  *curr = ST(str);
    char      *m, *lf = (needLFs ? "\n" : "");
    int       lastinput = 0;
    int       lastsource = 2; /* 2=current file, 1=macro, 0=#include */

    msgfunc(csound, Str("  section %d:  at position %d%s"), csound->sectcnt,
                    ST(linepos), lf);

    do {
      if (curr == ST(inputs)) lastinput = 1;
      if (curr->string) {  /* macro input */
        if (UNLIKELY(!curr->mac || !curr->mac->name))
          csoundDie(csound, Str("Internal error in print_input_backtrace()"));
        switch(lastsource) {
        case 0: m = Str("  included from line %d of macro %s%s"); break;
        case 1: m = Str("  called from line %d of macro %s%s"); break;
        default:
        case 2: m = Str("  in line %d of macro %s%s"); break;
        }
        msgfunc(csound, m, (lastsource == 0 ? curr->line - 1 : curr->line),
                curr->mac->name, lf);  /* #include is one line before */
      }
      else {  /* file input */
        switch(lastsource) {
        case 0: m = Str("  included from line %d of file input %s%s"); break;
        case 1: m = Str("  called from line %d of file input %s%s"); break;
        default:
        case 2: m = Str("  in line %d of file input %s%s"); break;
        }
        if (lastinput && csound->oparms->useCsdLineCounts && csound->csdname) {
          /* print name & line # of CSD instead of temp sco */
          msgfunc(csound, m,
                  (lastsource == 0 ? csound->scoLineOffset + curr->line - 1 :
                   csound->scoLineOffset + curr->line), csound->csdname, lf);
        }
        else {
          msgfunc(csound, m, (lastsource == 0 ? curr->line - 1 : curr->line),
                  curr->body, lf);  /* #include is one line before */
        }
      }
      lastsource = curr->string;
      curr--;
    }
    while (!lastinput);
    return;
}

static MYFLT operate(CSOUND *csound, MYFLT a, MYFLT b, char c)
{
    MYFLT ans;
    extern MYFLT MOD(MYFLT,MYFLT);

    switch (c) {
    case '+': ans = a + b; break;
    case '-': ans = a - b; break;
    case '*': ans = a * b; break;
    case '/': ans = a / b; break;
    case '%': ans = MOD(a, b); break;
    case '^': ans = (MYFLT) pow((double) a, (double) b); break;
    case '&': ans = (MYFLT) (MYFLT2LRND(a) & MYFLT2LRND(b)); break;
    case '|': ans = (MYFLT) (MYFLT2LRND(a) | MYFLT2LRND(b)); break;
    case '#': ans = (MYFLT) (MYFLT2LRND(a) ^ MYFLT2LRND(b)); break;
    default:
      csoundDie(csound, Str("Internal error op=%c"), c);
      ans = FL(0.0);    /* compiler only */
    }
    return ans;
}

static int undefine_score_macro(CSOUND *csound, const char *name)
{
    MACRO *mm, *nn;
    int   i;

    if (strcmp(name, ST(macros)->name) == 0) {
      mm = ST(macros)->next;
      mfree(csound, ST(macros)->name);
      mfree(csound, ST(macros)->body);
      for (i = 0; i < ST(macros)->acnt; i++)
        mfree(csound, ST(macros)->arg[i]);
      mfree(csound, ST(macros));
      ST(macros) = mm;
    }
    else {
      mm = ST(macros);
      nn = mm->next;
      while (strcmp(name, nn->name) != 0) {
        mm = nn; nn = nn->next;
        if (UNLIKELY(nn == NULL)) {
          scorerr(csound, Str("Undefining undefined macro"));
          return -1;
        }
      }
      mfree(csound, nn->name);
      mfree(csound, nn->body);
      for (i = 0; i < nn->acnt; i++)
        mfree(csound, nn->arg[i]);
      mm->next = nn->next;
      mfree(csound, nn);
    }
    return 0;
}

static inline int isNameChar(int c, int pos)
{
    c = (int) ((unsigned char) c);
    return (isalpha(c) || (pos && (c == '_' || isdigit(c))));
}

/* Functions to read/unread chracters from
 * a stack of file and macro inputs */

static inline void ungetscochar(CSOUND *csound, int c)
{
    if (LIKELY(ST(str)->unget_cnt < 128))
      ST(str)->unget_buf[ST(str)->unget_cnt++] = (char) c;
    else
      csoundDie(csound, Str("ungetscochar(): buffer overflow"));
}

static int getscochar(CSOUND *csound, int expand)
{                   /* Read a score character, expanding macros if flag set */
    int     c;
 top:
    if (ST(str)->unget_cnt) {
      c = (int) ((unsigned char) ST(str)->unget_buf[--ST(str)->unget_cnt]);
      if (c == '\n')
        ST(linepos) = -1;
      return c;
    }
    else if (ST(str)->string) {
      c = *ST(str)->body++;
      if (c == '\0') {
        if (ST(str) == &ST(inputs)[0]) {
          ST(str)->body--;      /* to ensure repeated EOF */
          return EOF;
        }
        ST(pop) += ST(str)->args;
        ST(str)--; ST(input_cnt)--;
        goto top;
      }
    }
    else {
      c = getc(ST(str)->file);
      if (UNLIKELY(c == EOF)) {
        if (ST(str) == &ST(inputs)[0])
          return EOF;
        if (ST(str)->fd != NULL) {
          csound->FileClose(csound, ST(str)->fd); ST(str)->fd = NULL;
        }
        ST(str)--; ST(input_cnt)--; goto top;
      }
    }
    if (c == '\r') {    /* can only occur in files, and not in macros */
      if ((c = getc(ST(str)->file)) != '\n')
        ungetc(c, ST(str)->file);   /* For macintosh */
      c = '\n';
    }
    if (c == '\n') {
      ST(str)->line++; ST(linepos) = -1;
    }
    else ST(linepos)++;
    if (ST(ingappop) && ST(pop)) {
      do {
        if (ST(macros) != NULL) {
#ifdef MACDEBUG
          csound->Message(csound,"popping %s\n", ST(macros)->name);
#endif
          undefine_score_macro(csound, ST(macros)->name);
        }
        ST(pop)--;
      } while (ST(pop));
    }
    if (c == '$' && expand) {
      char      name[100];
      unsigned int i = 0;
      int       j;
      MACRO     *mm, *mm_save = NULL;
      ST(ingappop) = 0;
      while (isNameChar((c = getscochar(csound, 1)), (int) i)) {
        name[i++] = c; name[i] = '\0';
        mm = ST(macros);
        while (mm != NULL) {    /* Find the definition */
          if (!(strcmp(name, mm->name))) {
            mm_save = mm;       /* found a match, save it */
            break;
          }
          mm = mm->next;
        }
      }
      mm = mm_save;
      if (UNLIKELY(mm == NULL)) {
        if (!i)
          scorerr(csound, Str("Macro expansion symbol ($) without macro name"));
        else
          scorerr(csound, Str("Undefined macro: '%s'"), name);
      }
      if (strlen(mm->name) != i) {
        int cnt = (int) i - (int) strlen(mm->name);
        csound->Warning(csound, Str("$%s matches macro name $%s"),
                                name, mm->name);
        do {
          ungetscochar(csound, c);
          c = name[--i];
        } while (cnt--);
      }
      else if (c != '.')
        ungetscochar(csound, c);
#ifdef MACDEBUG
      csound->Message(csound, "Found macro %s required %d arguments\n",
                              mm->name, mm->acnt);
#endif
                                /* Should bind arguments here */
                                /* How do I recognise entities?? */
      if (mm->acnt) {
        if (UNLIKELY((c=getscochar(csound, 1)) != '('))
          scorerr(csound, Str("Syntax error in macro call"));
        for (j = 0; j < mm->acnt; j++) {
          char term = (j == mm->acnt - 1 ? ')' : '\'');
          char trm1 = (j == mm->acnt - 1 ? ')' : '#');
          MACRO* nn = (MACRO*) mmalloc(csound, sizeof(MACRO));
          unsigned int size = 100;
          nn->name = mmalloc(csound, strlen(mm->arg[j])+1);
          strcpy(nn->name, mm->arg[j]);
#ifdef MACDEBUG
          csound->Message(csound,"defining argument %s ", nn->name);
#endif
          i = 0;
          nn->body = (char*) mmalloc(csound, 100);
          while ((c = getscochar(csound, 1))!= term && c != trm1) {
            nn->body[i++] = c;
            if (i>= size) nn->body = mrealloc(csound, nn->body, size += 100);
          }
          nn->body[i]='\0';
#ifdef MACDEBUG
          csound->Message(csound,"as...#%s#\n", nn->body);
#endif
          nn->acnt = 0; /* No arguments for arguments */
          nn->next = ST(macros);
          ST(macros) = nn;
        }
      }
      ST(input_cnt)++;
      if (UNLIKELY(ST(input_cnt)>=ST(input_size))) {
        int old = ST(str)-ST(inputs);
        ST(input_size) += 20;
        ST(inputs) = mrealloc(csound, ST(inputs), ST(input_size)
                                                  * sizeof(IN_STACK));
        ST(str) = &ST(inputs)[old];     /* In case it moves */
      }
      ST(str)++;
      ST(str)->string = 1; ST(str)->body = mm->body; ST(str)->args = mm->acnt;
      ST(str)->is_marked_repeat = 0;
      ST(str)->mac = mm; ST(str)->line = 1; ST(str)->unget_cnt = 0;
#ifdef MACDEBUG
      csound->Message(csound,
                      "Macro %s definded as >>%s<<\n", mm->name, mm->body);
#endif
      ST(ingappop) = 1;
      goto top;
    }
/* End of macro expander */
    if (expand && c == '[') {           /* Evaluable section */
      char  stack[30];
      MYFLT vv[30];
      char  *op = stack - 1;
      MYFLT *pv = vv - 1;
      char  buffer[100];
      int   i;
      int   type = 0;  /* 1 -> expecting binary operator,')', or ']'; else 0 */
      *++op = '[';
      c = getscochar(csound, 1);
      do {
        switch (c) {
        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
        case '.':
          if (UNLIKELY(type)) {
            scorerr(csound, Str("illegal placement of number in [] "
                                "expression"));
          }
 parseNumber:
          i = 0;
          do {
            buffer[i++] = c;
            c = getscochar(csound, 1);
          } while (isdigit(c) || c == '.');
          if (c == 'e' || c == 'E') {
            buffer[i++] = c;
            c = getscochar(csound, 1);
            if (c == '+' || c == '-') {
              buffer[i++] = c;
              c = getscochar(csound, 1);
            }
            while (isdigit(c)) {
              buffer[i++] = c;
              c = getscochar(csound, 1);
            }
          }
          buffer[i] = '\0';
          *++pv = stof(csound, buffer);
          type = 1;
          break;
        case '~':
          if (UNLIKELY(type)) {
            scorerr(csound, Str("illegal placement of operator ~ in [] "
                                "expression"));
          }
          *++pv = (MYFLT) (csound->Rand31(&(csound->randSeed1)) - 1)
                  / FL(2147483645);
          type = 1;
          c = getscochar(csound, 1);
          break;
        case '@':
          if (UNLIKELY(type)) {
            scorerr(csound, Str("illegal placement of operator @ or @@ in"
                                " [] expression"));
          }
          {
            int n = 0;
            int k = 0;          /* 0 or 1 depending on guard bit */
            c = getscochar(csound, 1);
            if (c=='@') { k = 1; c = getscochar(csound, 1);}
            while (isdigit(c)) {
              n = 10*n + c - '0';
              c = getscochar(csound, 1);
            }
            i = 1;
            while (i<=n-k && i< 0x4000000) i <<= 1;
            *++pv = (MYFLT)(i+k);
            type = 1;
          }
          break;
        case '+': case '-':
          if (!type)
            goto parseNumber;
          if (*op != '[' && *op != '(') {
            MYFLT v = operate(csound, *(pv-1), *pv, *op);
            op--; pv--;
            *pv = v;
          }
          type = 0;
          *++op = c; c = getscochar(csound, 1); break;
        case '*':
        case '/':
        case '%':
          if (UNLIKELY(!type)) {
            scorerr(csound, Str("illegal placement of operator %c in [] "
                                "expression"), c);
          }
          if (*op == '*' || *op == '/' || *op == '%') {
            MYFLT v = operate(csound, *(pv-1), *pv, *op);
            op--; pv--;
            *pv = v;
          }
          type = 0;
          *++op = c; c = getscochar(csound, 1); break;
        case '&':
        case '|':
        case '#':
          if (UNLIKELY(!type)) {
            scorerr(csound, Str("illegal placement of operator %c in [] "
                                "expression"), c);
          }
          if (*op == '|' || *op == '&' || *op == '#') {
            MYFLT v = operate(csound, *(pv-1), *pv, *op);
            op--; pv--;
            *pv = v;
          }
          type = 0;
          *++op = c; c = getscochar(csound, 1); break;
        case '(':
          if (UNLIKELY(type)) {
            scorerr(csound, Str("illegal placement of '(' in [] expression"));
          }
          type = 0;
          *++op = c; c = getscochar(csound, 1); break;
        case ')':
          if (UNLIKELY(!type)) {
            scorerr(csound, Str("missing operand before ')' in [] expression"));
          }
          while (*op != '(') {
            MYFLT v = operate(csound, *(pv-1), *pv, *op);
            op--; pv--;
            *pv = v;
          }
          type = 1;
          op--; c = getscochar(csound, 1); break;
        case '^':
          type = 0;
          *++op = c; c = getscochar(csound, 1); break;
        case ']':
          if (UNLIKELY(!type)) {
            scorerr(csound, Str("missing operand before closing bracket in []"));
          }
          while (*op != '[') {
            MYFLT v = operate(csound, *(pv-1), *pv, *op);
            op--; pv--;
            *pv = v;
          }
          c = '$';
          break;
        case '$':
          break;
        case ' ':               /* Ignore spaces */
          c = getscochar(csound, 1);
          continue;
        default:
          scorerr(csound, Str("illegal character %c(%.2x) in [] expression"),
                  c, c);
        }
      } while (c != '$');
      /* Make string macro or value */
      sprintf(buffer, "%f", *pv);
      {
        MACRO *nn = (MACRO*) mmalloc(csound, sizeof(MACRO));
        nn->name = mmalloc(csound, 2);
        strcpy(nn->name, "[");
        nn->body = (char*) mmalloc(csound, strlen(buffer) + 1);
        strcpy(nn->body, buffer);
        nn->acnt = 0;   /* No arguments for arguments */
        nn->next = ST(macros);
        ST(macros) = nn;
        ST(input_cnt)++;
        if (UNLIKELY(ST(input_cnt)>=ST(input_size))) {
          int old = ST(str)-ST(inputs);
          ST(input_size) += 20;
          ST(inputs) = mrealloc(csound, ST(inputs), ST(input_size)
                                                    * sizeof(IN_STACK));
          ST(str) = &ST(inputs)[old];     /* In case it moves */
        }
        ST(str)++;
        ST(str)->string = 1; ST(str)->body = nn->body; ST(str)->args = 0;
        ST(str)->is_marked_repeat = 0;
        ST(str)->mac = NULL; ST(str)->line = 1; ST(str)->unget_cnt = 0;
#ifdef MACDEBUG
        csound->Message(csound,"[] defined as >>%s<<\n", nn->body);
#endif
        ST(ingappop) = 1;
        goto top;
      }
    }
    return c;
}

static int nested_repeat(CSOUND *csound)                /* gab A9*/
{
    ST(repeat_cnt_n)[ST(repeat_index)]--;
    if (ST(repeat_cnt_n)[ST(repeat_index)] == 0) {      /* Expired */
      if (ST(repeat_index) > 1) {
        char c[41];
        int j;
        for (j = 0; j<ST(repeat_index); j++) {
          c[j]=' ';
          c[j+1]='\0';
        }
        if (csound->oparms->msglevel)
          csound->Message(csound,Str("%s Nested LOOP terminated, level:%d\n"),
                          c,ST(repeat_index));

      }
      else {
        if (csound->oparms->msglevel)
          csound->Message(csound,Str("External LOOP terminated, level:%d\n"),
                          ST(repeat_index));
      }
      undefine_score_macro(csound, ST(repeat_name_n)[ST(repeat_index)]);
      ST(repeat_index)--;
    }
    else {
      int i;
      fseek(ST(str)->file, ST(repeat_point_n)[ST(repeat_index)], SEEK_SET);
      sscanf(ST(repeat_mm_n)[ST(repeat_index)]->body, "%d", &i);
      i = i + ST(repeat_inc_n);
      sprintf(ST(repeat_mm_n)[ST(repeat_index)]->body, "%d", i);
      if (ST(repeat_index) > 1) {
        char c[41];
        int j;
        for (j = 0; j<ST(repeat_index); j++) {
          c[j]=' ';
          c[j+1]='\0';
        }
        if (csound->oparms->msglevel)
          csound->Message(csound,Str("%s  Nested LOOP section (%d) Level:%d\n"),
                          c, i, ST(repeat_index));
      }
      else {
        if (csound->oparms->msglevel)
          csound->Message(csound,Str(" External LOOP section (%d) Level:%d\n"),
                          i, ST(repeat_index));
      }
      return 1;
    }
    return 0;
}

static int do_repeat(CSOUND *csound)
{                               /* At end of section repeat if necessary */
    ST(repeat_cnt)--;
    if (ST(repeat_cnt) == 0) {  /* Expired */
      /* Delete macro (assuming there is any) */
      if (csound->oparms->msglevel)
        csound->Message(csound, Str("Loop terminated\n"));
      if (ST(repeat_name)[0] != '\0')
        undefine_score_macro(csound, ST(repeat_name));
      ST(repeat_name)[0] = '\0';
    }
    else {
      int i;
      fseek(ST(str)->file, ST(repeat_point), SEEK_SET);
      if (ST(repeat_name)[0] != '\0') {
        sscanf(ST(repeat_mm)->body, "%d", &i);
        i = i + ST(repeat_inc);
        sprintf(ST(repeat_mm)->body, "%d", i);
        if (csound->oparms->msglevel)
          csound->Message(csound, Str("Repeat section (%d)\n"), i);
      }
      else
        csound->Message(csound, Str("Repeat section\n"));
      /* replace 'e' or 'r' with 's' and end section */
      ST(bp)->text[0] = 's';
      ST(clock_base) = FL(0.0);
      ST(warp_factor) = FL(1.0);
      ST(prvp2) = -FL(1.0);
      return 1;
    }
    return 0;
}

static void init_smacros(CSOUND *csound, NAMES *nn)
{
    MACRO *mm;
    while (nn) {
      char  *s = nn->mac;
      char  *p = strchr(s, '=');
      char  *mname;

      if (p == NULL)
        p = s + strlen(s);
      if (csound->oparms->msglevel & 7)
        csound->Message(csound, Str("Macro definition for %*s\n"), p - s, s);
      s = strchr(s, ':') + 1;                   /* skip arg bit */
      if (UNLIKELY(s == NULL || s >= p))
        csound->Die(csound, Str("Invalid macro name for --smacro"));
      mname = (char*) mmalloc(csound, (p - s) + 1);
      strncpy(mname, s, p - s);
      mname[p - s] = '\0';
      /* check if macro is already defined */
      for (mm = ST(macros); mm != NULL; mm = mm->next) {
        if (strcmp(mm->name, mname) == 0)
          break;
      }
      if (mm == NULL) {
        mm = (MACRO*) mcalloc(csound, sizeof(MACRO));
        mm->name = mname;
        mm->next = ST(macros);
        ST(macros) = mm;
      }
      else
        mfree(csound, mname);
      mm->margs = MARGS;    /* Initial size */
      mm->acnt = 0;
      if (*p != '\0')
        p++;
      mm->body = (char*) mmalloc(csound, strlen(p) + 1);
      strcpy(mm->body, p);
      nn = nn->next;
    }
    mm = (MACRO*) mcalloc(csound, sizeof(MACRO));
    mm->name = (char*)mmalloc(csound,4);
    strcpy(mm->name, "INF");
    mm->body = (char*)mmalloc(csound,14);
    strcpy(mm->body, "2147483647.0");
    mm->next = ST(macros);
    ST(macros) = mm;
}

void sread_init(CSOUND *csound)
{
    sread_alloc_globals(csound);
    ST(inputs) = (IN_STACK*) mmalloc(csound, 20 * sizeof(IN_STACK));
    ST(input_size) = 20;
    ST(input_cnt) = 0;
    ST(str) = ST(inputs);
    ST(str)->file = csound->scorein;
    ST(str)->fd = NULL;
    ST(str)->string = 0; ST(str)->body = csound->scorename;
    ST(str)->is_marked_repeat = 0;
    ST(str)->line = 1; ST(str)->unget_cnt = 0; ST(str)->mac = NULL;
    init_smacros(csound, csound->smacros);
}

void sread_initstr(CSOUND *csound)
{
    sread_alloc_globals(csound);
    ST(inputs) = (IN_STACK*) mmalloc(csound, 20 * sizeof(IN_STACK));
    ST(input_size) = 20;
    ST(input_cnt) = 0;
    ST(str) = ST(inputs);
    ST(str)->file = NULL;
    ST(str)->fd = NULL;
    ST(str)->string = 1; ST(str)->body = corfile_body(csound->scorestr);
    ST(str)->is_marked_repeat = 0;
    ST(str)->line = 1; ST(str)->unget_cnt = 0; ST(str)->mac = NULL;
    init_smacros(csound, csound->smacros);
}

int sread(CSOUND *csound)       /*  called from main,  reads from SCOREIN   */
{                               /*  each score statement gets a sortblock   */
    int  rtncod;                /* return code to calling program:      */
                                /*   1 = section read                   */
                                /*   0 = end of file                    */
    sread_alloc_globals(csound);
    ST(bp) = ST(prvibp) = csound->frstbp = NULL;
    ST(nxp) = NULL;
    ST(warpin) = 0;
    ST(lincnt) = 1;
    csound->sectcnt++;
    rtncod = 0;
    salcinit(csound);           /* init the mem space for this section  */

    while ((ST(op) = getop(csound)) != EOF) { /* read next op from scorefile */
      rtncod = 1;
      salcblk(csound);          /* build a line structure; init bp,nxp  */
    again:
      switch (ST(op)) {         /*  and dispatch on opcodes             */
      case 'i':
      case 'f':
      case 'a':
      case 'q':
        ifa(csound);
        break;
      case 'w':
        ST(warpin)++;
        copypflds(csound);
        break;
      case 't':
        copypflds(csound);
        break;
      case 'b': /* Set a clock base */
        {
          char *old_nxp = ST(nxp)-2;
          getpfld(csound);
          ST(clock_base) = stof(csound, ST(sp));
          if (csound->oparms->msglevel)
            csound->Message(csound,Str("Clockbase = %f\n"), ST(clock_base));
          flushlin(csound);
          ST(op) = getop(csound);
          ST(nxp) = old_nxp;
          *ST(nxp)++ = ST(op); /* Undo this line */
          ST(nxp)++;
          goto again;
        }
      case 's':
      case 'e':
        /* check for optional p1 before doing repeats */
        copylin(csound);
        {
          char  *p = &(ST(bp)->text[1]);
          char q;
          while (*p == ' ' || *p == '\t')
            p++;
          /* Measurement shows isdigit and 3 cases is about 30% */
          /* faster than use of strchr (measured on Suse9.3)    */
          /*         if (strchr("+-.0123456789", *p) != NULL) { */
          q = *p;
          if (isdigit(q) || q=='+' || q=='-' || q=='.') {
            double  tt;
            char    *tmp = p;
            tt = strtod(p, &tmp);
            if (tmp != p && (*tmp == '\0' || isspace(*tmp))) {
              ST(bp)->pcnt = 1;
              ST(bp)->p1val = ST(bp)->p2val = ST(bp)->newp2 = (MYFLT) tt;
            }
          }
        }
        /* If we are in a repeat of a marked section ('n' statement),
           we must pop those inputs before doing an 'r' repeat. */
        while (ST(str)->is_marked_repeat && ST(input_cnt) > 0) {
          /* close all marked repeat inputs */
          if (ST(str)->fd != NULL) {
            csound->FileClose(csound, ST(str)->fd); ST(str)->fd = NULL;
          }
          ST(str)--; ST(input_cnt)--;
        }
        if (ST(repeat_cnt) != 0) {
          if (do_repeat(csound))
            return rtncod;
        }
        if (ST(op) != 'e') {
          ST(clock_base) = FL(0.0);
          ST(warp_factor) = FL(1.0);
          ST(prvp2) = -FL(1.0);
        }
        return rtncod;
      case '}':
        {
          int temp;
          char *old_nxp = ST(nxp)-2;
          if ((temp=ST(repeat_cnt_n)[ST(repeat_index)])!=0)
            nested_repeat(csound);
          ST(op) = getop(csound);
          ST(nxp) = old_nxp;
          *ST(nxp)++ = ST(op);
          ST(nxp)++;
          goto again;
        }
      case '{':
        {
          char *old_nxp = ST(nxp)-2;
          ST(repeat_index)++;
          if (UNLIKELY(ST(repeat_index) >= RPTDEPTH))
            scorerr(csound, Str("Loops are nested too deeply"));
          if (UNLIKELY(ST(str)->string)) {
            /* seems too dangerous at this point to continue  -- akozar */
            scorerr(csound, Str("Loop cannot start inside of a macro"));
            ST(repeat_index)--;
            flushlin(csound);     /* Ignore rest of line */
          }
          else {
            int c, i;
            ST(repeat_mm_n)[ST(repeat_index)] =
              (MACRO*)mmalloc(csound, sizeof(MACRO));
            ST(repeat_cnt_n)[ST(repeat_index)] = 0;
            do {
              c = getscochar(csound, 1);
            } while (c==' '||c=='\t');
            while (isdigit(c)) {
              ST(repeat_cnt_n)[ST(repeat_index)] =
                10 * ST(repeat_cnt_n)[ST(repeat_index)] + c - '0';
              c = getscochar(csound, 1);
            }
            if (UNLIKELY(ST(repeat_cnt_n)[ST(repeat_index)] <= 0
                         || (c != ' ' && c != '\t' && c != '\n')))
              scorerr(csound, Str("{: invalid repeat count"));
            if (ST(repeat_index) > 1) {
              char st[41];
              int j;
              for (j = 0; j < ST(repeat_index); j++) {
                st[j] = ' ';
                st[j+1] = '\0';
              }
              if (csound->oparms->msglevel)
                csound->Message(csound, Str("%s Nested LOOP=%d Level:%d\n"),
                                        st, ST(repeat_cnt_n)[ST(repeat_index)],
                                        ST(repeat_index));
            }
            else {
              if (csound->oparms->msglevel)
                csound->Message(csound, Str("External LOOP=%d Level:%d\n"),
                                        ST(repeat_cnt_n)[ST(repeat_index)],
                                        ST(repeat_index));
            }
            while (c == ' ' || c == '\t') {
              c = getscochar(csound, 1);
            }
            for (i = 0; isNameChar(c, i) && i < (NAMELEN-1); i++) {
              ST(repeat_name_n)[ST(repeat_index)][i] = c;
              c = getscochar(csound, 1);
            }
            ST(repeat_name_n)[ST(repeat_index)][i] = '\0';
            ungetscochar(csound, c);
            /* Define macro for counter */
            ST(repeat_mm_n)[ST(repeat_index)]->name =
              mmalloc(csound, strlen(ST(repeat_name_n)[ST(repeat_index)])+1);
            strcpy(ST(repeat_mm_n)[ST(repeat_index)]->name,
                   ST(repeat_name_n)[ST(repeat_index)]);
            ST(repeat_mm_n)[ST(repeat_index)]->acnt = 0;
            ST(repeat_mm_n)[ST(repeat_index)]->body = (char*)mmalloc(csound, 12);
            sprintf(ST(repeat_mm_n)[ST(repeat_index)]->body, "%d", 0);
            ST(repeat_mm_n)[ST(repeat_index)]->next = ST(macros);
            ST(macros) = ST(repeat_mm_n)[ST(repeat_index)];
            flushlin(csound);     /* Ignore rest of line */
            ST(repeat_point_n)[ST(repeat_index)] = ftell(ST(str)->file);
          }
          /* { does not start a new section - akozar */
          /* ST(clock_base) = FL(0.0);
          ST(warp_factor) = FL(1.0);
          ST(prvp2) = -FL(1.0); */
          ST(op) = getop(csound);
          ST(nxp) = old_nxp;
          *ST(nxp)++ = ST(op);
          ST(nxp)++;
          goto again;
        }
      case 'r':                 /* For now treat as s */
                                /* First deal with previous section */
        /* If we are in a repeat of a marked section ('n' statement),
           we must pop those inputs before doing an 'r' repeat. */
        if (ST(str)->is_marked_repeat) {
          while (ST(str)->is_marked_repeat && ST(input_cnt) > 0) {
            /* close all marked repeat inputs */
            if (ST(str)->fd != NULL) {
              csound->FileClose(csound, ST(str)->fd); ST(str)->fd = NULL;
            }
            ST(str)--; ST(input_cnt)--;
          }
          /* last time thru an 'r', cleanup up 'r' before finishing 'n' */
          if (ST(repeat_cnt) == 1)  do_repeat(csound);
          if (ST(repeat_cnt) == 0) {
            /* replace with 's' and end section if no previous 'r'
               or just finished an 'r' loop */
            ST(bp)->text[0] = 's';
            ST(clock_base) = FL(0.0);
            ST(warp_factor) = FL(1.0);
            ST(prvp2) = -FL(1.0);
            return rtncod;
          }
        }
        if (ST(repeat_cnt) != 0) {
          if (do_repeat(csound))
            return rtncod;
        }
        /* Then remember this state */
        *(ST(nxp)-2) = 's'; *ST(nxp)++ = LF;
        if (ST(nxp) >= ST(memend))              /* if this memblk exhausted */
          expand_nxp(csound);
        if (UNLIKELY(ST(str)->string)) {
          sreaderr(csound,Str("Repeat cannot start inside of a macro "
                                                           "(ignored)"));
          flushlin(csound);     /* Ignore rest of line */
        }
        else {
          int   c, i;
          ST(repeat_cnt) = 0;
          do {
            c = getscochar(csound, 1);
          } while (c == ' ' || c == '\t');
          while (isdigit(c)) {
            ST(repeat_cnt) = 10 * ST(repeat_cnt) + c - '0';
            c = getscochar(csound, 1);
          }
          if (UNLIKELY(ST(repeat_cnt) <= 0 || (c != ' ' && c != '\t' && c != '\n')))
            scorerr(csound, Str("r: invalid repeat count"));
          if (csound->oparms->msglevel)
            csound->Message(csound, Str("Repeats=%d\n"), ST(repeat_cnt));
          while (c == ' ' || c == '\t') {
            c = getscochar(csound, 1);
          }
          for (i = 0; isNameChar(c, i) && i < (NAMELEN-1); i++) {
            ST(repeat_name)[i] = c;
            c = getscochar(csound, 1);
          }
          ST(repeat_name)[i] = '\0';
          ungetscochar(csound, c);
          flushlin(csound);     /* Ignore rest of line */
          if (i) {
            /* Only if there is a name: define macro for counter */
            ST(repeat_mm) = (MACRO*) mmalloc(csound, sizeof(MACRO));
            ST(repeat_mm)->name = mmalloc(csound, strlen(ST(repeat_name)) + 1);
            strcpy(ST(repeat_mm)->name, ST(repeat_name));
            ST(repeat_mm)->acnt = 0;
            ST(repeat_mm)->body = (char*)mmalloc(csound, 8);
            sprintf(ST(repeat_mm)->body, "%d", 1); /* Set value */
            ST(repeat_mm)->next = ST(macros);
            ST(macros) = ST(repeat_mm);
          }
          ST(repeat_point) = ftell(ST(str)->file);
        }
        ST(clock_base) = FL(0.0);
        ST(warp_factor) = FL(1.0);
        ST(prvp2) = -FL(1.0);
        return rtncod;
      case 'm': /* Remember this place */
        {
          char  *old_nxp = ST(nxp)-2;
          char  buff[200];
          int   c;
          int   i = 0, j;
          while ((c = getscochar(csound, 1)) == ' ' || c == '\t');
          while (isNameChar(c, i)) {
            buff[i++] = c;
            c = getscochar(csound, 1);
          }
          buff[i] = '\0';
          if (c != '\n' && c != EOF) flushlin(csound);
          if (csound->oparms->msglevel)
            csound->Message(csound,Str("Named section >>>%s<<<\n"), buff);
          for (j=0; j<=ST(next_name); j++)
            if (strcmp(buff, ST(names)[j].name)==0) break;
          if (j>ST(next_name)) {
            j = ++ST(next_name);
            ST(names)[j].name = (char*)mmalloc(csound, i+1);
            strcpy(ST(names)[j].name, buff);
          }
          else mfree(csound, ST(names)[j].file);
          if (!ST(str)->string) {
            ST(names)[ST(next_name)].posit = ftell(ST(str)->file);
            ST(names)[ST(next_name)].line = ST(str)->line;
            ST(names)[ST(next_name)].file = mmalloc(csound,
                                                    strlen(ST(str)->body) + 1);
            strcpy(ST(names)[ST(next_name)].file, ST(str)->body);
            if (csound->oparms->msglevel)
              csound->Message(csound,Str("%d: File %s position %ld\n"),
                              ST(next_name), ST(names)[ST(next_name)].file,
                              ST(names)[ST(next_name)].posit);
          }
          else {
            csound->Message(csound,
                             Str("Ignoring name %s not in file\n"), buff);
            ST(names)[j].name[0] = '\0'; /* Destroy name */
          }
          ST(op) = getop(csound);
          ST(nxp) = old_nxp;
          *ST(nxp)++ = ST(op); /* Undo this line */
          ST(nxp)++;
          goto again;           /* suggested this loses a line?? */
        }
      case 'n':
        {
          char *old_nxp = ST(nxp)-2;
          char buff[200];
          int c;
          int i = 0;
          while ((c = getscochar(csound, 1)) == ' ' || c == '\t');
          while (isNameChar(c, i)) {
            buff[i++] = c;
            c = getscochar(csound, 1);
          }
          buff[i] = '\0';
          if (c != '\n' && c != EOF) flushlin(csound);
          for (i = 0; i<=ST(next_name); i++)
            if (strcmp(buff, ST(names)[i].name)==0) break;
          if (UNLIKELY(i > ST(next_name)))
            sreaderr(csound, Str("Name %s not found"), buff);
          else {
            csound->Message(csound, Str("Duplicate %d: %s (%s,%ld)\n"),
                            i, buff, ST(names)[i].file, ST(names)[i].posit);
            ST(input_cnt)++;
            if (ST(input_cnt)>=ST(input_size)) {
              int old = ST(str)-ST(inputs);
              ST(input_size) += 20;
              ST(inputs) = mrealloc(csound, ST(inputs),
                                            ST(input_size) * sizeof(IN_STACK));
              ST(str) = &ST(inputs)[old];     /* In case it moves */
            }
            ST(str)++;
            ST(str)->string = 0;
            ST(str)->is_marked_repeat = 1;
            ST(str)->fd = fopen_path(csound, &(ST(str)->file),
                                             ST(names)[i].file, NULL, NULL, 1);
            /* RWD 3:2000 */
            if (UNLIKELY(ST(str)->fd == NULL)) {
              csoundDie(csound, Str("cannot open input file %s"),
                                ST(names)[i].file);
            }
            ST(str)->body = csound->GetFileName(ST(str)->fd);
            ST(str)->line = ST(names)[i].line;
            ST(str)->unget_cnt = 0;
            fseek(ST(str)->file, ST(names)[i].posit, SEEK_SET);
          }
          ST(op) = getop(csound);
          ST(nxp) = old_nxp;
          *ST(nxp)++ = ST(op); /* Undo this line */
          ST(nxp)++;
          goto again;
        }
      case 'v': /* Suggestion of Bryan Bales */
        {       /* Set local variability of time */
          char *old_nxp = ST(nxp)-2;
          getpfld(csound);
          ST(warp_factor) = stof(csound, ST(sp));
          if (csound->oparms->msglevel)
            csound->Message(csound, Str("Warp_factor = %f\n"), ST(warp_factor));
          flushlin(csound);
          ST(op) = getop(csound);
          ST(nxp) = old_nxp;
          *ST(nxp)++ = ST(op);          /* Undo this line */
          ST(nxp)++;
          goto again;
        }
      case 'x':                         /* Skip section */
        while (1) {
          switch (ST(op) = getop(csound)) {
          case 's':
          case 'r':
          case 'm':
          case 'e':
            salcblk(csound);            /* place op, blank into text    */
            goto again;
          case EOF:
            goto ending;
          default:
            flushlin(csound);
          }
        }
        break;
      default:
        csound->Message(csound,Str("sread is confused on legal opcodes\n"));
        break;
      }
    }
 ending:
    if (ST(repeat_cnt) > 0) {
      ST(op) = 'e';
      salcblk(csound);
      if (do_repeat(csound))
        return rtncod;
      *ST(nxp)++ = LF;
    }
    if (!rtncod) {                      /* Ending so clear macros */
      while (ST(macros) != NULL) {
        undefine_score_macro(csound, ST(macros)->name);
      }
    }
    return rtncod;
}

static void copylin(CSOUND *csound)     /* copy source line to srtblk   */
{
    int c;
    ST(nxp)--;
    if (ST(nxp) >= ST(memend))          /* if this memblk exhausted */
      expand_nxp(csound);
    do {
      c = getscochar(csound, 1);
      *ST(nxp)++ = c;
    } while (c != LF && c != EOF);
    if (c == EOF) *(ST(nxp)-1) = '\n';  /* Avoid EOF characters */
    ST(lincnt)++;
    ST(linpos) = 0;
}

static void copypflds(CSOUND *csound)
{
    ST(bp)->pcnt = 0;
    while (getpfld(csound))     /* copy each pfield,    */
      ST(bp)->pcnt++;           /* count them,          */
    *(ST(nxp)-1) = LF;          /* terminate with newline */
}

static void ifa(CSOUND *csound)
{
    SRTBLK *prvbp;
    int n, nocarry = 0;

    ST(bp)->pcnt = 0;
    while (getpfld(csound)) {   /* while there's another pfield,  */
      nocarry = 0;
      ++ST(bp)->pcnt;
      /* if (UNLIKELY(++ST(bp)->pcnt == PMAX)) { */
      /*   sreaderr(csound, Str("instr pcount exceeds PMAX")); */
      /*   csound->Message(csound, Str("      remainder of line flushed\n")); */
      /*   flushlin(csound); */
      /*   continue; */
      /* } */
      if (*ST(sp) == '^' && ST(op) == 'i' && ST(bp)->pcnt == 2) {
        int foundplus = 0;
        if (*(ST(sp)+1)=='+') { ST(sp)++; foundplus = 1; }
        if (UNLIKELY(ST(prvp2)<0)) {
          sreaderr(csound,Str("No previous event for ^"));
          ST(prvp2) = ST(bp)->p2val = ST(warp_factor) * stof(csound, ST(sp)+1);
        }
        else if (UNLIKELY(isspace(*(ST(sp)+1)))) {
          /* stof() assumes no leading whitespace -- 070204, akozar */
          sreaderr(csound, Str("illegal space following %s, zero substituted"),
                           (foundplus ? "^+" : "^"));
          ST(prvp2) = ST(bp)->p2val = ST(prvp2);
        }
        else ST(prvp2) = ST(bp)->p2val =
                         ST(prvp2) + ST(warp_factor) * stof(csound, ST(sp) + 1);
      }
      else if (ST(nxp)-ST(sp) == 2 && (*ST(sp) == '.' || *ST(sp) == '+')) {
        if (ST(op) == 'i'
            && (*ST(sp) == '.' || ST(bp)->pcnt == 2)
            && ((ST(bp)->pcnt >= 2 && (prvbp = ST(prvibp)) != NULL
                 && ST(bp)->pcnt <= prvbp->pcnt)
                || (ST(bp)->pcnt == 1 && (prvbp = ST(bp)->prvblk) != NULL
                    && prvbp->text[0] == 'i'))) {
          if (*ST(sp) == '.') {
            ST(nxp) = ST(sp);
            pcopy(csound, (int) ST(bp)->pcnt, 1, prvbp);
            if (ST(bp)->pcnt >= 2) ST(prvp2) = ST(bp)->p2val;
          }
          else /* need the fabs() in case of neg p3 */
            ST(prvp2) = ST(bp)->p2val =
                        prvbp->p2val + FABS(prvbp->p3val);
        }
        else carryerror(csound);
      }
      else if (*ST(sp) == '!') {
        int getmore = 0;
        if (UNLIKELY(ST(op) != 'i')) {
          *(ST(nxp)-1) = '\0';
          getmore = 1;
          sreaderr(csound, Str("ignoring '%s' in '%c' event"), ST(sp), ST(op));
        }
        else if (UNLIKELY(ST(bp)->pcnt < 4)) {
          sreaderr(csound, Str("! invalid in p1, p2, or p3"));
          csound->Message(csound, Str("      remainder of line flushed\n"));
          flushlin(csound);
        }
        else if (UNLIKELY(ST(nxp)-ST(sp) != 2)) {
          sreaderr(csound, Str("illegal character after !: '%c'"), *(ST(sp)+1));
          csound->Message(csound, Str("      remainder of line flushed\n"));
          flushlin(csound);
        }
        else {
          nocarry = 1;         /* only set when no syntax errors */
          flushlin(csound);
        }
        /* but always delete the pfield beginning with '!' */
        ST(nxp) = ST(sp);
        ST(bp)->pcnt--;
        if (getmore) continue; /* not the best, but not easy to delete event */
                               /* since ifa() doesn't return anything */
        else break;
      }
      else switch (ST(bp)->pcnt) {      /*  watch for p1,p2,p3, */
      case 1:                           /*   & MYFLT, setinsno..*/
        if ((ST(op) == 'i' || ST(op) == 'q') && *ST(sp) == '"') {
        /*   printf("***Entering second dubious code scnt=%d\n", csound->scnt0); */
        /*   ST(bp)->p1val = ((int[4]){SSTRCOD,SSTRCOD1,SSTRCOD2,SSTRCOD3})[csound->scnt0++]; */
        /*   if (csound->scnt0>3) { */
        /*     csound->scnt0 = 3; */
        /*   } */
          ST(bp)->p1val = SSTRCOD;      /* allow string name */
        }
        else
          ST(bp)->p1val = stof(csound, ST(sp));
        if (ST(op) == 'i')
          setprv(csound);
        else ST(prvibp) = NULL;
        break;
      case 2: ST(prvp2) = ST(bp)->p2val =
                          ST(warp_factor)*stof(csound, ST(sp)) + ST(clock_base);
        break;
      case 3: if (ST(op) == 'i')
                ST(bp)->p3val = ST(warp_factor) * stof(csound, ST(sp));
              else ST(bp)->p3val = stof(csound, ST(sp));
      break;
      default:break;
      }
      switch (ST(bp)->pcnt) {               /* newp2, newp3:   */
      case 2: if (ST(warpin)) {             /* for warpin,     */
        getpfld(csound);                    /*   newp2 follows */
        ST(bp)->newp2 = ST(warp_factor) * stof(csound, ST(sp)) + ST(clock_base);
        ST(nxp) = ST(sp);                   /*    (skip text)  */
      }
      else ST(bp)->newp2 = ST(bp)->p2val;   /* else use p2val  */
      break;
      case 3: if (ST(warpin) && (ST(op) == 'i' || ST(op) == 'f')) {
        getpfld(csound);                    /* same for newp3  */
        ST(bp)->newp3 = ST(warp_factor) * stof(csound, ST(sp));
        ST(nxp) = ST(sp);
      }
      else ST(bp)->newp3 = ST(bp)->p3val;
      break;
      }
    }
    if (ST(op) == 'i' && !nocarry &&    /* then carry any rem pflds */
        ((prvbp = ST(prvibp)) != NULL ||
         (!ST(bp)->pcnt && (prvbp = ST(bp)->prvblk) != NULL &&
          prvbp->text[0] == 'i')) &&
        (n = prvbp->pcnt - ST(bp)->pcnt) > 0) {
      pcopy(csound, (int) ST(bp)->pcnt + 1, n, prvbp);
      ST(bp)->pcnt += n;
    }
    *(ST(nxp)-1) = LF;                  /* terminate this stmnt with newline */
}

static void setprv(CSOUND *csound)      /*  set insno = (int) p1val         */
{                                       /*  prvibp = prv note, same insno   */
    SRTBLK *p = ST(bp);
    int16 n;

    if (ST(bp)->p1val == SSTRCOD && *ST(sp) == '"') {   /* IV - Oct 31 2002 */
      char name[MAXNAME], *c, *s = ST(sp);
      /* unquote instrument name */
      c = name; while (*++s != '"') *c++ = *s; *c = '\0';
      /* find corresponding insno */
      if (UNLIKELY(!(n = (int16) named_instr_find(csound, name)))) {
        csound->Message(csound, Str("WARNING: instr %s not found, "
                                    "assuming insno = -1\n"), name);
        n = -1;
      }
    }
    else n = (int16) ST(bp)->p1val;         /* set current insno */
    ST(bp)->insno = n;

    while ((p = p->prvblk) != NULL)
      if (p->insno == n) {
        ST(prvibp) = p;                     /* find prev same */
        return;
      }
    ST(prvibp) = NULL;                      /*  if there is one */
}

static void carryerror(CSOUND *csound)      /* print offending text line  */
{                                           /*      (partial)             */
    char *p;

    csound->Message(csound, Str("sread: illegal use of carry, "
                                "  0 substituted\n"));
    *(ST(nxp) - 3) = SP;
    p = ST(bp)->text;
    while (p <= ST(nxp) - 2)
      csound->Message(csound, "%c", *p++);
    csound->Message(csound, "<=\n");
    print_input_backtrace(csound, 1, csoundMessage);
    *(ST(nxp) - 2) = '0';
}

static void pcopy(CSOUND *csound, int pfno, int ncopy, SRTBLK *prvbp)
                                /* cpy pfields from prev note of this instr */
                                /*     begin at pfno, copy 'ncopy' fields   */
                                /*     uses *nxp++;    sp untouched         */
{
    char *p, *pp, c;
    int  n;

    pp = prvbp->text;                       /* in text of prev note,    */
    n = pfno;
    while (n--)
      while (*pp++ != SP)                   /*    locate starting pfld  */
        ;
    n = ncopy;
    p = ST(nxp);
    while (n--) {                           /*      and copy n pflds    */
      if (*pp != '"')
        while ((*p++ = c = *pp++) != SP && c != LF)
          ;
      else {
        *p++ = *pp++;
        while ((*p++ = *pp++) != '"')
          ;
        *p++ = *pp++;
      }
      switch (pfno) {
      case 1: ST(bp)->p1val = prvbp->p1val;       /*  with p1-p3 vals */
        setprv(csound);
        break;
      case 2: if (*(p-2) == '+')              /* (interpr . of +) */
        ST(prvp2) = ST(bp)->p2val = prvbp->p2val + FABS(prvbp->p3val);
      else ST(prvp2) = ST(bp)->p2val = prvbp->p2val;
      ST(bp)->newp2 = ST(bp)->p2val;
      break;
      case 3: ST(bp)->newp3 = ST(bp)->p3val = prvbp->p3val;
        break;
      default:break;
      }
      ST(bp)->lineno = prvbp->lineno;
      pfno++;
    }
    ST(nxp) = p;                                /* adjust globl nxp pntr */
}

static void salcinit(CSOUND *csound)
{                             /* init the sorter mem space for a new section */
    if (ST(curmem) == NULL) { /*  alloc 1st memblk if nec; init *nxp to this */
      ST(curmem) = (char*) mmalloc(csound, (size_t) (MEMSIZ + MARGIN));
      ST(memend) = (char*) ST(curmem) + MEMSIZ;
    }
    ST(nxp) = (char*) ST(curmem);
}

static void salcblk(CSOUND *csound)
{                               /* alloc a srtblk from current mem space:   */
    SRTBLK  *prvbp;             /*   align following *nxp, set new bp, nxp  */
                                /*   set srtblk lnks, put op+blank in text  */
    if (ST(nxp) >= ST(memend))          /* if this memblk exhausted */
      expand_nxp(csound);
    /* now allocate a srtblk from this space: */
    prvbp = ST(bp);
    ST(bp) = (SRTBLK*) (((uintptr_t) ST(nxp) + (uintptr_t)7) & ~((uintptr_t)7));
    if (csound->frstbp == NULL)
      csound->frstbp = ST(bp);
    if (prvbp != NULL)
      prvbp->nxtblk = ST(bp);           /* link with prev srtblk        */
    ST(bp)->nxtblk = NULL;
    ST(bp)->prvblk = prvbp;
    ST(bp)->insno = 0;
    ST(bp)->pcnt = 0;
    ST(bp)->lineno = ST(lincnt);
    ST(nxp) = &(ST(bp)->text[0]);
    *ST(nxp)++ = ST(op);                /* place op, blank into text    */
    *ST(nxp)++ = SP;
    *ST(nxp) = '\0';
}

void sfree(CSOUND *csound)       /* free all sorter allocated space */
{                                /*    called at completion of sort */
    sread_alloc_globals(csound);
    if (ST(curmem) != NULL) {
      mfree(csound, ST(curmem));
      ST(curmem) = NULL;
    }
    while (ST(str) != &ST(inputs)[0]) {
      if (!ST(str)->string && ST(str)->fd != NULL) {
        csound->FileClose(csound, ST(str)->fd);
        ST(str)->fd = NULL;
      }
      ST(str)--;
    }
    corfile_rm(csound->scorestr);
}

static void flushlin(CSOUND *csound)
{                                   /* flush input to end-of-line; inc lincnt */
    int c;
    while ((c = getscochar(csound, 0)) != LF && c != EOF)
      ;
    ST(linpos) = 0;
    ST(lincnt)++;
}

static inline int check_preproc_name(CSOUND *csound, const char *name)
{
    int   i;
    char  c;
    for (i = 1; name[i] != '\0'; i++) {
      c = (char) getscochar(csound, 1);
      if (c != name[i])
        return 0;
    }
    return 1;
}

static int sget1(CSOUND *csound)    /* get first non-white, non-comment char */
{
    int c;

 srch:
    while ((c = getscochar(csound, 1)) == SP || c == '\t' || c == LF)
      if (c == LF) {
        ST(lincnt)++;
        ST(linpos) = 0;
      }
    if (c == ';' || c == 'c') {
      flushlin(csound);
      goto srch;
    }
    if (c == '\\') {            /* Deal with continuations and specials */
 again:
      c = getscochar(csound, 1);
      if (c==';') {
        while ((c=getscochar(csound, 1)!='\n') && c!=EOF);
        goto srch;
      }
      if (c==' ' || c=='\t') goto again;
      if (c!='\n' && c!=EOF) {
        csound->Message(csound, Str("Improper \\"));
        while (c!='\n' && c!=EOF) c = getscochar(csound, 1);
      }
      goto srch;
    }
    if (c == '/') {             /* Could be a C-comment */
      c = getscochar(csound, 1);
      if (c != '*') {
        ungetscochar(csound, c);
        c = '/';
      }
      else {                    /* It is a comment */
      top:
        while ((c = getscochar(csound, 0)) != '*');
        if ((c = getscochar(csound, 0)) != '/') {
          if (c != EOF) goto top;
          return EOF;
        }
        goto srch;
      }
    }
    if (c == '#') {
      int mlen = 40;
      char  *mname = malloc(40);         /* Start Macro definition */
      int   i = 0;
      while (isspace((c = getscochar(csound, 1))));
      if (c == 'd') {
        int   arg = 0;
        int   size = 100;
        MACRO *mm = (MACRO*) mmalloc(csound, sizeof(MACRO));
        mm->margs = MARGS;
        if (UNLIKELY(!check_preproc_name(csound, "define"))) {
          csound->Message(csound, Str("Not #define"));
          mfree(csound, mm); free(mname);
          flushlin(csound);
          //          free(mname);
          goto srch;
        }
        while (isspace((c = getscochar(csound, 1))));
        while (isNameChar(c, i)) {
          mname[i++] = c;
          if (i==mlen) mname = (char *)realloc(mname, mlen+=40);
          c = getscochar(csound, 1);
        }
        mname[i] = '\0';
        if (csound->oparms->msglevel)
          csound->Message(csound, Str("Macro definition for %s\n"), mname);
        mm->name = mmalloc(csound, i + 1);
        strcpy(mm->name, mname);
        if (c == '(') { /* arguments */
          do {
            while (isspace((c = getscochar(csound, 1))));
            i = 0;
            while (isNameChar(c, i)) {
              mname[i++] = c;
              if (i==mlen) mname = (char *)realloc(mname, mlen+=40);
              c = getscochar(csound, 1);
            }
            mname[i] = '\0';
            mm->arg[arg] = mmalloc(csound, i+1);
            strcpy(mm->arg[arg++], mname);
            if (arg>=mm->margs) {
              mm = (MACRO*)mrealloc(csound, mm,
                                    sizeof(MACRO)+mm->margs*sizeof(char*));
              mm->margs += MARGS;
            }
            while (isspace(c)) c = getscochar(csound, 1);
          } while (c=='\'' || c=='#');
          if (UNLIKELY(c!=')')) {
            csound->Message(csound, Str("macro error\n"));
            flushlin(csound);
            free(mname);
            goto srch;
          }
        }
        mm->acnt = arg;
        i = 0;
        while ((c = getscochar(csound, 1)) != '#');   /* Skip to next # */
        mm->body = (char*)mmalloc(csound, 100);
        while ((c = getscochar(csound, 0)) != '#') {  /* Do not expand here!! */
          mm->body[i++] = c;
          if (UNLIKELY(i>= size))
            mm->body = mrealloc(csound, mm->body, size += 100);
          if (c=='\\') {
            mm->body[i++] = getscochar(csound, 0);    /* Allow escaped # */
            if (UNLIKELY(i>= size))
              mm->body = mrealloc(csound, mm->body, size += 100);
          }
          if (c=='\n') ST(lincnt)++;
        }
        mm->body[i]='\0';
        mm->next = ST(macros);
        ST(macros) = mm;
#ifdef MACDEBUG
        csound->Message(csound, Str("Macro %s with %d arguments defined\n"),
                                mm->name, mm->acnt);
#endif
        c = ' ';
        flushlin(csound);
        free(mname);
        goto srch;
      }
      else if (c == 'i') {
        int delim;
        if (UNLIKELY(!check_preproc_name(csound, "include"))) {
          csound->Message(csound, Str("Not #include"));
          flushlin(csound);
          free(mname);
          goto srch;
        }
        while (isspace((c = getscochar(csound, 1))));
        delim = c;
        i = 0;
        while ((c=getscochar(csound, 1))!=delim) {
          mname[i++] = c;
          if (i==mlen) mname = (char *)realloc(mname, mlen+=40);
        }
        mname[i]='\0';
        while ((c=getscochar(csound, 1))!='\n');
        ST(input_cnt)++;
        if (ST(input_cnt)>=ST(input_size)) {
          int old = ST(str)-ST(inputs);
          ST(input_size) += 20;
          ST(inputs) = mrealloc(csound, ST(inputs), ST(input_size)
                                                    * sizeof(IN_STACK));
          ST(str) = &ST(inputs)[old];     /* In case it moves */
        }
        ST(str)++;
        ST(str)->string = 0;
        ST(str)->is_marked_repeat = 0;
        ST(str)->fd = fopen_path(csound, &(ST(str)->file), mname,
                                         csound->scorename, "INCDIR", 1);
        if (ST(str)->fd == NULL) {
          ST(str)--;
          ST(str)->line--; /* include was one line earlier */
          ST(linepos) = 0;
          scorerr(csound, Str("Cannot open #include'd file %s"), mname);
        }
        else {
          ST(str)->body = csound->GetFileName(ST(str)->fd);
          ST(str)->line = 1; ST(str)->unget_cnt = 0;
          free(mname);
          goto srch;
        }
      }
      else if (c == 'u') {
        if (UNLIKELY(!check_preproc_name(csound, "undef"))) {
          csound->Message(csound, Str("Not #undef"));
          flushlin(csound);
          free(mname);
          goto srch;
        }
        while (isspace((c = getscochar(csound, 1))));
        while (isNameChar(c, i)) {
          mname[i++] = c;
          if (i==mlen) mname = (char *)realloc(mname, mlen+=40);
          c = getscochar(csound, 1);
        }
        mname[i] = '\0';
        if (csound->oparms->msglevel)
          csound->Message(csound, Str("macro %s undefined\n"), mname);
        undefine_score_macro(csound, mname);
        while (c != '\n' && c != EOF)
          c = getscochar(csound, 1); /* ignore rest of line */
      }
      else {
        sreaderr(csound, Str("unknown # option"));
        flushlin(csound);
      }
      free(mname);
      goto srch;
    }

    return c;
}

static int getop(CSOUND *csound)        /* get next legal opcode */
{
    int c;

 nextc:
    c = sget1(csound);  /* get first active char */

    switch (c) {        /*   and check legality  */
    case 'a':           /* Advance time */
    case 'b':           /* Reset base clock */
    case 'e':           /* End of all */
    case 'f':           /* f-table */
    case 'i':           /* Instrument */
    case 'm':           /* Mark this point */
    case 'n':           /* Duplicate from named position */
    case 'q':           /* quiet instrument ie mute */
    case 'r':           /* Repeated section */
    case 's':           /* Section */
    case 't':           /* time warp */
    case 'v':           /* Local warping */
    case 'w':
    case 'x':
    case '{':           /* Section brackets */
    case '}':
    case EOF:
      break;            /* if ok, go with it    */
    default:            /*   else complain      */
      sreaderr(csound, Str("illegal opcode %c"), c);
      csound->Message(csound,Str("      remainder of line flushed\n"));
      flushlin(csound);
      goto nextc;
    }
    ST(linpos)++;
    return(c);
}

static int getpfld(CSOUND *csound)      /* get pfield val from SCOREIN file */
{                                       /*      set sp, nxp                 */
    int  c;
    char *p;

    if ((c = sget1(csound)) == EOF)     /* get 1st non-white,non-comment c  */
      return(0);
                    /* if non-numeric, and non-carry, and non-special-char: */
    /*    if (strchr("0123456789.+-^np<>()\"~!", c) == NULL) { */
    if (!isdigit(c) && c!='.' && c!='+' && c!='-' && c!='^' && c!='n'
        && c!='p' && c!='<' && c!='>' && c!='(' && c!=')'
        && c!='"' && c!='~' && c!='!' && c!='z') {
      ungetscochar(csound, c);                /* then no more pfields    */
      if (UNLIKELY(ST(linpos))) {
        sreaderr(csound, Str("unexpected char %c"), c);
        csound->Message(csound, Str("      remainder of line flushed\n"));
        flushlin(csound);
      }
      return(0);                              /*    so return            */
    }
    p = ST(sp) = ST(nxp);                     /* else start copying to text */
    *p++ = c;
    ST(linpos)++;
    if (c == '"') {                           /* if have quoted string,  */
      /* IV - Oct 31 2002: allow string instr name for i and q events */
      if (UNLIKELY(ST(bp)->pcnt < 3 &&
          !((ST(op) == 'i' || ST(op) == 'q') &&
            !ST(bp)->pcnt))) {
        sreaderr(csound, Str("illegally placed string"));
        csound->Message(csound, Str("      remainder of line flushed\n"));
        flushlin(csound);
        return(0);
      }
      while ((c = getscochar(csound, 1)) != '"') {
        if (UNLIKELY(c == LF || c == EOF)) {
          sreaderr(csound, Str("unmatched quote"));
          return(0);
        }
        *p++ = c;                       /*   copy to matched quote */
        /* **** CHECK **** */
        if (p >= ST(memend))
          p = (char*) ((uintptr_t) p + expand_nxp(csound));
        /* **** END CHECK **** */
      }
      *p++ = c;
      goto blank;
    }
    while (1) {
      c = getscochar(csound, 1);
      /* else while legal chars, continue to bld string */
      /*      if (strchr("0123456789.+-eEnp<>()~", c) != NULL) { */
      if (isdigit(c) || c=='.' || c=='+' || c=='-' || c=='e' ||
          c=='E' || c=='n' || c=='p' || c=='<' || c=='>' || c=='(' ||
          c==')' || c=='~' || c=='z') {
        *p++ = c;
        /* **** CHECK **** */
        if (p >= ST(memend))
          p = (char*) ((uintptr_t) p + expand_nxp(csound));
        /* **** END CHECK **** */
      }
      else {                                /* any illegal is delimiter */
        ungetscochar(csound, c);
        break;
      }
    }
 blank:
    *p++ = SP;
    ST(nxp) = p;                            /*  add blank      */
    return(1);                              /*  and report ok  */
}

MYFLT stof(CSOUND *csound, char s[])            /* convert string to MYFLT  */
                                    /* (assumes no white space at beginning */
{                                   /*      but a blank or nl at end)       */
    char    *p;
    MYFLT   x = (MYFLT) strtod(s, &p);

    if (*p=='z') return FL(INF); /* Infinity or 7 years */
    if (UNLIKELY(s == p || !(*p == '\0' || isspace(*p)))) {
      csound->Message(csound, Str("sread: illegal number format:  "));
      p = s;
      while (!(*p == '\0' || isspace(*p))) {
        csound->Message(csound, "%c", *p);
        *p++ = '0';
      }
      csound->Message(csound,Str("   zero substituted.\n"));
      print_input_backtrace(csound, 1, csoundMessage);
      return FL(0.0);
    }
    return x;
}

