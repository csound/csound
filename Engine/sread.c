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

//#define MARGS   (3)
//#define MACDEBUG (1)

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

//#define ST(x)   (((SREAD_GLOBALS*) csound->sreadGlobals)->x)
#define STA(x)  (csound->sreadStatics.x)

static intptr_t expand_nxp(CSOUND *csound)
{
    char      *oldp;
    SRTBLK    *p;
    intptr_t  offs;
    size_t    nbytes;

    if (UNLIKELY(STA(nxp) >= (STA(memend) + MARGIN))) {
      csound->Die(csound, Str("sread:  text space overrun, increase MARGIN"));
      return 0;     /* not reached */
    }
    /* calculate the number of bytes to allocate */
    nbytes = (size_t) (STA(memend) - STA(curmem));
    nbytes = nbytes + (nbytes >> 3) + (size_t) (MEMSIZ - 1);
    nbytes &= ~((size_t) (MEMSIZ - 1));
    /* extend allocated memory */
    oldp = STA(curmem);
    STA(curmem) = (char*) csound->ReAlloc(csound, STA(curmem),
                                                 nbytes + (size_t) MARGIN);
    STA(memend) = (char*) STA(curmem) + (int32) nbytes;
    /* did the pointer change ? */
    if (STA(curmem) == oldp)
      return (intptr_t) 0;      /* no, nothing to do */
    /* correct all pointers for the change */
    offs = (intptr_t) ((uintptr_t) STA(curmem) - (uintptr_t) oldp);
    if (STA(bp) != NULL)
      STA(bp) = (SRTBLK*) ((uintptr_t) STA(bp) + (intptr_t) offs);
    if (STA(prvibp) != NULL)
      STA(prvibp) = (SRTBLK*) ((uintptr_t) STA(prvibp) + (intptr_t) offs);
    if (STA(sp) != NULL)
      STA(sp) = (char*) ((uintptr_t) STA(sp) + (intptr_t) offs);
    if (STA(nxp) != NULL)
      STA(nxp) = (char*) ((uintptr_t) STA(nxp) + (intptr_t) offs);
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
    IN_STACK  *curr = STA(str);
    char      *m, *lf = (needLFs ? "\n" : "");
    int       lastinput = 0;
    int       lastsource = 2; /* 2=current file, 1=macro, 0=#include */

    msgfunc(csound, Str("  section %d:  at position %d%s"), csound->sectcnt,
                    STA(linepos), lf);

    do {
      if (curr == STA(inputs)) lastinput = 1;
      if (UNLIKELY(!curr->mac || !curr->mac->name)){
        csound->Warning(csound, Str("Internal error in print_input_backtrace()"));
        return;
      }
      switch(lastsource) {
      case 0: m = Str("  included from line %d of macro %s%s"); break;
      case 1: m = Str("  called from line %d of macro %s%s"); break;
        //default:
      case 2: m = Str("  in line %d of macro %s%s"); break;
      }
      msgfunc(csound, m, (lastsource == 0 ? curr->line - 1 : curr->line),
              curr->mac->name, lf);  /* #include is one line before */
      if (lastinput && csound->oparms->useCsdLineCounts && csound->csdname) {
        /* print name & line # of CSD instead of temp sco */
        msgfunc(csound, m,
                (lastsource == 0 ? csound->scoLineOffset + curr->line - 1 :
                 csound->scoLineOffset + curr->line), csound->csdname, lf);
      }
      /* else { */
      /*   msgfunc(csound, m, (lastsource == 0 ? curr->line - 1 : curr->line), */
      /*     corfile_tell(curr->cf), lf);  /\* #include is one line before *\/ */
      /* } */
    } while (!lastsource);
    curr--;
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
    case '^': ans = POWER(a, b); break;
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
    S_MACRO *mm, *nn;
    int   i;

    if (strcmp(name, STA(macros)->name) == 0) {
      mm = STA(macros)->next;
      if (strcmp(STA(macros)->name, "[") != 0)
        corfile_rm(&(STA(macros)->body));
      csound->Free(csound, STA(macros)->name);
 #ifdef MACDEBUG
      csound->DebugMsg(csound,"%s(%d): corfile is %p\n",
                       __FILE__, __LINE__, STA(macros)->body);
 #endif
      for (i = 0; i < STA(macros)->acnt; i++)
        csound->Free(csound, STA(macros)->arg[i]);
      csound->Free(csound, STA(macros));
      STA(macros) = mm;
    }
    else {
      mm = STA(macros);
      nn = mm->next;
      while (strcmp(name, nn->name) != 0) {
        mm = nn; nn = nn->next;
        if (UNLIKELY(nn == NULL)) {
          scorerr(csound, Str("Undefining undefined macro"));
          return -1;
        }
      }
      csound->Free(csound, nn->name);
      corfile_rm(&nn->body);
      for (i = 0; i < nn->acnt; i++)
        csound->Free(csound, nn->arg[i]);
      mm->next = nn->next;
      csound->Free(csound, nn);
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
    corfile_ungetc(STA(str)->cf);
    STA(str)->cf->body[(STA(str)->cf)->p] = (char)c;
}

static int getscochar(CSOUND *csound, int expand)
{                   /* Read a score character, expanding macros if flag set */
    int     c;
 top:
    c = corfile_getc(STA(str)->cf);
    if (c == EOF) {
      if (STA(str) == &STA(inputs)[0]) {
        corfile_putc('\n', STA(str)->cf);  /* to ensure repeated EOF */
        return EOF;
      }
      if (STA(str)->mac == 0) {
        corfile_rm(&(STA(str)->cf)); /* No longer needed */
      }
      else {
        corfile_rewind(STA(str)->cf);
      }
      STA(pop) += STA(str)->args;
      STA(str)--; STA(input_cnt)--;
      goto top;
    }
#ifdef MACDEBUG
    csound->DebugMsg(csound,"%s(%d): character = %c(%.2d)\n",
                     __FILE__, __LINE__, c, c);
#endif
    if (c == '\r') {    /* can only occur in files, and not in macros */
      if ((c = corfile_getc(STA(str)->cf)) != '\n') {
        if (c == EOF)
          goto top;
        corfile_ungetc(STA(str)->cf);
      }
      c = '\n';
    }
    if (c == '\n') {
      STA(str)->line++; STA(linepos) = -1;
    }
    else STA(linepos)++;
    if (STA(ingappop) && STA(pop)) {
      do {
        if (STA(macros) != NULL) {
#ifdef MACDEBUG
          csound->Message(csound,"popping %s\n", STA(macros)->name);
#endif
          undefine_score_macro(csound, STA(macros)->name);
        }
        STA(pop)--;
      } while (STA(pop));
    }
    if (c == '$' && expand) {
      char      name[100];
      unsigned int i = 0;
      int       j;
      S_MACRO     *mm, *mm_save = NULL;
      STA(ingappop) = 0;
      while (isNameChar((c = getscochar(csound, 1)), (int) i)) {
        name[i++] = c; name[i] = '\0';
        mm = STA(macros);
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
          S_MACRO* nn = (S_MACRO*) csound->Malloc(csound, sizeof(S_MACRO));
          nn->name = csound->Malloc(csound, strlen(mm->arg[j])+1);
          strcpy(nn->name, mm->arg[j]);
#ifdef MACDEBUG
          csound->Message(csound,"defining argument %s ", nn->name);
#endif
          nn->body = corfile_create_w();
#ifdef MACDEBUG
          csound->DebugMsg(csound,"%s(%d): creating\n",
                           __FILE__, __LINE__, nn->body);
#endif
          while ((c = getscochar(csound, 1))!= term && c != trm1) {
            if (UNLIKELY(c==EOF))
              scorerr(csound, Str("Syntax error in macro call"));
            corfile_putc(c, nn->body);
          }
          corfile_rewind(nn->body);
#ifdef MACDEBUG
          csound->Message(csound,"as...#%s#\n", corfile_body(nn->body));
#endif
          nn->acnt = 0; /* No arguments for arguments */
          nn->next = STA(macros);
          STA(macros) = nn;
        }
      }
      STA(input_cnt)++;
      if (UNLIKELY(STA(input_cnt)>=STA(input_size))) {
        int old = STA(str)-STA(inputs);
        STA(input_size) += 20;
        STA(inputs) = csound->ReAlloc(csound, STA(inputs), STA(input_size)
                                                  * sizeof(IN_STACK));
        STA(str) = &STA(inputs)[old];     /* In case it moves */
      }
      STA(str)++;
      STA(str)->fd = STA(str)->cf = mm->body; STA(str)->args = mm->acnt;
      STA(str)->is_marked_repeat = 0;
      STA(str)->mac = mm; STA(str)->line = 1;
#ifdef MACDEBUG
      csound->Message(csound,
                      "Macro %s definded as >>%s<<\n",
                      mm->name, corfile_body(mm->body));
#endif
      STA(ingappop) = 1;
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
      CS_SPRINTF(buffer, "%f", *pv);
      {
        S_MACRO *nn = (S_MACRO*) csound->Malloc(csound, sizeof(S_MACRO));
        nn->name = csound->Malloc(csound, 2);
        strcpy(nn->name, "[");
        nn->body = corfile_create_r(buffer);
#ifdef MACDEBUG
        csound->DebugMsg(csound,"%s(%d): creating arg %p\n",
                         __FILE__, __LINE__, nn->body);
#endif
        nn->acnt = 0;   /* No arguments for arguments */
        nn->next = STA(macros);
        STA(macros) = nn;
        STA(input_cnt)++;
        if (UNLIKELY(STA(input_cnt)>=STA(input_size))) {
          int old = STA(str)-STA(inputs);
          STA(input_size) += 20;
          STA(inputs) = csound->ReAlloc(csound, STA(inputs), STA(input_size)
                                                    * sizeof(IN_STACK));
          STA(str) = &STA(inputs)[old];     /* In case it moves */
        }
        STA(str)++;
        STA(str)->cf = nn->body; STA(str)->args = 0;
        STA(str)->fd = NULL;
        STA(str)->is_marked_repeat = 0;
        STA(str)->mac = NULL; STA(str)->line = 1;
#ifdef MACDEBUG
        csound->Message(csound,"[] defined as >>%s<<\n", corfile_body(nn->body));
#endif
        STA(ingappop) = 1;
        goto top;
      }
    }
    return c;
}

