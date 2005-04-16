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

#include "cs.h"                                     /*   SREAD.C     */
#include <math.h>      /* for fabs() */
#include <ctype.h>
#include "namedins.h"           /* IV - Oct 31 2002 */

static SRTBLK  *bp, *prvibp;    /* current srtblk,  prev w/same int(p1) */
static char    *sp, *nxp;       /* string pntrs into srtblk text        */
static int     op;              /* opcode of current event              */
static int     warpin;          /* input format sensor                  */
static int     linpos;          /* line position sensor                 */
static int     lincnt;          /* count of lines/section in scorefile  */
static MYFLT   prvp2 = -FL(1.0);        /* Last event time */
static MYFLT   clock_base = FL(0.0);
static MYFLT   warp_factor = FL(1.0);

static void    copylin(void), copypflds(void), ifa(void), setprv(void);
static void    carryerror(void), pcopy(int, int, SRTBLK*), salcinit(void);
static void    salcblk(void), flushlin(void);
static int     getop(void), getpfld(void);
       MYFLT   stof(char *);
extern FILE    *fopen_path(ENVIRON *, char *, char *, char *);

#define MEMSIZ  16384           /* size of memory requests from system  */
#define MARGIN  4096            /* minimum remaining before new request */

static char   *curmem = NULL;
static char   *memend = NULL;          /* end of cur memblk     */

#define MARGS   (3)
typedef struct MACRO {          /* To store active macros */
  char          *name;          /* Use is by name */
  int           acnt;           /* Count of arguments */
  char          *body;          /* The text of the macro */
  struct MACRO  *next;          /* Chain of active macros */
  int           margs;          /* ammount of space for args */
  char          *arg[MARGS];    /* With these arguments */
} MACRO;

static  MACRO   *macros = NULL;
struct in_stack {               /* Stack of active inputs */
  short string;                 /* Flag to say if string or file */
  short args;                   /* Argument count for macro */
  char  *body;                  /* String */
  FILE  *file;                  /* File case only */
  MACRO *mac;
  short line;
};

struct marked_sections {
  char  *name;
  long  posit;
  char  *file;
} names[30], *current_name = NULL;

static int next_name = -1;

static struct in_stack *inputs = NULL, *str; /* Currently allow 20 maximum */
static int input_size = 0, input_cnt = 0;
static int pop = 0;                          /* Number of macros to pop */
static int ingappop = 1;                     /* Are we in a popable gap? */
static int linepos = -1;