static int nested_repeat(CSOUND *csound)                /* gab A9*/
{
    STA(repeat_cnt_n)[STA(repeat_index)]--;
    if (STA(repeat_cnt_n)[STA(repeat_index)] == 0) {      /* Expired */
      if (STA(repeat_index) > 1) {
        char c[41];
        int j;
        memset(c, '\0', 41);
        for (j = 0; j<STA(repeat_index); j++) {
          c[j]=' ';
          c[j+1]='\0';
        }
        if (csound->oparms->odebug/*csound->oparms->msglevel & TIMEMSG*/)
          csound->Message(csound,Str("%s Nested LOOP terminated, level:%d\n"),
                          c,STA(repeat_index));

      }
      else {
        if (csound->oparms->odebug/*csound->oparms->msglevel & TIMEMSG*/)
          csound->Message(csound,Str("External LOOP terminated, level:%d\n"),
                          STA(repeat_index));
      }
      undefine_score_macro(csound, STA(repeat_name_n)[STA(repeat_index)]);
      STA(repeat_index)--;
    }
    else {
      int i;
      corfile_set(STA(str)->cf,STA(repeat_point_n)[STA(repeat_index)]);
      sscanf(corfile_current(STA(repeat_mm_n)[STA(repeat_index)]->body),
             "%d", &i);
#ifdef MACDEBUG
      csound->DebugMsg(csound,"%s(%d) reset point to %d\n", __FILE__, __LINE__,
             STA(repeat_point_n)[STA(repeat_index)], i);
      csound->DebugMsg(csound,"%s(%d) corefile: %s %d %d\n", __FILE__, __LINE__,
             STA(repeat_mm_n)[STA(repeat_index)]->body->body,
             STA(repeat_mm_n)[STA(repeat_index)]->body->p,
             STA(repeat_mm_n)[STA(repeat_index)]->body->len);
#endif
      i = i + STA(repeat_inc_n);
      {
        char buffer[128];
        memset(buffer, '\0', 128);
        snprintf(buffer, 128, "%d", i);
#ifdef MACDEBUG
        csound->DebugMsg(csound,"%s(%d) new i = %s\n",
                         __FILE__, __LINE__,  buffer);
#endif
        corfile_reset(STA(repeat_mm_n)[STA(repeat_index)]->body);
        corfile_puts(buffer, STA(repeat_mm_n)[STA(repeat_index)]->body);
        corfile_rewind(STA(repeat_mm_n)[STA(repeat_index)]->body);
#ifdef MACDEBUG
        csound->DebugMsg(csound,"%s(%d) corefile: %s %d %d\n", __FILE__, __LINE__,
               STA(repeat_mm_n)[STA(repeat_index)]->body->body,
               STA(repeat_mm_n)[STA(repeat_index)]->body->p,
               STA(repeat_mm_n)[STA(repeat_index)]->body->len);
#endif
      }
      if (STA(repeat_index) > 1) {
        char c[41];
        int j;
         memset(c, '\0', 41);
        for (j = 0; j<STA(repeat_index); j++) {
          c[j]=' ';
          c[j+1]='\0';
        }
        if (csound->oparms->odebug/*csound->oparms->msglevel & TIMEMSG*/)
          csound->Message(csound,Str("%s  Nested LOOP section (%d) Level:%d\n"),
                          c, i, STA(repeat_index));
      }
      else {
        if (csound->oparms->odebug/*csound->oparms->msglevel & TIMEMSG*/)
          csound->Message(csound,Str(" External LOOP section (%d) Level:%d\n"),
                          i, STA(repeat_index));
      }
      return 1;
    }
    return 0;
}

static int do_repeat(CSOUND *csound)
{                               /* At end of section repeat if necessary */
    STA(repeat_cnt)--;
    if (STA(repeat_cnt) == 0) {  /* Expired */
      /* Delete macro (assuming there is any) */
      if (csound->oparms->msglevel & TIMEMSG)
        csound->Message(csound, Str("Loop terminated\n"));
      if (STA(repeat_name)[0] != '\0')
        undefine_score_macro(csound, STA(repeat_name));
      STA(repeat_name)[0] = '\0';
    }
    else {
      int i, n;
      corfile_set(STA(str)->cf, STA(repeat_point));
      if (STA(repeat_name)[0] != '\0') {
        sscanf(corfile_current(STA(repeat_mm)->body), "%d%n", &i, &n);
        i = i + STA(repeat_inc);
        corfile_seek(STA(repeat_mm)->body, n, SEEK_CUR);
        {
          char buffer[128];
          snprintf(buffer, 128, "%d", i);
          corfile_reset(STA(repeat_mm)->body);
          corfile_puts(buffer, STA(repeat_mm)->body);
          corfile_rewind(STA(repeat_mm)->body);
        }
        if (csound->oparms->msglevel & TIMEMSG)
          csound->Message(csound, Str("Repeat section (%d)\n"), i);
      }
      else
        csound->Message(csound, Str("Repeat section\n"));
      /* replace 'e' or 'r' with 's' and end section */
      STA(bp)->text[0] = 's';
      STA(clock_base) = FL(0.0);
      STA(warp_factor) = FL(1.0);
      STA(prvp2) = -FL(1.0);
      return 1;
    }
    return 0;
}

static void init_smacros(CSOUND *csound, NAMES *nn)
{
    S_MACRO *mm;
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
      mname = (char*) csound->Malloc(csound, (p - s) + 1);
      strncpy(mname, s, p - s);
      mname[p - s] = '\0';
      /* check if macro is already defined */
      for (mm = STA(macros); mm != NULL; mm = mm->next) {
        if (strcmp(mm->name, mname) == 0)
          break;
      }
      if (mm == NULL) {
        mm = (S_MACRO*) csound->Calloc(csound, sizeof(S_MACRO));
        mm->name = mname;
        mm->next = STA(macros);
        STA(macros) = mm;
      }
      else
        csound->Free(csound, mname);
      mm->margs = MARGS;    /* Initial size */
      mm->acnt = 0;
      if (*p != '\0')
        p++;
      mm->body = corfile_create_r(p);
#ifdef MACDEBUG
      csound->DebugMsg(csound,"%s(%d): init %s %p\n",
                       __FILE__, __LINE__, mm->name, mm->body);
#endif
      nn = nn->next;
    }
    mm = (S_MACRO*) csound->Calloc(csound, sizeof(S_MACRO));
    mm->name = (char*)csound->Malloc(csound,4);
    strcpy(mm->name, "INF");
    mm->body = corfile_create_r("800000000000.0");
#ifdef MACDEBUG
    csound->DebugMsg(csound,"%s(%d): INF %p\n", __FILE__, __LINE__, mm->body);
#endif
    mm->next = STA(macros);
    STA(macros) = mm;
}

#if never
void sread_init(CSOUND *csound)
{
    /* sread_alloc_globals(csound); */
    STA(inputs) = (IN_STACK*) csound->Malloc(csound, 20 * sizeof(IN_STACK));
    STA(input_size) = 20;
    STA(input_cnt) = 0;
    STA(str) = STA(inputs);
    STA(str)->fd = NULL;
    STA(str)->cf = csound->scstr;
    STA(str)->is_marked_repeat = 0;
    STA(str)->line = 1; STA(str)->mac = NULL;
    init_smacros(csound, csound->smacros);
}
#endif

void sread_initstr(CSOUND *csound, CORFIL *sco)
{
    /* sread_alloc_globals(csound); */
    STA(inputs) = (IN_STACK*) csound->Malloc(csound, 20 * sizeof(IN_STACK));
    STA(input_size) = 20;
    STA(input_cnt) = 0;
    STA(str) = STA(inputs);
    STA(str)->fd = NULL;
    STA(str)->fd =  STA(str)->cf = sco;
    STA(str)->is_marked_repeat = 0;
    STA(str)->line = 1; STA(str)->mac = NULL;
    init_smacros(csound, csound->smacros);
}