static intptr_t expand_nxp(ENVIRON *csound)
{
    char      *oldp;
    SRTBLK    *p;
    intptr_t  offs;
    size_t    nbytes;

    if (nxp >= (memend + MARGIN)) {
      csound->Die(csound, Str("sread:  text space overrun, increase MARGIN"));
      return 0;     /* not reached */
    }
    /* calculate the number of bytes to allocate */
    nbytes = (size_t) (memend - curmem);
    nbytes = nbytes + (nbytes >> 3) + (size_t) (MEMSIZ - 1);
    nbytes &= ~((size_t) (MEMSIZ - 1));
    /* extend allocated memory */
    oldp = curmem;
    curmem = (char*) csound->ReAlloc(csound, curmem, nbytes + (size_t) MARGIN);
    memend = (char*) curmem + (long) nbytes;
    /* did the pointer change ? */
    if (curmem == oldp)
      return (intptr_t) 0;      /* no, nothing to do */
    /* correct all pointers for the change */
    offs = (intptr_t) ((uintptr_t) curmem - (uintptr_t) oldp);
    if (bp != NULL)
      bp = (SRTBLK*) ((uintptr_t) bp + (intptr_t) offs);
    if (prvibp != NULL)
      prvibp = (SRTBLK*) ((uintptr_t) prvibp + (intptr_t) offs);
    if (sp != NULL)
      sp = (char*) ((uintptr_t) sp + (intptr_t) offs);
    if (nxp != NULL)
      nxp = (char*) ((uintptr_t) nxp + (intptr_t) offs);
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

typedef struct scotables {
        long len;
        MYFLT *table;
} SCOTABLE;
SCOTABLE sco_table[100]; /* gab A9 */

static void scorerr(char *s)
{
    ENVIRON *csound = &cenviron;
    struct in_stack *curr = str;

    csound->Message(csound,Str("score error:  %s on line %d position %d"),
           s, str->line, linepos);

    while (curr!=inputs) {
      if (curr->string) {
        MACRO *mm = NULL;
        while (mm != curr->mac) mm = mm->next;
        csound->Message(csound,Str("called from line %d of macro %s\n"),
               curr->line, mm->name);
      }
      else {
        csound->Message(csound,Str("in line %f of file input %s\n"),
               curr->line, curr->body);
      }
      curr--;
    }
    longjmp(csound->exitjmp, 1);
}


MYFLT operate(MYFLT a, MYFLT b, char c)
{
    ENVIRON *csound = &cenviron;
    MYFLT ans;
    extern MYFLT MOD(MYFLT,MYFLT);

    switch (c) {
    case '+': ans = a+b; break;
    case '-': ans = a-b; break;
    case '*': ans = a*b; break;
    case '/': ans = a/b; break;
    case '%': ans = MOD(a,b); break;
    case '^': ans = (MYFLT)pow((double)a, (double)b); break;
    case '&': ans = (MYFLT)(((long)a)&((long)b)); break;
    case '|': ans = (MYFLT)(((long)a)|((long)b)); break;
    case '#': ans = (MYFLT)(((long)a)^((long)b)); break;
    default:
      csoundDie(csound, Str("Internal error op=%c"), c);
      ans = FL(0.0);    /* compiler only */
    }
    return ans;
}

#define ungetscochar(c) if (str->string) str->body--; else ungetc(c, str->file)

static int getscochar(int expand) /* Read a score character, expanding macros */
{                                 /* if flag set */
    ENVIRON *csound = &cenviron;
    int     c;
top:
    if (str->string) {
      c= *str->body++;
      if (c == '\0') {
        pop += str->args;
        str--; input_cnt--;
        goto top;
      }
    }
    else {
      c = getc(str->file);
      if (c == EOF) {
        if (str == &inputs[0]) return EOF;
        fclose(str->file);
        mfree(csound, str->body);
        str--; input_cnt--; goto top;
      }
    }
    if (c =='\r') {
      if ((c = getc(str->file)) != '\n')
        ungetc(c, str->file);   /* For macintosh */
      c = '\n';
    }
    if (c == '\n') {
      str->line++; linepos = -1;
    }
    else linepos++;
    if (ingappop && pop)
      do {
        MACRO *nn = macros->next;
        int i;
#ifdef MACDEBUG
        csound->Message(csound,"popping %s\n", macros->name);
#endif
        mfree(csound, macros->name); mfree(csound, macros->body);
        for (i=0; i<macros->acnt; i++) {
          mfree(csound, macros->arg[i]);
        }
        mfree(csound, macros);
        macros = nn;
        pop--;
      } while (pop);
    if (c == '$' && expand) {
      char name[100];
      unsigned int i=0;
      int j;
      MACRO *mm, *mm_save = NULL;
      ingappop = 0;
      while (isalpha(c=getscochar(1)) || (i!=0 && (isdigit(c)||c=='_'))) {
        name[i++] = c; name[i] = '\0';
        mm = macros;
        while (mm != NULL) {  /* Find the definition */
          if (!(strcmp (name, mm->name))) break;
          mm = mm->next;
        }
        if (mm != NULL) mm_save = mm;   /* found a name */
      }
      mm = mm_save;
      if (mm == NULL) {
        csoundDie(csound, Str("Macro expansion symbol ($) without macro name"));
      }
      if (strlen (mm->name) != i) {
/*      csound->Warning(csound, "$%s matches macro name $%s", name, mm->name); */
        do {
          ungetscochar (c);
          c = name[--i];
        } while (i >= strlen (mm->name));
        c = getscochar (1); i++;
      }
      if (c!='.') { ungetscochar(c); }
#ifdef MACDEBUG
      csound->Message(csound,"Macro %s found\n", name);
#endif
      if (mm==NULL) scorerr(Str("Undefined macro"));
#ifdef MACDEBUG
      csound->Message(csound, "Found macro %s required %d arguments\n",
                              mm->name, mm->acnt);
#endif
                                /* Should bind arguments here */
                                /* How do I recognise entities?? */
      if (mm->acnt) {
        if ((c=getscochar(1))!='(') scorerr(Str("Syntax error in macro call"));
        for (j=0; j<mm->acnt; j++) {
          char term = (j==mm->acnt-1 ? ')' : '\'');
          char trm1 = (j==mm->acnt-1 ? ')' : '#');
          MACRO* nn = (MACRO*) mmalloc(csound, sizeof(MACRO));
          unsigned int size = 100;
          nn->name = mmalloc(csound, strlen(mm->arg[j])+1);
          strcpy(nn->name, mm->arg[j]);
#ifdef MACDEBUG
          csound->Message(csound,"defining argument %s ", nn->name);
#endif
          i = 0;
          nn->body = (char*)mmalloc(csound, 100);
          while ((c = getscochar(1))!= term && c != trm1) {
            nn->body[i++] = c;
            if (i>= size) nn->body = mrealloc(csound, nn->body, size += 100);
          }
          nn->body[i]='\0';
#ifdef MACDEBUG
          csound->Message(csound,"as...#%s#\n", nn->body);
#endif
          nn->acnt = 0; /* No arguments for arguments */
          nn->next = macros;
          macros = nn;
        }
      }
      input_cnt++;
      if (input_cnt>=input_size) {
        int old = str-inputs;
        input_size += 20;
/*      csound->Message(csound,"Expanding includes to %d\n", input_size); */
        inputs = mrealloc(csound, inputs, input_size*sizeof(struct in_stack));
        if (inputs == NULL) {
          csoundDie(csound, Str("No space for include files"));
        }
        str = &inputs[old];     /* In case it moves */
      }
      str++;
      str->string = 1; str->body = mm->body; str->args = mm->acnt;
      str->mac = mm; str->line = 1;
#ifdef MACDEBUG
      csound->Message(csound,
                      "Macro %s definded as >>%s<<\n", mm->name, mm->body);
#endif
      ingappop = 1;
      goto top;
    }
/* End of macro expander */
    if (expand && c == '[') {           /* Evaluable section */
      char stack[30];
      MYFLT vv[30];
      char *op = stack-1;
      MYFLT *pv = vv-1;
      char buffer[100];
      int i;
      int type=0;
      *++op = '[';
      c = getscochar(1);
      do {
        switch (c) {
        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
        case '.':
          if (type==1) {
            csoundDie(csound, Str("Number not allowed in context []"));
          }
          i = 0;
          while (isdigit(c) || c=='.' || c=='e' || c=='E') {
            buffer[i++]=c;
            c = getscochar(1);
          }
          buffer[i] = '\0';
          *++pv = (MYFLT)atof(buffer);
          type = 1;
          break;
        case '~':
          if (type==1) {
            csoundDie(csound, Str("Random not in context []"));
          }
          *++pv = (MYFLT) rand()/(MYFLT)RAND_MAX;
          type = 1;
          c = getscochar(1);
          break;
        case '@':
          if (type==1) {
            csoundDie(csound, Str("Upper not in context []"));
          }
          {
            int n = 0;
            int k = 0;          /* 0 or 1 depending on guard bit */
            c = getscochar(1);
            if (c=='@') { k = 1; c = getscochar(1);}
            while (isdigit(c)) {
              n = 10*n + c - '0';
              c = getscochar(1);
            }
            i = 1;
            while (i<=n-k && i< 0x4000000) i <<= 1;
            *++pv = (MYFLT)(i+k);
            type = 1;
          }
          break;
        case '+': case '-':
          if (type==0) {
            csoundDie(csound,
                      Str("Operator %c not allowed in context []"), c);
          }
          if (*op != '[' && *op != '(') {
            MYFLT v = operate(*(pv-1), *pv, *op);
            op--; pv--;
            *pv = v;
          }
          type = 0;
          *++op = c; c = getscochar(1); break;
        case '*':
        case '/':
        case '%':
          if (type==0) {
            csoundDie(csound,
                      Str("Operator %c not allowed in context []"), c);
          }
          if (*op == '*' || *op == '/' || *op == '%') {
            MYFLT v = operate(*(pv-1), *pv, *op);
            op--; pv--;
            *pv = v;
          }
          type = 0;
          *++op = c; c = getscochar(1); break;
        case '&':
        case '|':
        case '#':
          if (type==0) {
            csoundDie(csound,
                      Str("Operator %c not allowed in context []"), c);
          }
          if (*op == '|' || *op == '&' || *op == '#') {
            MYFLT v = operate(*(pv-1), *pv, *op);
            op--; pv--;
            *pv = v;
          }
          type = 0;
          *++op = c; c = getscochar(1); break;
        case '(':
          if (type==1) {
            csoundDie(csound,
                      Str("Open bracket not allowed in context []"));
          }
          type = 0;
          *++op = c; c = getscochar(1); break;
        case ')':
          if (type==0) {
            csoundDie(csound,
                      Str("Closing bracket not allowed in context []"));
          }
          while (*op != '(') {
            MYFLT v = operate(*(pv-1), *pv, *op);
            op--; pv--;
            *pv = v;
          }
          type = 1;
          op--; c = getscochar(1); break;
        case '^':
          type = 0;
          *++op = c; c = getscochar(1); break;
        case ']':
          if (type==0) {
            csoundDie(csound,
                      Str("Closing bracket not allowed in context []"));
          }
          while (*op != '[') {
            MYFLT v = operate(*(pv-1), *pv, *op);
            op--; pv--;
            *pv = v;
          }
          c = '$';
          break;
        case '$':
          break;
        case ' ':               /* Ignore spaces */
          c = getscochar(1);
          continue;
        default:
          csound->Message(csound,"read %c(%.2x)\n", c, c);
          csoundDie(csound, Str("Incorrect evaluation"));
        }
      } while (c!='$');
      /* Make string macro or value */
      sprintf(buffer, "%f", *pv);
/*    csound->Message(csound, "Buffer:>>>%s<<<\n", buffer); */
      {
        MACRO* nn = (MACRO*) mmalloc(csound, sizeof(MACRO));
        nn->name = mmalloc(csound, 2);
        strcpy(nn->name, "[");
        nn->body = (char*)mmalloc(csound, strlen(buffer)+1);
        strcpy(nn->body, buffer);
        nn->acnt = 0;   /* No arguments for arguments */
        nn->next = macros;
        macros = nn;
        input_cnt++;
        if (input_cnt>=input_size) {
          int old = str-inputs;
/*        csound->Message(csound,"Expanding includes to %d\n",input_size+20); */
          input_size += 20;
          inputs = mrealloc(csound, inputs, input_size*sizeof(struct in_stack));
          if (inputs == NULL) {
            csoundDie(csound, Str("No space for include files"));
          }
          str = &inputs[old];     /* In case it moves */
        }
        str++;
        str->string = 1; str->body = nn->body; str->args = 0;
        str->mac = NULL; str->line = 1;
#ifdef MACDEBUG
        csound->Message(csound,"[] defined as >>%s<<\n", nn->body);
#endif
        ingappop = 1;
        goto top;
      }
    }
    return c;
}

static char repeat_name_n[40][40];
static int repeat_cnt_n[40] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                               0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
static long repeat_point_n[40];
static int repeat_inc_n = 1;
static MACRO *repeat_mm_n[40];
static int repeat_index=0;

static int nested_repeat(void)  /* gab A9*/
{
    ENVIRON *csound = &cenviron;
    repeat_cnt_n[repeat_index]--;
    if (repeat_cnt_n[repeat_index]==0) { /* Expired */
      if (repeat_index > 1) {
        char c[41];
        int j;
        for (j = 0; j<repeat_index; j++) {
          c[j]=' ';
          c[j+1]='\0';
        }
        if (csound->oparms->msglevel)
          csound->Message(csound,Str("%s Nested LOOP terminated, level:%d\n"),
                          c,repeat_index);

      }
      else {
        if (csound->oparms->msglevel)
          csound->Message(csound,Str("External LOOP terminated, level:%d\n"),
                          repeat_index);
      }
      if (strcmp(repeat_name_n[repeat_index], macros->name)==0) {
        MACRO *mm=macros->next;
        mfree(csound, macros->name); mfree(csound, macros->body);
        mfree(csound, macros); macros = mm;
      }
      else {
        MACRO *mm = macros;
        MACRO *nn = mm->next;
        while (strcmp(repeat_name_n[repeat_index], nn->name)!=0) {
          mm = nn; nn = nn->next;
          if (nn==NULL)
            scorerr(Str("Undefining undefined macro"));
        }
        mfree(csound, nn->name); mfree(csound, nn->body);
        mm->next = nn->next; mfree(csound, nn);
      }
      repeat_index--;
    }
    else {
      int i;
      fseek(str->file, repeat_point_n[repeat_index], SEEK_SET);
      sscanf(repeat_mm_n[repeat_index]->body, "%d", &i);
      i = i + repeat_inc_n;
      sprintf(repeat_mm_n[repeat_index]->body, "%d", i);
      if (repeat_index > 1) {
        char c[41];
        int j;
        for (j = 0; j<repeat_index; j++) {
          c[j]=' ';
          c[j+1]='\0';
        }
        if (csound->oparms->msglevel)
          csound->Message(csound,Str("%s  Nested LOOP section (%d) Level:%d\n"),
                          c, i, repeat_index);
      }
      else {
        if (csound->oparms->msglevel)
          csound->Message(csound,Str(" External LOOP section (%d) Level:%d\n"),
                          i, repeat_index);
      }
      *(nxp-2) = 's'; *nxp++ =  LF;
      return 1;
    }
    return 0;
}

/* Variable for repeat sections */
static char repeat_name[40];
static int repeat_cnt = 0;
static long repeat_point;
static int repeat_inc = 1;
static MACRO *repeat_mm;

static int do_repeat(void)      /* At end of section repeat if necessary */
{
    ENVIRON *csound = &cenviron;
    repeat_cnt--;
    if (repeat_cnt==0) { /* Expired */
      /* Delete macro */
      if (csound->oparms->msglevel)
        csound->Message(csound,Str("Loop terminated\n"));
      if (strcmp(repeat_name, macros->name)==0) {
        MACRO *mm=macros->next;
        mfree(csound, macros->name); mfree(csound, macros->body);
        mfree(csound, macros); macros = mm;
      }
      else {
        MACRO *mm = macros;
        MACRO *nn = mm->next;
        while (strcmp(repeat_name, nn->name)!=0) {
          mm = nn; nn = nn->next;
          if (nn==NULL)
            scorerr(Str("Undefining undefined macro"));
        }
        mfree(csound, nn->name); mfree(csound, nn->body);
        mm->next = nn->next; mfree(csound, nn);
      }
    }
    else {
      int i;
      fseek(str->file, repeat_point, SEEK_SET);
      sscanf(repeat_mm->body, "%d", &i);
      i = i + repeat_inc;
      sprintf(repeat_mm->body, "%d", i);
      if (csound->oparms->msglevel)
        csound->Message(csound,Str("Repeat section (%d)\n"), i);
      *(nxp-2) = 's'; *nxp++ = LF;
      if (nxp >= memend)                /* if this memblk exhausted */
        expand_nxp(csound);
      return 1;
    }
    return 0;
}

void sread_init(void)
{
    ENVIRON *csound = &cenviron;
    inputs = (struct in_stack*)mmalloc(csound, 20*sizeof(struct in_stack));
    input_size = 20;
    input_cnt = 0;
    str = inputs;
    str->file = SCOREIN;
    str->string = 0; str->body = scorename;
    str->line = 1; str->mac = NULL;
}


int sread(void)                 /*  called from main,  reads from SCOREIN   */
{                               /*  each score statement gets a sortblock   */
    ENVIRON *csound = &cenviron;
    int  rtncod;                /* return code to calling program:      */
                                /*   2 = section read, more remaining   */
                                /*   1 = last section,   0 = null file  */
    bp = prvibp = csound->frstbp = NULL;
    nxp = NULL;
    warpin = 0;
    lincnt = 1;
    csound->sectcnt++;
    rtncod = 1;
    salcinit();              /* init the mem space for this section */

    while ((op = getop()) != EOF) {  /* read next op from scorefile */
      rtncod = 2;
      salcblk();             /* build a line structure; init bp,nxp */
    again:
      switch (op) {                 /*  and dispatch on opcodes     */
      case 'i':
      case 'f':
      case 'a':
      case 'q':
        ifa();
        break;
      case 'w':
        warpin++;
        copypflds();
        break;
      case 't':
        copypflds();
        break;
      case 'b': /* Set a clock base */
        {
          char *old_nxp = nxp-2;
          getpfld();
          clock_base = stof(sp);
          if (csound->oparms->msglevel)
            csound->Message(csound,Str("Clockbase = %f\n"), clock_base);
          flushlin();
          op = getop();
          nxp = old_nxp;
          *nxp++ = op; /* Undo this line */
          nxp++;
          goto again;
        }
      case 's':
        if (repeat_cnt!=0) {
          if (do_repeat()) return (rtncod);
        }
/*         if (current_name) { */
/*           fclose(str->file); */
/*           mfree(csound, str->body); */
/*           str--; input_cnt--; */
/*           current_name = NULL; */
/*         } */
        copylin();
        clock_base = FL(0.0);
        warp_factor = FL(1.0);
        prvp2 = -FL(1.0);
        return(rtncod);
      case '}':
        {
          int temp;
          char *old_nxp = nxp-2;
          if ((temp=repeat_cnt_n[repeat_index])!=0)
            nested_repeat();
          op = getop();
          nxp = old_nxp;
          *nxp++ = op;
          nxp++;
          goto again;
        }
      case '{':
        {
          char *old_nxp = nxp-2;
          repeat_index++;
/*           if (current_name) { */
/*             fclose(str->file); */
/*             mfree(csound, str->body); */
/*             str--; input_cnt--; */
/*             current_name = NULL; */
/*           } */
          if (str->string) {
            int c;
            csound->Message(csound,Str("LOOP not at top level; ignored\n"));
            do {    /* Ignore rest of line */
              c = getscochar(1);
            } while (c != LF && c != EOF);
          }
          else {
            char *nn = repeat_name_n[repeat_index];
            int c;
            repeat_mm_n[repeat_index] =
              (MACRO*)mmalloc(csound, sizeof(MACRO));
            repeat_cnt_n[repeat_index] = 0;
            do {
              c = getscochar(1);
            } while (c==' '||c=='\t');
            do {
              repeat_cnt_n[repeat_index] =
                10 * repeat_cnt_n[repeat_index] + c - '0';
              c = getscochar(1);
            } while (isdigit(c));
            if (repeat_index > 1) {
              char st[41];
              int j;
              for (j = 0; j<repeat_index; j++) {
                st[j]=' ';
                st[j+1]='\0';
              }
              if (csound->oparms->msglevel)
                csound->Message(csound,Str("%s Nested LOOP=%d Level:%d\n"), st,
                                repeat_cnt_n[repeat_index], repeat_index);
            }
            else {
              if (csound->oparms->msglevel)
                csound->Message(csound,Str("External LOOP=%d Level:%d\n"),
                                repeat_cnt_n[repeat_index], repeat_index);
            }
            do {
              c = getscochar(1);
            } while (c==' '||c=='\t');
            do {
              *nn++ = c;
            } while (isalpha(c=getscochar(1)) ||
                     (nn!=repeat_name_n[repeat_index] &&
                      (isdigit(c)||c=='_')));
            *nn = '\0';
            /* Define macro for counter */
            /* csound->Message(csound,"Found macro definition for %s\n", */
            /*                           repeat_name); */
            repeat_mm_n[repeat_index]->name =
              mmalloc(csound, strlen(repeat_name_n[repeat_index])+1);
            strcpy(repeat_mm_n[repeat_index]->name,
                   repeat_name_n[repeat_index]);
            repeat_mm_n[repeat_index]->acnt = 0;
            repeat_mm_n[repeat_index]->body = (char*)mmalloc(csound, 8);
            sprintf(repeat_mm_n[repeat_index]->body, "%d", 0);
            repeat_mm_n[repeat_index]->next = macros;
            macros = repeat_mm_n[repeat_index];
            while (c != LF && c != EOF) {       /* Ignore rest of line */
              c = getscochar(1);
            }
            repeat_point_n[repeat_index] = ftell(str->file);
          }
          clock_base = FL(0.0);
          warp_factor = FL(1.0);
          prvp2 = -FL(1.0);
          op = getop();
          nxp = old_nxp;
          *nxp++ = op;
          nxp++;
          goto again;
        }
      case 'r': /* For now treat as s */
                                /* First deal with previous section */
        if (repeat_cnt!=0) {
          if (do_repeat()) return (rtncod);
        }
/*         if (current_name) { */
/*           fclose(str->file); */
/*           mfree(csound, str->body); */
/*           str--; */
/*           current_name = NULL; */
/*         } */
        /* Then remember this state */
        *(nxp-2) = 's'; *nxp++ = LF;
        if (nxp >= memend)              /* if this memblk exhausted */
          expand_nxp(csound);
        if (str->string) {
          int c;
          csound->Message(csound,Str("Repeat not at top level; ignored\n"));
          do {    /* Ignore rest of line */
            c = getscochar(1);
          } while (c != LF && c != EOF);
        }
        else {
          char *nn = repeat_name;
          int c;
          repeat_mm = (MACRO*)mmalloc(csound, sizeof(MACRO));
          repeat_cnt = 0;
          do {
            c = getscochar(1);
          } while (c==' '||c=='\t');
          do {
            repeat_cnt = 10 * repeat_cnt + c - '0';
            c = getscochar(1);
          } while (isdigit(c));
          if (csound->oparms->msglevel)
            csound->Message(csound,Str("Repeats=%d\n"), repeat_cnt);
          do {
            c = getscochar(1);
          } while (c==' '||c=='\t');
          do {
            *nn++ = c;
          } while (isalpha(c=getscochar(1)) ||
                   (nn!=repeat_name && (isdigit(c)||c=='_')));
          *nn = '\0';
          /* Define macro for counter */
          /*             printf("Found macro definition for %s\n", */
          /*                     repeat_name); */
          repeat_mm->name = mmalloc(csound, strlen(repeat_name)+1);
          strcpy(repeat_mm->name, repeat_name);
          repeat_mm->acnt = 0;
          repeat_mm->body = (char*)mmalloc(csound, 8);
          sprintf(repeat_mm->body, "%d", 1); /* Set value */
          repeat_mm->next = macros;
          macros = repeat_mm;
          while (c != LF && c != EOF) {  /* Ignore rest of line */
            c = getscochar(1);
          };
          repeat_point = ftell(str->file);
        }
        clock_base = FL(0.0);
        warp_factor = FL(1.0);
        prvp2 = -FL(1.0);
        return (rtncod);
      case 'e':
        if (repeat_cnt!=0) {
          if (do_repeat()) return (rtncod);
        }
/*         if (current_name) { */
/*           fclose(str->file); */
/*           mfree(csound, str->body); */
/*           str--; input_cnt--; */
/*           current_name = NULL; */
/*         } */
        copylin();
        return(--rtncod);
      case 'm': /* Remember this place */
        {
          char *old_nxp = nxp-2;
          char buff[200];
          int c;
          int i = 0;
          while ((c=getscochar(1))==' ' || c=='\t');
          do {
            buff[i++]=c;
          } while (isalpha(c=getscochar(1))|| (i!=0 && (isdigit(c)||c=='_')));
          buff[i]=0;
          if (csound->oparms->msglevel)
            csound->Message(csound,Str("Named section >>>%s<<<\n"), buff);
          for (i=0; i<=next_name; i++)
            if (strcmp(buff, names[i].name)==0) break;
          if (i>next_name) {
            i = ++next_name;
            names[i].name = (char*)mmalloc(csound, i+1);
            strcpy(names[i].name, buff);
          }
          else mfree(csound, names[i].file);
          flushlin();
          if (!str->string) {
            names[next_name].posit = ftell(str->file);
            names[next_name].file = mmalloc(csound, strlen(str->body)+1);
            strcpy(names[next_name].file, str->body);
            if (csound->oparms->msglevel)
              csound->Message(csound,Str("%d: File %s position %ld\n"),
                              next_name, names[next_name].file,
                              names[next_name].posit);
          }
          else {
            csound->Message(csound,
                             Str("Ignoring name %s not in file\n"), buff);
            names[i].name[0] = '\0'; /* Destroy name */
          }
          op = getop();
          nxp = old_nxp;
          *nxp++ = op; /* Undo this line */
          nxp++;
          goto again;           /* suggested this loses a line?? */
        }
      case 'n':
        {
          char *old_nxp = nxp-2;
          char buff[200];
          int c;
          int i = 0;
          while ((c=getscochar(1))==' ' || c=='\t');
          do {
            buff[i++]=c;
          } while (isalpha(c=getscochar(1))|| (i!=0 && (isdigit(c)||c=='_')));
          buff[i]='\0';
          flushlin();
          for (i = 0; i<=next_name; i++)
            if (strcmp(buff, names[i].name)==0) break;
          if (i>next_name) csound->Message(csound,Str("Name not found"), buff);
          else {
            csound->Message(csound,Str("Duplicate %d: %s (%s,%ld)\n"),
                   i, buff, names[i].file, names[i].posit);
            input_cnt++;
            if (input_cnt>=input_size) {
              int old = str-inputs;
              input_size += 20;
/*               csound->Message(csound,
                                  "Expanding includes to %d\n", input_size); */
              inputs = mrealloc(csound, inputs,
                                input_size*sizeof(struct in_stack));
              if (inputs == NULL) {
                csoundDie(csound, Str("No space for include files"));
              }
              str = &inputs[old];     /* In case it moves */
            }
            str++;
            str->string = 0;
            str->file = fopen(names[i].file, "r");
            /*RWD 3:2000*/
            if (str->file==NULL) {
              csoundDie(csound, Str("cannot open input file %s"),
                                   names[i].file);
            }
            str->body = mmalloc(csound, strlen(names[i].file)+1);
            fseek(str->file, names[i].posit, SEEK_SET);
            strcpy(str->body, names[i].file);
          }
          op = getop();
          nxp = old_nxp;
          *nxp++ = op; /* Undo this line */
          nxp++;
          goto again;
        }
      case 'v': /* Suggestion of Bryan Bales */
        {       /* Set local variability of time */
          char *old_nxp = nxp-2;
          getpfld();
          warp_factor = stof(sp);
          if (csound->oparms->msglevel)
            csound->Message(csound,Str("Warp_factor = %f\n"), warp_factor);
          flushlin();
          op = getop();
          nxp = old_nxp;
          *nxp++ = op; /* Undo this line */
          nxp++;
          goto again;
        }
      case 'x':                 /* Skip section */
        while (1) {
          switch(op = getop()) {
          case 's':
          case 'r':
          case 'm':
          case 'e':
            *(nxp-2) = op;                    /* place op, blank into text    */
            goto again;
          case EOF:
            *(nxp-2) = 'e';
            goto ending;
          default:
            flushlin();
/*             csound->Message(csound,"Ignoring %c\n", op); */
          }
        };
        break;
      default:
        csound->Message(csound,Str("sread is confused on legal opcodes\n"));
        break;
      }
    }
 ending:
    if (rtncod<0)               /* Ending so clear macros */
      while (macros!=NULL) {
        int i;
        mfree(csound, macros->body);
        mfree(csound, macros->name);
        for (i=0; i<macros->acnt; i++) mfree(csound, macros->arg[i]);
        macros = macros->next;
      }
    return(--rtncod);
}

static void copylin(void)       /* copy source line to srtblk   */
{
    ENVIRON *csound = &cenviron;
    int c;
    nxp--;
    if (nxp >= memend)          /* if this memblk exhausted */
      expand_nxp(csound);
    do {
      c = getscochar(1);
      *nxp++ = c;
    } while (c != LF && c != EOF);
    if (c == EOF) *(nxp-1) = '\n'; /* Avoid EOF characters */
    lincnt++;
    linpos = 0;
}

static void copypflds(void)
{
    bp->pcnt = 0;
    while (getpfld())       /* copy each pfield,    */
      bp->pcnt++;           /* count them,          */
    *(nxp-1) = LF;          /* terminate with newline */
}

static void ifa(void)
{
    ENVIRON *csound = &cenviron;
    SRTBLK *prvbp;
    int n;

    bp->pcnt = 0;
    while (getpfld()) {             /* while there's another pfield,  */
      if (++bp->pcnt == PMAX) {
        csound->Message(csound,Str("sread: instr pcount exceeds PMAX\n"));
        csound->Message(csound,Str("\t sect %d line %d\n"),
                         csound->sectcnt, lincnt);
        csound->Message(csound,Str("      remainder of line flushed\n"));
        flushlin();
        continue;
      }
      if (*sp == '^' && op == 'i' && bp->pcnt == 2) {
        int foundplus = 0;
        if (*(sp+1)=='+') { sp++; foundplus = 1; }
        if (prvp2<0) {
          csound->Message(csound,Str("No previous event in ^\n"));
          prvp2 = bp->p2val = warp_factor*stof(sp+1);
        }
        else if (isspace(*(sp+1))) {
          /* stof() assumes no leading whitespace -- 070204, akozar */
          csound->Message(csound, Str("sread: illegal space following %s, "
                                      "sect %d line %d:  "),
                                  (foundplus?"^+":"^"),csound->sectcnt,lincnt);
          csound->Message(csound, Str("   zero substituted.\n"));
          prvp2 = bp->p2val = prvp2;
        }
        else prvp2 = bp->p2val = prvp2 + warp_factor*stof(sp+1);
      }
      else if (nxp-sp == 2 && (*sp == '.' || *sp == '+')) {
        if (op == 'i'
            && (*sp == '.' || bp->pcnt == 2)
            && ((bp->pcnt >= 2 && (prvbp = prvibp) != NULL
                 && bp->pcnt <= prvbp->pcnt)
                || (bp->pcnt == 1 && (prvbp = bp->prvblk) != NULL
                    && prvbp->text[0] == 'i'))) {
          if (*sp == '.') {
            nxp = sp;
            pcopy((int)bp->pcnt, 1, prvbp);
            if (bp->pcnt >= 2) prvp2 = bp->p2val;
          }
          else /* need the fabs() in case of neg p3 */
            prvp2 = bp->p2val = prvbp->p2val + (MYFLT)fabs(prvbp->p3val);
        }
        else carryerror();
      }
      else switch(bp->pcnt) {         /*  watch for p1,p2,p3, */
      case 1:                         /*   & MYFLT, setinsno..*/
        if ((op == 'i' || op == 'q') && *sp == '"')     /* IV - Oct 31 2002: */
          bp->p1val = SSTRCOD;                          /* allow string name */
        else
          bp->p1val = stof(sp);
        if (op == 'i')
          setprv();
        else prvibp = NULL;
        break;
      case 2: prvp2 = bp->p2val = warp_factor*stof(sp)+clock_base;
        break;
      case 3: if (op == 'i') bp->p3val = warp_factor*stof(sp);
      else bp->p3val = stof(sp);
      break;
      default:break;
      }
      switch (bp->pcnt) {             /* newp2, newp3:        */
      case 2: if (warpin) {                   /* for warpin,  */
        getpfld();                         /*   newp2 follows */
        bp->newp2 = warp_factor*stof(sp)+clock_base;
        nxp = sp;                          /*    (skip text)  */
      }
      else bp->newp2 = bp->p2val;          /* else use p2val  */
      break;
      case 3: if (warpin && (op == 'i' || op == 'f')) {
        getpfld();
        bp->newp3 = warp_factor*stof(sp);  /* same for newp3  */
        nxp = sp;
      }
      else bp->newp3 = bp->p3val;
      break;
      }
    }
    if (op == 'i' &&                /* then carry any rem pflds */
        ((prvbp = prvibp) != NULL ||
         (!bp->pcnt && (prvbp = bp->prvblk) != NULL &&
          prvbp->text[0] == 'i')) &&
        (n = prvbp->pcnt - bp->pcnt) > 0) {
      pcopy((int)bp->pcnt + 1, n, prvbp);
      bp->pcnt += n;
    }
    *(nxp-1) = LF;                  /* terminate this stmnt with newline */
}

static void setprv(void)        /*  set insno = (int) p1val     */
{                               /*  prvibp = prv note, same insno */
    ENVIRON *csound = &cenviron;
    SRTBLK *p = bp;
    short n;

    if (bp->p1val == SSTRCOD && *sp == '"') {   /* IV - Oct 31 2002 */
      char name[MAXNAME], *c, *s = sp;
      /* unquote instrument name */
      c = name; while (*++s != '"') *c++ = *s; *c = '\0';
      /* find corresponding insno */
      if (!(n = (short) named_instr_find(csound, name))) {
        csound->Message(csound, Str("WARNING: instr %s not found, "
                                    "assuming insno = -1\n"), name);
        n = -1;
      }
    }
    else n = (short) bp->p1val;         /* set current insno */
    bp->insno = n;

    while ((p = p->prvblk) != NULL)
      if (p->insno == n) {
        prvibp = p;                             /* find prev same */
        return;
      }
    prvibp = NULL;                              /*  if there is one */
}

static void carryerror(void)    /* print offending text line */
{                               /*      (partial)            */
    ENVIRON *csound = &cenviron;
    char *p;

    csound->Message(csound,
       Str("sread: illegal use of carry, sect %d line %d,   0 substituted\n"),
       csound->sectcnt,lincnt);
    *(nxp-3) = SP;
    p = bp->text;
    while (p <= nxp-2)
      csound->Message(csound,"%c",*p++);
    csound->Message(csound,"<=\n");
    *(nxp-2) = '0';
}

static void pcopy(int pfno, int ncopy, SRTBLK *prvbp)
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
    p = nxp;
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
      case 1: bp->p1val = prvbp->p1val;       /*  with p1-p3 vals */
        setprv();
        break;
      case 2: if (*(p-2) == '+')              /* (interpr . of +) */
        prvp2 = bp->p2val = prvbp->p2val + (MYFLT)fabs(prvbp->p3val);
      else prvp2 = bp->p2val = prvbp->p2val;
      bp->newp2 = bp->p2val;
      break;
      case 3: bp->newp3 = bp->p3val = prvbp->p3val;
        break;
      default:break;
      }
      pfno++;
    }
    nxp = p;                                /* adjust globl nxp pntr */
}