int sread(CSOUND *csound)       /*  called from main,  reads from SCOREIN   */
{                               /*  each score statement gets a sortblock   */
    int  rtncod;                /* return code to calling program:      */
                                /*   1 = section read                   */
                                /*   0 = end of file                    */
    /* sread_alloc_globals(csound); */
    STA(bp) = STA(prvibp) = csound->frstbp = NULL;
    STA(nxp) = NULL;
    STA(warpin) = 0;
    STA(lincnt) = 1;
    csound->sectcnt++;
    rtncod = 0;
    salcinit(csound);           /* init the mem space for this section  */
#ifdef SCORE_PARSER
    if (csound->score_parser) {
      extern int scope(CSOUND*);
      printf("**********************************************************\n");
      printf("*******************EXPERIMENTAL CODE**********************\n");
      printf("**********************************************************\n");
      scope(csound);
      exit(0);
    }
#endif

    while ((STA(op) = getop(csound)) != EOF) { /* read next op from scorefile */
      rtncod = 1;
      salcblk(csound);          /* build a line structure; init bp,nxp  */
    again:
      switch (STA(op)) {         /*  and dispatch on opcodes             */
      case 'y':
        {
          char  *p = &(STA(bp)->text[1]);
          char q;
          //char *old_nxp = STA(nxp)-2;
          //printf("text=%s<<\n", STA(bp)->text);
          /* Measurement shows isdigit and 3 cases is about 30% */
          /* faster than use of strchr (measured on Suse9.3)    */
          /*         if (strchr("+-.0123456789", *p) != NULL) { */
          while ((q=getscochar(csound,1))!='\n') *p++ = q;
          *p = '\0';
          //printf("text=%s<<\n", STA(bp)->text);
          p = &(STA(bp)->text[1]);
          while (isblank(q=*p)) p++;
          if (isdigit(q) || q=='+' || q=='-' || q=='.') {
            double  tt;
            char    *tmp = p;
            tt = cs_strtod(p, &tmp);
            //printf("tt=%lf q=%c\n", tt, q);
            csound->randSeed1 = (int)tt;
            printf("seed from score %d\n", csound->randSeed1);
          }
          else {
            uint32_t tmp = (uint32_t) csound->GetRandomSeedFromTime();
            while (tmp >= (uint32_t) 0x7FFFFFFE)
              tmp -= (uint32_t) 0x7FFFFFFE;
            csound->randSeed1 = tmp+1;
            printf("seed from clock %d\n", csound->randSeed1);
          }
          //printf("cleaning up\n");
          break;
          //q = STA(op) = getop(csound);
          //printf("next op = %c(%.2x)\n", q, q);
          //goto again;
        }
      case 'i':
      case 'f':
      case 'a':
      case 'q':
        ifa(csound);
        break;
      case 'w':
        STA(warpin)++;
        copypflds(csound);
        break;
      case 't':
        copypflds(csound);
        break;
      case 'b': /* Set a clock base */
        {
          char *old_nxp = STA(nxp)-2;
          getpfld(csound);
          STA(clock_base) = stof(csound, STA(sp));
          if (csound->oparms->msglevel & TIMEMSG)
            csound->Message(csound,Str("Clockbase = %f\n"), STA(clock_base));
          flushlin(csound);
          STA(op) = getop(csound);
          STA(nxp) = old_nxp;
          *STA(nxp)++ = STA(op); /* Undo this line */
          STA(nxp)++;
          goto again;
        }
      case 's':
      case 'e':
        /* check for optional p1 before doing repeats */
        copylin(csound);
        {
          char  *p = &(STA(bp)->text[1]);
          char q;
          while (isblank(*p))
            p++;
          /* Measurement shows isdigit and 3 cases is about 30% */
          /* faster than use of strchr (measured on Suse9.3)    */
          /*         if (strchr("+-.0123456789", *p) != NULL) { */
          q = *p;
          if (isdigit(q) || q=='+' || q=='-' || q=='.') {
            double  tt;
            char    *tmp = p;
            tt = cs_strtod(p, &tmp);
            if (tmp != p && (*tmp == '\0' || isspace(*tmp))) {
              STA(bp)->pcnt = 1;
              STA(bp)->p1val = STA(bp)->p2val = STA(bp)->newp2 = (MYFLT) tt;
            }
          }
          else STA(bp)->p1val = STA(bp)->p2val = STA(bp)->newp2 = FL(0.0);
        }
        /* If we are in a repeat of a marked section ('n' statement),
           we must pop those inputs before doing an 'r' repeat. */
        while (STA(str)->is_marked_repeat && STA(input_cnt) > 0) {
          /* close all marked repeat inputs */
          /* if (STA(str)->fd != NULL) { */
          /*   csound->FileClose(csound, STA(str)->fd); STA(str)->fd = NULL; */
          /* } */
          corfile_rm(&(STA(str)->cf));
          STA(str)--; STA(input_cnt)--;
        }
        if (STA(repeat_cnt) != 0) {
          if (do_repeat(csound))
            return rtncod;
        }
        if (STA(op) != 'e') {
          STA(clock_base) = FL(0.0);
          STA(warp_factor) = FL(1.0);
          STA(prvp2) = -FL(1.0);
        }
        return rtncod;
      case '}':
        {
          int temp;
          char *old_nxp = STA(nxp)-2;
          if ((temp=STA(repeat_cnt_n)[STA(repeat_index)])!=0)
            nested_repeat(csound);
          STA(op) = getop(csound);
          STA(nxp) = old_nxp;
          *STA(nxp)++ = STA(op);
          STA(nxp)++;
          goto again;
        }
      case '{':
        {
          char *old_nxp = STA(nxp)-2;
          int c, i;
          STA(repeat_index)++;
          if (UNLIKELY(STA(repeat_index) >= RPTDEPTH))
            scorerr(csound, Str("Loops are nested too deeply"));
          STA(repeat_mm_n)[STA(repeat_index)] =
            (S_MACRO*)csound->Malloc(csound, sizeof(S_MACRO));
          STA(repeat_cnt_n)[STA(repeat_index)] = 0;
          do {
            c = getscochar(csound, 1);
          } while(isblank(c));
          while (isdigit(c)) {
            STA(repeat_cnt_n)[STA(repeat_index)] =
              10 * STA(repeat_cnt_n)[STA(repeat_index)] + c - '0';
            c = getscochar(csound, 1);
          }
          if (UNLIKELY(STA(repeat_cnt_n)[STA(repeat_index)] <= 0
                       || !isspace(c))) // != ' ' && c != '\t' && c != '\n')))
            scorerr(csound, Str("{: invalid repeat count"));
          if (STA(repeat_index) > 1) {
            char st[41];
            int j;
            for (j = 0; j < STA(repeat_index); j++) {
              st[j] = ' ';
              st[j+1] = '\0';
            }
            if (csound->oparms->odebug/*csound->oparms->msglevel & TIMEMSG*/)
              csound->Message(csound, Str("%s Nested LOOP=%d Level:%d\n"),
                              st, STA(repeat_cnt_n)[STA(repeat_index)],
                              STA(repeat_index));
          }
          else {
            if (csound->oparms->odebug/*csound->oparms->msglevel & TIMEMSG*/)
              csound->Message(csound, Str("External LOOP=%d Level:%d\n"),
                              STA(repeat_cnt_n)[STA(repeat_index)],
                              STA(repeat_index));
          }
          while (isblank(c)) {
            c = getscochar(csound, 1);
          }
          for (i = 0; isNameChar(c, i) && i < (NAMELEN-1); i++) {
            STA(repeat_name_n)[STA(repeat_index)][i] = c;
            c = getscochar(csound, 1);
          }
          STA(repeat_name_n)[STA(repeat_index)][i] = '\0';
          ungetscochar(csound, c);
          /* Define macro for counter */
          STA(repeat_mm_n)[STA(repeat_index)]->name =
            csound->Malloc(csound, strlen(STA(repeat_name_n)[STA(repeat_index)])+1);
          strcpy(STA(repeat_mm_n)[STA(repeat_index)]->name,
                 STA(repeat_name_n)[STA(repeat_index)]);
          STA(repeat_mm_n)[STA(repeat_index)]->acnt = 0;
          STA(repeat_mm_n)[STA(repeat_index)]->body = corfile_create_r("0");
#ifdef MACDEBUG
          csound->DebugMsg(csound,"%s(%d): repeat %s zero %p\n", __FILE__, __LINE__,
                 STA(repeat_name_n)[STA(repeat_index)],
                 STA(repeat_mm_n)[STA(repeat_index)]->body);
#endif
          STA(repeat_mm_n)[STA(repeat_index)]->next = STA(macros);
          STA(macros) = STA(repeat_mm_n)[STA(repeat_index)];
          flushlin(csound);     /* Ignore rest of line */
          STA(repeat_point_n)[STA(repeat_index)] =
            corfile_tell(STA(str)->cf);

          /* { does not start a new section - akozar */
          /* STA(clock_base) = FL(0.0);
          STA(warp_factor) = FL(1.0);
          STA(prvp2) = -FL(1.0); */
          STA(op) = getop(csound);
          STA(nxp) = old_nxp;
          *STA(nxp)++ = STA(op);
          STA(nxp)++;
          goto again;
        }
      case 'r':                 /* For now treat as s */
                                /* First deal with previous section */
        /* If we are in a repeat of a marked section ('n' statement),
           we must pop those inputs before doing an 'r' repeat. */
        if (STA(str)->is_marked_repeat) {
          while (STA(str)->is_marked_repeat && STA(input_cnt) > 0) {
            /* close all marked repeat inputs */
            /* if (STA(str)->fd != NULL) { */
            /*   csound->FileClose(csound, STA(str)->fd); STA(str)->fd = NULL; */
            /* } */
            corfile_rm(&(STA(str)->cf));
            STA(str)--; STA(input_cnt)--;
          }
          /* last time thru an 'r', cleanup up 'r' before finishing 'n' */
          if (STA(repeat_cnt) == 1)  do_repeat(csound);
          if (STA(repeat_cnt) == 0) {
            /* replace with 's' and end section if no previous 'r'
               or just finished an 'r' loop */
            STA(bp)->text[0] = 's';
            STA(clock_base) = FL(0.0);
            STA(warp_factor) = FL(1.0);
            STA(prvp2) = -FL(1.0);
            return rtncod;
          }
        }
        if (STA(repeat_cnt) != 0) {
          if (do_repeat(csound))
            return rtncod;
        }
        /* Then remember this state */
        *(STA(nxp)-2) = 's'; *STA(nxp)++ = LF;
        if (STA(nxp) >= STA(memend))              /* if this memblk exhausted */
          expand_nxp(csound);
        {
          int   c, i;
          STA(repeat_cnt) = 0;
          do {
            c = getscochar(csound, 1);
          } while (isblank(c));
          while (isdigit(c)) {
            STA(repeat_cnt) = 10 * STA(repeat_cnt) + c - '0';
            c = getscochar(csound, 1);
          }
          if (UNLIKELY(STA(repeat_cnt) <= 0 || !isspace(c)))
            //(c != ' ' && c != '\t' && c != '\n')
            scorerr(csound, Str("r: invalid repeat count"));
          if (csound->oparms->msglevel & TIMEMSG)
            csound->Message(csound, Str("Repeats=%d\n"), STA(repeat_cnt));
          while (isblank(c)) {
            c = getscochar(csound, 1);
          }
          for (i = 0; isNameChar(c, i) && i < (NAMELEN-1); i++) {
            STA(repeat_name)[i] = c;
            c = getscochar(csound, 1);
          }
          STA(repeat_name)[i] = '\0';
          ungetscochar(csound, c);
          flushlin(csound);     /* Ignore rest of line */
          if (i) {
            /* Only if there is a name: define macro for counter */
            STA(repeat_mm) = (S_MACRO*) csound->Malloc(csound, sizeof(S_MACRO));
            STA(repeat_mm)->name = csound->Malloc(csound,
                                                  strlen(STA(repeat_name)) + 1);
            strcpy(STA(repeat_mm)->name, STA(repeat_name));
            STA(repeat_mm)->acnt = 0;
            STA(repeat_mm)->body = corfile_create_r("1");
#ifdef MACDEBUG
            csound->DebugMsg(csound,"%s(%d): 1 %p\n",
                             __FILE__, __LINE__,STA(repeat_mm)->body);
#endif
            STA(repeat_mm)->next = STA(macros);
            STA(macros) = STA(repeat_mm);
          }
          STA(repeat_point) = corfile_tell(STA(str)->cf);
        }
        STA(clock_base) = FL(0.0);
        STA(warp_factor) = FL(1.0);
        STA(prvp2) = -FL(1.0);
        return rtncod;
      case 'm': /* Remember this place */
        {
          char  *old_nxp = STA(nxp)-2;
          char  buff[200];
          int   c;
          int   i = 0, j;
          while (isblank(c = getscochar(csound, 1)));
          while (isNameChar(c, i)) {
            buff[i++] = c;
            c = getscochar(csound, 1);
          }
          buff[i] = '\0';
          if (c != '\n' && c != EOF) flushlin(csound);
          if (csound->oparms->msglevel & TIMEMSG)
            csound->Message(csound,Str("Named section >>>%s<<<\n"), buff);
          for (j=0; j<=STA(next_name); j++)
            if (strcmp(buff, STA(names)[j].name)==0) break;
          if (j>STA(next_name)) {
            j = ++STA(next_name);
            STA(names)[j].name = (char*)csound->Malloc(csound, i+1);
            strcpy(STA(names)[j].name, buff);
          }
          else csound->Free(csound, STA(names)[j].file);
          STA(names)[STA(next_name)].posit = corfile_tell(STA(str)->cf);
          STA(names)[STA(next_name)].line = STA(str)->line;
          STA(names)[STA(next_name)].file =
            csound->Malloc(csound, strlen(corfile_body(STA(str)->cf)) + 1);
          strcpy(STA(names)[STA(next_name)].file, corfile_body(STA(str)->cf));
          if (csound->oparms->msglevel & TIMEMSG)
            csound->Message(csound,Str("%d: File %s position %ld\n"),
                            STA(next_name), STA(names)[STA(next_name)].file,
                            STA(names)[STA(next_name)].posit);
          STA(op) = getop(csound);
          STA(nxp) = old_nxp;
          *STA(nxp)++ = STA(op); /* Undo this line */
          STA(nxp)++;
          goto again;           /* suggested this loses a line?? */
        }
      case 'n':
        {
          char *old_nxp = STA(nxp)-2;
          char buff[200];
          int c;
          int i = 0;
          while (isblank(c = getscochar(csound, 1)));
          while (isNameChar(c, i)) {
            buff[i++] = c;
            c = getscochar(csound, 1);
          }
          buff[i] = '\0';
          if (c != '\n' && c != EOF) flushlin(csound);
          for (i = 0; i<=STA(next_name); i++)
            if (strcmp(buff, STA(names)[i].name)==0) break;
          if (UNLIKELY(i > STA(next_name)))
            sreaderr(csound, Str("Name %s not found"), buff);
          else {
            csound->Message(csound, Str("Duplicate %d: %s (%s,%ld)\n"),
                            i, buff, STA(names)[i].file, STA(names)[i].posit);
            STA(input_cnt)++;
            if (STA(input_cnt)>=STA(input_size)) {
              int old = STA(str)-STA(inputs);
              STA(input_size) += 20;
              STA(inputs) = csound->ReAlloc(csound, STA(inputs),
                                            STA(input_size) * sizeof(IN_STACK));
              STA(str) = &STA(inputs)[old];     /* In case it moves */
            }
            STA(str)++;
            STA(str)->is_marked_repeat = 1;
            STA(str)->cf = corfile_create_r(STA(names)[i].file);
//            STA(str)->cf = corfile_create_r(csound->GetFileName(STA(str)->fd));
            STA(str)->line = STA(names)[i].line;
            corfile_set(STA(str)->cf, STA(names)[i].posit);
          }
          STA(op) = getop(csound);
          STA(nxp) = old_nxp;
          *STA(nxp)++ = STA(op); /* Undo this line */
          STA(nxp)++;
          goto again;
        }
      case 'v': /* Suggestion of Bryan Bales */
        {       /* Set local variability of time */
          char *old_nxp = STA(nxp)-2;
          getpfld(csound);
          STA(warp_factor) = stof(csound, STA(sp));
          if (csound->oparms->msglevel & TIMEMSG)
            csound->Message(csound, Str("Warp_factor = %f\n"), STA(warp_factor));
          flushlin(csound);
          STA(op) = getop(csound);
          STA(nxp) = old_nxp;
          *STA(nxp)++ = STA(op);          /* Undo this line */
          STA(nxp)++;
          goto again;
        }
      case 'x':                         /* Skip section */
        while (1) {
          switch (STA(op) = getop(csound)) {
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
      case -1:
        break;
      default:
        csound->Message(csound,
                        Str("sread is confused on legal opcodes %c(%.2x)\n"),
                        STA(op), STA(op));
        break;
      }
    }
 ending:
    if (STA(repeat_cnt) > 0) {
      STA(op) = 'e';
      salcblk(csound);
      if (do_repeat(csound))
        return rtncod;
      *STA(nxp)++ = LF;
    }
    if (!rtncod) {                      /* Ending so clear macros */
      while (STA(macros) != NULL) {
        undefine_score_macro(csound, STA(macros)->name);
      }
    }
    return rtncod;
}

static void copylin(CSOUND *csound)     /* copy source line to srtblk   */
{
    int c;
    STA(nxp)--;
    if (STA(nxp) >= STA(memend))          /* if this memblk exhausted */
      expand_nxp(csound);
    do {
      c = getscochar(csound, 1);
      *STA(nxp)++ = c;
    } while (c != LF && c != EOF);
    if (c == EOF) *(STA(nxp)-1) = '\n';  /* Avoid EOF characters */
    STA(lincnt)++;
    STA(linpos) = 0;
}

static void copypflds(CSOUND *csound)
{
    STA(bp)->pcnt = 0;
    while (getpfld(csound))     /* copy each pfield,    */
      STA(bp)->pcnt++;           /* count them,          */
    *(STA(nxp)-1) = LF;          /* terminate with newline */
}

static void ifa(CSOUND *csound)
{
    SRTBLK *prvbp;
    int n, nocarry = 0;

    STA(bp)->pcnt = 0;
    while (getpfld(csound)) {   /* while there's another pfield,  */
      nocarry = 0;
      ++STA(bp)->pcnt;
      /* if (UNLIKELY(++STA(bp)->pcnt == PMAX)) { */
      /*   sreaderr(csound, Str("instr pcount exceeds PMAX")); */
      /*   csound->Message(csound, Str("      remainder of line flushed\n")); */
      /*   flushlin(csound); */
      /*   continue; */
      /* } */
      if (*STA(sp) == '^' && STA(op) == 'i' && STA(bp)->pcnt == 2) {
        int foundplus = 0;
        if (*(STA(sp)+1)=='+') { STA(sp)++; foundplus = 1; }
        if (UNLIKELY(STA(prvp2)<0)) {
          sreaderr(csound,Str("No previous event for ^"));
          STA(prvp2) = STA(bp)->p2val = STA(warp_factor) * stof(csound, STA(sp)+1);
        }
        else if (UNLIKELY(isspace(*(STA(sp)+1)))) {
          /* stof() assumes no leading whitespace -- 070204, akozar */
          sreaderr(csound, Str("illegal space following %s, zero substituted"),
                           (foundplus ? "^+" : "^"));
          STA(prvp2) = STA(bp)->p2val = STA(prvp2);
        }
        else STA(prvp2) = STA(bp)->p2val =
                         STA(prvp2) + STA(warp_factor) * stof(csound, STA(sp) + 1);
      }
      else if (STA(nxp)-STA(sp) == 2 && (*STA(sp) == '.' || *STA(sp) == '+')) {
        if (STA(op) == 'i'
            && (*STA(sp) == '.' || STA(bp)->pcnt == 2)
            && ((STA(bp)->pcnt >= 2 && (prvbp = STA(prvibp)) != NULL
                 && STA(bp)->pcnt <= prvbp->pcnt)
                || (STA(bp)->pcnt == 1 && (prvbp = STA(bp)->prvblk) != NULL
                    && prvbp->text[0] == 'i'))) {
          if (*STA(sp) == '.') {
            STA(nxp) = STA(sp);
            pcopy(csound, (int) STA(bp)->pcnt, 1, prvbp);
            if (STA(bp)->pcnt >= 2) STA(prvp2) = STA(bp)->p2val;
          }
          else /* need the fabs() in case of neg p3 */
            STA(prvp2) = STA(bp)->p2val =
                        prvbp->p2val + FABS(prvbp->p3val);
        }
        else carryerror(csound);
      }
      else if (*STA(sp) == '!') {
        int getmore = 0;
        if (UNLIKELY(STA(op) != 'i')) {
          *(STA(nxp)-1) = '\0';
          getmore = 1;
          sreaderr(csound, Str("ignoring '%s' in '%c' event"), STA(sp), STA(op));
        }
        else if (UNLIKELY(STA(bp)->pcnt < 4)) {
          sreaderr(csound, Str("! invalid in p1, p2, or p3"));
          csound->Message(csound, Str("      remainder of line flushed\n"));
          flushlin(csound);
        }
        else if (UNLIKELY(STA(nxp)-STA(sp) != 2)) {
          sreaderr(csound, Str("illegal character after !: '%c'"), *(STA(sp)+1));
          csound->Message(csound, Str("      remainder of line flushed\n"));
          flushlin(csound);
        }
        else {
          nocarry = 1;         /* only set when no syntax errors */
          flushlin(csound);
        }
        /* but always delete the pfield beginning with '!' */
        STA(nxp) = STA(sp);
        STA(bp)->pcnt--;
        if (getmore) continue; /* not the best, but not easy to delete event */
                               /* since ifa() doesn't return anything */
        else break;
      }
      else switch (STA(bp)->pcnt) {      /*  watch for p1,p2,p3, */
      case 1:                           /*   & MYFLT, setinsno..*/
        if ((STA(op) == 'i' || STA(op) == 'q') && *STA(sp) == '"') {
        /*   csound->DebugMsg(csound,"***Entering second dubious code scnt=%d\n",
                                      csound->scnt0); */
          STA(bp)->p1val = SSTRCOD;      /* allow string name */
        }
        else
          STA(bp)->p1val = stof(csound, STA(sp));
        if (STA(op) == 'i')
          setprv(csound);
        else STA(prvibp) = NULL;
        break;
      case 2: STA(prvp2) = STA(bp)->p2val =
                          STA(warp_factor)*stof(csound, STA(sp)) + STA(clock_base);
        break;
      case 3: if (STA(op) == 'i')
                STA(bp)->p3val = STA(warp_factor) * stof(csound, STA(sp));
              else STA(bp)->p3val = stof(csound, STA(sp));
      break;
      default:break;
      }
      switch (STA(bp)->pcnt) {               /* newp2, newp3:   */
      case 2: if (STA(warpin)) {             /* for warpin,     */
        getpfld(csound);                    /*   newp2 follows */
        STA(bp)->newp2 = STA(warp_factor) * stof(csound, STA(sp)) + STA(clock_base);
        STA(nxp) = STA(sp);                   /*    (skip text)  */
      }
      else STA(bp)->newp2 = STA(bp)->p2val;   /* else use p2val  */
      break;
      case 3: if (STA(warpin) && (STA(op) == 'i' || STA(op) == 'f')) {
        getpfld(csound);                    /* same for newp3  */
        STA(bp)->newp3 = STA(warp_factor) * stof(csound, STA(sp));
        STA(nxp) = STA(sp);
      }
      else STA(bp)->newp3 = STA(bp)->p3val;
      break;
      }
    }
    if (STA(op) == 'i' && !nocarry &&    /* then carry any rem pflds */
        ((prvbp = STA(prvibp)) != NULL ||
         (!STA(bp)->pcnt && (prvbp = STA(bp)->prvblk) != NULL &&
          prvbp->text[0] == 'i')) &&
        (n = prvbp->pcnt - STA(bp)->pcnt) > 0) {
      pcopy(csound, (int) STA(bp)->pcnt + 1, n, prvbp);
      STA(bp)->pcnt += n;
    }
    *(STA(nxp)-1) = LF;                  /* terminate this stmnt with newline */
}

static void setprv(CSOUND *csound)      /*  set insno = (int) p1val         */
{                                       /*  prvibp = prv note, same insno   */
    SRTBLK *p = STA(bp);
    int16 n;

    if (ISSTRCOD(STA(bp)->p1val) && *STA(sp) == '"') {   /* IV - Oct 31 2002 */
      char name[MAXNAME], *c, *s = STA(sp);
      /* unquote instrument name */
      c = name; while (*++s != '"') *c++ = *s; *c = '\0';
      /* find corresponding insno */
      if (UNLIKELY(!(n = (int16) named_instr_find(csound, name)))) {
        csound->Message(csound, Str("WARNING: instr %s not found, "
                                    "assuming insno = -1\n"), name);
        n = -1;
      }
    }
    else n = (int16) STA(bp)->p1val;         /* set current insno */
    STA(bp)->insno = n;

    while ((p = p->prvblk) != NULL)
      if (p->insno == n) {
        STA(prvibp) = p;                     /* find prev same */
        return;
      }
    STA(prvibp) = NULL;                      /*  if there is one */
}

static void carryerror(CSOUND *csound)      /* print offending text line  */
{                                           /*      (partial)             */
    char *p;

    csound->Message(csound, Str("sread: illegal use of carry, "
                                "  0 substituted\n"));
    *(STA(nxp) - 3) = SP;
    p = STA(bp)->text;
    while (p <= STA(nxp) - 2)
      csound->Message(csound, "%c", *p++);
    csound->Message(csound, "<=\n");
    print_input_backtrace(csound, 1, csoundMessage);
    *(STA(nxp) - 2) = '0';
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
    p = STA(nxp);
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
      case 1: STA(bp)->p1val = prvbp->p1val;       /*  with p1-p3 vals */
        setprv(csound);
        break;
      case 2: if (*(p-2) == '+')              /* (interpr . of +) */
        STA(prvp2) = STA(bp)->p2val = prvbp->p2val + FABS(prvbp->p3val);
      else STA(prvp2) = STA(bp)->p2val = prvbp->p2val;
      STA(bp)->newp2 = STA(bp)->p2val;
      break;
      case 3: STA(bp)->newp3 = STA(bp)->p3val = prvbp->p3val;
        break;
      default:break;
      }
      STA(bp)->lineno = prvbp->lineno;
      pfno++;
    }
    STA(nxp) = p;                                /* adjust globl nxp pntr */
}