static void salcinit(void)    /* init the sorter mem space for a new section */
{                             /*  alloc 1st memblk if nec; init *nxp to this */
    ENVIRON *csound = &cenviron;
    if (curmem == NULL) {
      curmem = (char*) mmalloc(csound, (size_t) (MEMSIZ + MARGIN));
      memend = (char*) curmem + MEMSIZ;
    }
    nxp = (char*) curmem;
}

static void salcblk(void)       /* alloc a srtblk from current mem space:   */
{                               /*   align following *nxp, set new bp, nxp  */
    SRTBLK  *prvbp;             /*   set srtblk lnks, put op+blank in text  */
    ENVIRON *csound = &cenviron;

    if (nxp >= memend)          /* if this memblk exhausted */
      expand_nxp(csound);
    /* now allocate a srtblk from this space: */
    prvbp = bp;
    bp = (SRTBLK*) (((uintptr_t) nxp + (uintptr_t) 15) & ~((uintptr_t) 15));
    if (csound->frstbp == NULL)
      csound->frstbp = bp;
    if (prvbp != NULL)
      prvbp->nxtblk = bp;           /* link with prev srtblk        */
    bp->prvblk = prvbp;
    bp->nxtblk = NULL;
    nxp = &(bp->text[0]);
    *nxp++ = op;                    /* place op, blank into text    */
    *nxp++ = SP;
    *nxp = '\0';
}

void sfree(void)                 /* free all sorter allocated space */
{                                /*    called at completion of sort */
    ENVIRON *csound = &cenviron;
    if (curmem != NULL) {
      mfree(csound, curmem);
      curmem = NULL;
    }
    while (str != &inputs[0]) {
      if (!str->string) {
        fclose(str->file);
        mfree(csound, str->body);
      }
      str--;
    }
}

static void flushlin(void)      /* flush input to end-of-line;  inc lincnt */
{
    int c;
    while ((c = getscochar(0)) != LF && c != EOF)
      ;
    lincnt++;
    linpos = 0;
}

static int sget1(void)          /* get first non-white, non-comment char */
{
    ENVIRON *csound = &cenviron;
    int c;

 srch:
    while ((c = getscochar(1)) == SP || c == '\t' || c == LF)
      if (c == LF) {
        lincnt++;
        linpos = 0;
      }
    if (c == ';' || c == 'c') {
      flushlin();
      goto srch;
    }
    if (c == '\\') {            /* Deal with continuations and specials */
   /* csound->Message(csound, "Escaped\n"); */
    again:
      c = getscochar(1);
      if (c==';') {
        while ((c=getscochar(1)!='\n') && c!=EOF);
        goto srch;
      }
      if (c==' ' || c=='\t') goto again;
      if (c!='\n' && c!=EOF) {
        csound->Message(csound, Str("Improper \\"));
        while (c!='\n' && c!=EOF) c = getscochar(1);
      }
      goto srch;
    }
    if (c == '/') {             /* Could be a C-comment */
      c = getscochar(1);
      if (c!='*') {
        ungetscochar(c);
        c = '/';
      }
      else {                    /* It is a comment */
      top:
        while ((c=getscochar(1))!='*');
        if ((c=getscochar(1))!='/') {
          if (c!=EOF) goto top;
          return EOF;
        }
        goto srch;
      }
    }
    if (c == '#') {
                                /* Start Macro definition */
      char mname[100];
      int i=0;
      int arg = 0;
      int size = 100;
      MACRO *mm = (MACRO*)mmalloc(csound, sizeof(MACRO));
      mm->margs = MARGS;
      while (isspace(c = getscochar(1)));
      if (c=='d') {
        if ((c = getscochar(1))!='e' || (c = getscochar(1))!='f' ||
            (c = getscochar(1))!='i' || (c = getscochar(1))!='n' ||
            (c = getscochar(1))!='e') {
          csound->Message(csound, Str("Not #define"));
          flushlin();
          goto srch;
        }
        while (isspace(c = getscochar(1)));
        do {
          mname[i++] = c;
        } while (isalpha(c = getscochar(1)) || (i!=0 && (isdigit(c)||c=='_')));
        mname[i] = '\0';
        if (csound->oparms->msglevel)
          csound->Message(csound, Str("Macro definition for %s\n"), mname);
        mm->name = mmalloc(csound, i+1);
        strcpy(mm->name, mname);
        if (c == '(') { /* arguments */
/*        csound->Message(csound, "M-arguments: "); */
          do {
            while (isspace(c = getscochar(1)));
            i = 0;
            while (isalpha(c) || (i!=0 && (isdigit(c)||c=='_'))) {
              mname[i++] = c;
              c = getscochar(1);
            }
            mname[i] = '\0';
/*          csound->Message(csound, "%s\t", mname); */
            mm->arg[arg] = mmalloc(csound, i+1);
            strcpy(mm->arg[arg++], mname);
            if (arg>=mm->margs) {
              mm = (MACRO*)mrealloc(csound, mm,
                                    sizeof(MACRO)+mm->margs*sizeof(char*));
              mm->margs += MARGS;
            }
            while (isspace(c)) c = getscochar(1);
          } while (c=='\'' || c=='#');
          if (c!=')') {
            csound->Message(csound, Str("macro error\n"));
            flushlin();
            goto srch;
          }
        }
        mm->acnt = arg;
        i = 0;
        while ((c = getscochar(1))!= '#'); /* Skip to next # */
        mm->body = (char*)mmalloc(csound, 100);
        while ((c = getscochar(0))!= '#') {     /* Do not expand here!! */
          mm->body[i++] = c;
          if (i>= size)
            mm->body = mrealloc(csound, mm->body, size += 100);
          if (c=='\\') {
            mm->body[i++] = getscochar(0); /* Allow escaped # */
            if (i>= size)
              mm->body = mrealloc(csound, mm->body, size += 100);
          }
          if (c=='\n') lincnt++;
        }
        mm->body[i]='\0';
        mm->next = macros;
        macros = mm;
#ifdef MACDEBUG
        csound->Message(csound, Str("Macro %s with %d arguments defined\n"),
                                mm->name, mm->acnt);
#endif
        c = ' ';
        flushlin();
        goto srch;
      }
      else if (c=='i') {
        int delim;
        if ((c = getscochar(1))!='n' || (c = getscochar(1))!='c' ||
            (c = getscochar(1))!='l' || (c = getscochar(1))!='u' ||
            (c = getscochar(1))!='d' || (c = getscochar(1))!='e') {
          csound->Message(csound, Str("Not #include"));
          flushlin();
          goto srch;
        }
        while (isspace(c = getscochar(1)));
        delim = c;
        i = 0;
        while ((c=getscochar(1))!=delim) mname[i++] = c;
        mname[i]='\0';
        while ((c=getscochar(1))!='\n');
        input_cnt++;
        if (input_cnt>=input_size) {
          int old = str-inputs;
          input_size += 20;
/*        csound->Message(csound, "Expanding includes to %d\n", input_size); */
          inputs = mrealloc(csound, inputs, input_size*sizeof(struct in_stack));
          if (inputs == NULL) {
            csoundDie(csound, Str("No space for include files"));
          }
          str = &inputs[old];     /* In case it moves */
        }
        str++;
        str->string = 0;
        str->file = fopen_path(csound, mname, scorename, "INCDIR");
        if (str->file==0) {
          char buff[256];
          sprintf(buff,Str("Cannot open #include'd file %s\n"), mname);
          scorerr(buff);
/*           str--; input_cnt--; */
        }
        else {
          str->body = mmalloc(csound, strlen(csound->name_full)+1);
          strcpy(str->body, csound->name_full);
          goto srch;
        }
      }
      else if (c=='u') {
        if ((c = getscochar(1))!='n' || (c = getscochar(1))!='d' ||
            (c = getscochar(1))!='e' || (c = getscochar(1))!='f') {
          csound->Message(csound,Str("Not #undef"));
          flushlin();
          goto srch;
        }
        while (isspace(c = getscochar(1)));
        do {
          mname[i++] = c;
        } while (isalpha(c = getscochar(1))|| (i!=0 && (isdigit(c)||'_')));
        mname[i] = '\0';
        if (csound->oparms->msglevel)
          csound->Message(csound, Str("macro %s undefined\n"), mname);
        if (strcmp(mname, macros->name)==0) {
          MACRO *mm=macros->next;
          mfree(csound, macros->name); mfree(csound, macros->body);
          for (i=0; i<macros->acnt; i++)
            mfree(csound, macros->arg[i]);
          mfree(csound, macros); macros = mm;
        }
        else {
          MACRO *mm = macros;
          MACRO *nn = mm->next;
          while (strcmp(mname, nn->name)!=0) {
            mm = nn; nn = nn->next;
            if (nn==NULL) scorerr(Str("Undefining undefined macro"));
          }
          mfree(csound, nn->name); mfree(csound, nn->body);
          for (i=0; i<nn->acnt; i++)
            mfree(csound, nn->arg[i]);
          mm->next = nn->next; mfree(csound, nn);
        }
        while (c!='\n') c = getscochar(1); /* ignore rest of line */
        lincnt++;
      }
      else {
        csound->Message(csound, Str("unknown # option"));
        flushlin();
        goto srch;
      }
      flushlin();
      goto srch;
    }

    return(c);
}