static void salcinit(CSOUND *csound)
{                             /* init the sorter mem space for a new section */
    if (STA(curmem) == NULL) { /*  alloc 1st memblk if nec; init *nxp to this */
      STA(curmem) = (char*) csound->Malloc(csound, (size_t) (MEMSIZ + MARGIN));
      STA(memend) = (char*) STA(curmem) + MEMSIZ;
    }
    STA(nxp) = (char*) STA(curmem);
}

static void salcblk(CSOUND *csound)
{                               /* alloc a srtblk from current mem space:   */
    SRTBLK  *prvbp;             /*   align following *nxp, set new bp, nxp  */
                                /*   set srtblk lnks, put op+blank in text  */
    if (STA(nxp) >= STA(memend))          /* if this memblk exhausted */
      expand_nxp(csound);
    /* now allocate a srtblk from this space: */
    prvbp = STA(bp);
    STA(bp) = (SRTBLK*) (((uintptr_t) STA(nxp) + (uintptr_t)7) & ~((uintptr_t)7));
    if (csound->frstbp == NULL)
      csound->frstbp = STA(bp);
    if (prvbp != NULL)
      prvbp->nxtblk = STA(bp);           /* link with prev srtblk        */
    STA(bp)->nxtblk = NULL;
    STA(bp)->prvblk = prvbp;
    STA(bp)->insno = 0;
    STA(bp)->pcnt = 0;
    STA(bp)->lineno = STA(lincnt);
    STA(nxp) = &(STA(bp)->text[0]);
    *STA(nxp)++ = STA(op);                /* place op, blank into text    */
    *STA(nxp)++ = SP;
    *STA(nxp) = '\0';
}