static int getop(void)          /* get next legal opcode */
{
    ENVIRON *csound = &cenviron;
    int c;

 nextc:
    c = sget1();                    /* get first active char */

    switch (c) {                    /*   and check legality  */
    case 'a':
    case 'b':           /* Reset base clock */
    case 'e':           /* End of all */
    case 'f':
    case 'i':
    case 'm':           /* Mark this point */
    case 'n':           /* Duplicate from named position */
    case 'q':           /* quiet instrument ie mute */
    case 'r':           /* Repeated section */
    case 's':           /* Section */
    case 't':
    case 'v':           /* Local warping */
    case 'w':
    case 'x':
    case '{':           /* Section brackets */
    case '}':
    case EOF:
      break;            /* if ok, go with it    */
    default:            /*   else complain      */
      csound->Message(csound,
                       Str("sread: illegal opcode %c, sect %d line %d\n"),
                       c,csound->sectcnt,lincnt);
      csound->Message(csound,Str("      remainder of line flushed\n"));
      flushlin();
      goto nextc;
    }
    linpos++;
    return(c);
}

static int getpfld(void)             /* get pfield val from SCOREIN file */
{                                    /*      set sp, nxp                 */
    ENVIRON *csound = &cenviron;
    int  c;
    char *p;

    if ((c = sget1()) == EOF)        /* get 1st non-white,non-comment c  */
      return(0);

    if (!isdigit(c)                  /* if non-numeric          */
        && c != '.' && c != '+' && c != '-'   /*    and non-carry        */
        && c != '^' && c != 'n' && c != 'p'   /*    and non-special-char */
        && c != '<' && c != '>' && c != '{' && c != '}' && c != '(' && c != ')'
        && c != '"' && c != '~' ) {
      ungetscochar(c);                        /* then no more pfields    */
      if (linpos)
        csound->Message(csound,
                         Str("sread: unexpected char %c, sect %d line %d\n"),
                   c, csound->sectcnt, lincnt);
      return(0);                              /*    so return            */
    }
    p = sp = nxp;                         /* else start copying to text  */
    *p++ = c;
    linpos++;
    if (c == '"') {                           /* if have quoted string,  */
      /* IV - Oct 31 2002: allow string instr name for i and q events */
      if (bp->pcnt < 3 && !((op == 'i' || op == 'q') && !bp->pcnt)) {
        csound->Message(csound, Str("sread: illegally placed string, "
                                    "sect %d line %d\n"),
                                csound->sectcnt, lincnt);
        return(0);
      }
      while ((c = getscochar(1)) != '"') {
        if (c == LF || c == EOF) {
          csound->Message(csound,
                           Str("sread: unmatched quote, sect %d line %d\n"),
                           csound->sectcnt, lincnt);
          return(0);
        }
        *p++ = c;                       /*   copy to matched quote */
        /* **** CHECK **** */
        if (p >= memend)
          p = (char*) ((uintptr_t) p + expand_nxp(csound));
        /* **** END CHECK **** */
      }
      *p++ = c;
      goto blank;
    }
    while (((c = getscochar(1)) >= '0' && c <= '9')
           || c == '.' || c == '+' || c == '-' || c == 'e' || c == 'E'
           || c == 'n' || c == 'p'          /* else while legal chars,  */
           || c == '<' || c == '>'
           /*|| c == '{' || c == '}'*/ || c == '(' || c == ')'
           || c == '~') { /*   continue to bld string */
      *p++ = c;
      /* **** CHECK **** */
      if (p >= memend)
        p = (char*) ((uintptr_t) p + expand_nxp(csound));
      /* **** END CHECK **** */
    }
    ungetscochar(c);                        /* any illegal is delimiter */
 blank:
    *p++ = SP;
    nxp = p;                                /*  add blank      */
    return(1);                              /*  and report ok  */
}

MYFLT stof(char s[])              /* convert string to MYFLT  */
                                  /* (assumes no white space at beginning */
{                                 /*      but a blank or nl at end)       */
    ENVIRON *csound = &cenviron;  /* sbrandon adds: or a \0, on NeXT m68k */
    char *p;
    MYFLT x = (MYFLT)strtod(s, &p);
#if defined(NeXT) && defined(__BIG_ENDIAN__)
/* NeXT hardware only... */
    if (*(p - 1) == SP) p--;
#endif
    if (s == p || (*p != SP && *p != LF)) {
      csound->Message(csound,
                       Str("sread: illegal number format, sect %d line %d:  "),
                       csound->sectcnt, lincnt);
      p = s;
      while (*p != SP && *p != LF) {
        csound->Message(csound,"%c", *p);
        *p++ = '0';
      }
      csound->Message(csound,Str("   zero substituted.\n"));
      return FL(0.0);
    }
    return x;
}