void sfree(CSOUND *csound)       /* free all sorter allocated space */
{                                /*    called at completion of sort */
    /* sread_alloc_globals(csound); */
    if (STA(curmem) != NULL) {
      csound->Free(csound, STA(curmem));
      STA(curmem) = NULL;
    }
    while (STA(str) != &STA(inputs)[0]) {
      corfile_rm(&(STA(str)->cf));
      STA(str)--;
    }
    corfile_rm(&(csound->scorestr));
}

static void flushlin(CSOUND *csound)
{                                   /* flush input to end-of-line; inc lincnt */
    int c;
    while ((c = getscochar(csound, 0)) != LF && c != EOF)
      ;
    STA(linpos) = 0;
    STA(lincnt)++;
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
    while (isblank(c = getscochar(csound, 1)) || c == LF)
      if (c == LF) {
        STA(lincnt)++;
        STA(linpos) = 0;
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
      if (isblank(c)) goto again;
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
        S_MACRO *mm = (S_MACRO*) csound->Malloc(csound, sizeof(S_MACRO));
        mm->margs = MARGS;
        if (UNLIKELY(!check_preproc_name(csound, "define"))) {
          csound->Message(csound, Str("Not #define"));
          csound->Free(csound, mm); free(mname);
          flushlin(csound);
          //          free(mname);
          goto srch;
        }
        while (isspace((c = getscochar(csound, 1))));
        while (isNameChar(c, i)) {
          char  *new;
          mname[i++] = c;
          if (i==mlen) {
            new = (char *)realloc(mname, mlen+=40);
            if (new==NULL) {
              fprintf(stderr, "Out of Memory\n");
              exit(7);
            }
            mname = new;
          }
          c = getscochar(csound, 1);
        }
        mname[i] = '\0';
        if (csound->oparms->msglevel & TIMEMSG)
          csound->Message(csound, Str("Macro definition for %s\n"), mname);
        mm->name = csound->Malloc(csound, i + 1);
        strcpy(mm->name, mname);
        if (c == '(') { /* arguments */
          do {
            while (isspace((c = getscochar(csound, 1))));
            i = 0;
            while (isNameChar(c, i)) {
              char *new;
              mname[i++] = c;
              if (i==mlen) {
                new = (char *)realloc(mname, mlen+=40);
                if (new==NULL) {
                  fprintf(stderr, "Out of Memory\n");
                  exit(7);
                }
                mname = new;
              }
              c = getscochar(csound, 1);
            }
            mname[i] = '\0';
            mm->arg[arg] = csound->Malloc(csound, i+1);
            strcpy(mm->arg[arg++], mname);
            if (arg>=mm->margs) {
              mm = (S_MACRO*)csound->ReAlloc(csound, mm,
                                    sizeof(S_MACRO)+mm->margs*sizeof(char*));
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
        while ((c = getscochar(csound, 1)) != '#') {   /* Skip to next # */
          if (UNLIKELY(c==EOF))
            scorerr(csound, Str("Syntax error in macro definition"));
        }
        mm->body = corfile_create_w();
#ifdef MACDEBUG
        csound->DebugMsg(csound,"%s(%d): macro %s %p\n",
                         __FILE__, __LINE__, mname, mm->body);
#endif
        while ((c = getscochar(csound, 0)) != '#') {  /* Do not expand here!! */
          if (UNLIKELY(c==EOF))
            scorerr(csound, Str("Syntax error in macro definition"));
          corfile_putc(c, mm->body);
          if (c=='\\') {
            corfile_putc(getscochar(csound, 0), mm->body);    /* Allow escaped # */
          }
          if (c=='\n') STA(lincnt)++;
        }
        corfile_rewind(mm->body);
        mm->next = STA(macros);
        STA(macros) = mm;
#ifdef MACDEBUG
        csound->Message(csound, Str("Macro %s with %d arguments defined\n"),
                                mm->name, mm->acnt);
        csound->Message(csound, "with body %s\n", corfile_body(mm->body));
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
          char *new;
          mname[i++] = c;
          if (i==mlen) {
            new = (char *)realloc(mname, mlen+=40);
            if (new==NULL) {
              fprintf(stderr, "Out of Memory\n");
              exit(7);
            }
            mname = new;
          }
        }
        mname[i]='\0';
        while ((c=getscochar(csound, 1))!='\n');
        STA(input_cnt)++;
        if (STA(input_cnt)>=STA(input_size)) {
          int old = STA(str)-STA(inputs);
          STA(input_size) += 20;
          STA(inputs) = csound->ReAlloc(csound, STA(inputs), STA(input_size)
                                                    * sizeof(IN_STACK));
          STA(str) = &STA(inputs)[old];     /* In case it moves */
        }
        STA(str)++;
        STA(str)->is_marked_repeat = 0;
#ifdef HAVE_CURL
        if (strstr(mname, "://"))
          STA(str)->cf = copy_url_corefile(csound, mname, 1);
        else
#endif
          STA(str)->cf = copy_to_corefile(csound, mname, "INCDIR", 1);
        if (STA(str)->cf == NULL) {
          STA(str)--;
          STA(str)->line--; /* include was one line earlier */
          STA(linepos) = 0;
          scorerr(csound, Str("Cannot open #include'd file %s"), mname);
        }
        else {
          STA(str)->line = 1;
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
          char *new;
          mname[i++] = c;
          if (i==mlen) {
            new = (char *)realloc(mname, mlen+=40);
            if (new==NULL) {
              fprintf(stderr, "Out of Memory\n");
              exit(7);
            }
            mname = new;
          }
          c = getscochar(csound, 1);
        }
        mname[i] = '\0';
        if (csound->oparms->msglevel & TIMEMSG)
          csound->Message(csound, Str("macro %s undefined\n"), mname);
        undefine_score_macro(csound, mname);
        while (c != '\n' && c != EOF)
          c = getscochar(csound, 1); /* ignore rest of line */
      }
#ifdef SCORE_PARSER
      else if (c=='e') {
        if (UNLIKELY(!check_preproc_name(csound, "exit"))) {
          csound->Message(csound, "Not #exit");
          flushlin(csound);
          free(mname);
          goto srch;
        }
        while (c != '\n' && c != EOF)
          c = getscochar(csound, 1); /* ignore rest of line */
      }
#endif
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
    case 'y':           /* Set random seed */
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
    STA(linpos)++;
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
      if (UNLIKELY(STA(linpos))) {
        sreaderr(csound, Str("unexpected char %c"), c);
        csound->Message(csound, Str("      remainder of line flushed\n"));
        flushlin(csound);
      }
      return(0);                              /*    so return            */
    }
    p = STA(sp) = STA(nxp);                     /* else start copying to text */
    *p++ = c;
    STA(linpos)++;
    if (c == '"') {                           /* if have quoted string,  */
      /* IV - Oct 31 2002: allow string instr name for i and q events */
      if (UNLIKELY(STA(bp)->pcnt < 3 &&
          !((STA(op) == 'i' || STA(op) == 'q') &&
            !STA(bp)->pcnt))) {
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
        if (p >= STA(memend))
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
        if (p >= STA(memend))
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
    STA(nxp) = p;                            /*  add blank      */
    return(1);                              /*  and report ok  */
}

MYFLT stof(CSOUND *csound, char s[])            /* convert string to MYFLT  */
                                    /* (assumes no white space at beginning */
{                                   /*      but a blank or nl at end)       */
    char    *p;
    MYFLT   x = (MYFLT) cs_strtod(s, &p);

    if (*p=='z') return FL(800000000000.0); /* 25367 years */
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
