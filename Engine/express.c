/*
    express.c:

    Copyright (C) 1991 Barry Vercoe, John ffitch

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

#include "cs.h"                                 /*       EXPRESS.C       */
#include "namedins.h"

#define BITSET  0x17
#define BITCLR  0x16
#define BITFLP  0x15

#define LENTOT  200L            /* This one is OK */
#define TOKMAX  50L             /* Should be 50 but bust */
#define POLMAX  30L             /* This one is OK */
#define XERROR(CAUSE)   { strncpy(st.xprmsg,CAUSE,80);  goto error; }

typedef struct token {
        char    *str;
        short   prec;
} TOKEN;

static struct local_express {
  long    polmax;
  long    toklen;
  TOKEN   *token;
  TOKEN   *tokend;
  int     acount, kcount, icount, Bcount, bcount;
  char    xprmsg[80], *stringend;
  TOKEN   **revp, **pushp, **argp, **endlist;
} st;

static  TOKEN   *tokens = NULL;
static  TOKEN   **tokenlist = NULL;
static  int     toklength = TOKMAX;
static  const char    strminus1[] = "-1";
static  const char    strmult[] = "*";
static  void    putokens(void), putoklist(void);
static  int     nontermin(int);
extern  char    argtyp(char *);
extern  void    *mrealloc(void*, void*, size_t);
        void    resetouts(void);
/* IV - Jan 08 2003: this variable is used in rdorch.c, so cannot be static */
        int     argcnt_offs = 0, opcode_is_assign = 0, assign_type = 0;
        char    *assign_outarg = NULL;

void expRESET(void)
{
    mfree(&cenviron, polish); polish=NULL;
    st.polmax   = 0;
    tokenstring = NULL;
    st.toklen   = 0;
    tokens      = st.token = st.tokend = NULL;
    tokenlist   = st.revp = st.pushp = st.argp = st.endlist = NULL;
    toklength   = TOKMAX;
    resetouts();
    memset(st.xprmsg,0,80*sizeof(char));
    st.stringend = 0;
    argcnt_offs = 0;
    opcode_is_assign = assign_type = 0;
    assign_outarg = NULL;
}

void resetouts(void)
{
    st.acount = st.kcount = st.icount = st.Bcount = st.bcount = 0;
}

#define copystring(s) strsav_string(s)

int express(char *s)
{
    POLISH      *pp;
    char        b, c, d, e, nextc, *t, *op, outype = '\0', *sorig;
    int         open, prec, polcnt, argcnt;
    int         argcnt_max = 0;

    if (*s == '"')                 /* if quoted string, not an exprssion */
      return (0);
    if (tokens == NULL) {
      tokens       = (TOKEN*) mmalloc(&cenviron, (long)TOKMAX*sizeof(TOKEN));
      st.tokend    = tokens+TOKMAX;
      tokenlist    = (TOKEN**) mmalloc(&cenviron, (long)TOKMAX*sizeof(TOKEN*));
      polish       = (POLISH*) mmalloc(&cenviron, (long)POLMAX*sizeof(POLISH));
      st.polmax    = POLMAX;
      tokenstring  = mmalloc(&cenviron, LENTOT);
      st.stringend = tokenstring+LENTOT;
      st.toklen    = LENTOT;
    }
    sorig = s;
    if (tokenstring+strlen(s) >= st.stringend) {
      char *tt;
      TOKEN *ttt;
      long n = st.toklen + LENTOT+strlen(s);
      tt = (char *)mrealloc(&cenviron, tokenstring, n);
      for (ttt=tokens; ttt<=st.token; ttt++) /* Adjust all previous tokens */
        ttt->str += (tt-tokenstring);
      tokenstring = tt;               /* Reset string and length */
      st.stringend = tokenstring + (st.toklen = n);
      printf(Str("Token length extended to %ld\n"), st.toklen);
    }

    st.token = tokens;
    st.token->str = t = tokenstring;
    open = 1;
    while ((c = *s++)) {
      if (open) {                   /* if unary possible here,   */
        if (c == '+')               /*   look for signs:         */
          continue;
        if (c == '-') {             /* neg const:  get past sign */
          if (*s == '.' || (*s >= '0' && *s <= '9'))
            *t++ = c;
          else {                    /* neg symbol: prv / illegal */
            if (st.token > tokens
                && *(st.token-1)->str == '/')
              XERROR(Str("divide by unary minus"))
            st.token->str = (char*) strminus1; st.token++;
            st.token->str = (char*) strmult; st.token++;
            st.token->str = t;     /* else -1 * symbol */
          }
          c = *s++;                /* beg rem of token */
        }
        else if (c == '*' || c == '/' || c == '%')  /* unary mlt, div */
          XERROR(Str("unary mult or divide"))       /*   illegal */
            open = 0;
      }
      *t++ = c;                    /* copy this character or    */
      if (((nextc = *s) == c && (c == '&' || c == '|')) /* double op */
          || (nextc == '=' && (c=='<' || c=='>' || c=='=' || c=='!')))
        *t++ = c = *s++, open = 1;
      else if ( c == '(' || c == '+' || c == '-' || c == '*' || c == '/' ||
                c == '%' || c == '>' || c == '<' || c == '=' || c == '&' ||
                c == '|' || c == '?' || c == ':' || c == '#' || c == '¬') {
        if (c == '&') *(t-1) = BITCLR;
        else if (c == '|') *(t-1) = BITSET;
        else if (c == '#') *(t-1) = BITFLP;
        open = 1;                     /* decl if unary can follow */
      }
      else if (nontermin(c))
        while (nontermin(*s))         /* if not just a termin char */
          *t++ = *s++;                /*      copy entire token    */
      *t++ = '\0';                    /* terminate this token      */
      if (t >= st.stringend) {        /* Extend token length as required */
        XERROR(Str("token storage LENTOT exceeded"));
      }
      if ((st.tokend - st.token)<= 4) { /* Extend token array and friends */
        int n = st.token - tokens;
        tokens =
          (TOKEN*)mrealloc(&cenviron, tokens,
                           (toklength+TOKMAX)*sizeof(TOKEN));
        tokenlist =
          (TOKEN**) mrealloc(&cenviron, tokenlist,
                             (toklength+TOKMAX)*sizeof(TOKEN*));
        toklength += TOKMAX;
/*         printf(Str("Tokens length extended to %d\n"), toklength); */
        st.token  = tokens + n;
        st.tokend = tokens + toklength;
      }
      /* IV - Jan 08 2003: check if the output arg of an '=' opcode is */
      /* used in the expression (only if optimisation is enabled) */
      if (opcode_is_assign == 1)
        if (!strcmp(st.token->str, assign_outarg))      /* if yes, mark as */
          opcode_is_assign = 2;                         /* dangerous case  */
      (++st.token)->str = t;                  /* & record begin of nxt one */
    }
    st.token->str = NULL;             /* expr end:  terminate tokens array */
    if (st.token - tokens <= 1)       /*              & return if no expr  */
      return(0);

    st.token = tokens;
    while ((s = st.token->str) != NULL) {  /* now for all tokens found, */
      c = *s;
      switch ((int) c) {        /* IV - Jan 15 2003 */
                                /* assign precedence values */
      case ')':         prec = 0;       break;
      case ',':         prec = 1;       break;
      case '?':
      case ':':         prec = 2;       break;
      case '|':         prec = 3;       break;
      case '&':         prec = 4;       break;
      case '<':
      case '=':
      case '>':
      case '!':         prec = 5;       break;
      case '+':
      case '-':         prec = (s[1] == '\0' ? 6 : 16); break;
      case '*':
      case '/':
      case '%':         prec = 7;       break;
      case '^':         prec = 8;       break;
      case BITSET:
      case BITFLP:      prec = 9;       break;
      case BITCLR:      prec = 10;      break;
      case '¬':         prec = 11;      break;
      case '(':         prec = 13;      break;
      default:
        if (((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) &&
            (t = (st.token+1)->str) != NULL && *t == '(') {
          prec = 12;            /* function call */
        }
        else {
          c = argtyp(s);
          /* terms: precedence depends on type (a < k < i) */
          if      (c == 'a')    prec = 14;
          else if (c == 'k')    prec = 15;
          else    prec = 16;
        }
        break;
      }
      (st.token++)->prec = prec;
    }
    if (O.odebug) putokens();

#define CONDVAL 2
#define LOGOPS  3
#define RELOPS  5
#define AOPS    6
#define BITOPS  9
#define FCALL   12
#define TERMS   14

    st.token = tokens;
    st.revp = tokenlist;
    st.pushp = st.endlist = tokenlist+toklength;      /* using precedence vals, */
    while (st.token->str != NULL) {             /*  put tokens rev pol order */
      if (*st.token->str == '(') {
        st.token->prec = -1;
        *--st.pushp = st.token++;
      }
      else if (st.pushp < st.endlist && (*st.pushp)->prec >= st.token->prec) {
        if (*st.token->str == ':' && *(*st.pushp)->str == '?')
          *st.pushp = st.token++;               /* replace ? with : */
        else *st.revp++ = *st.pushp++;
      }
      else if (*st.token->str == ')') {
        if (st.token++ && *(*st.pushp++)->str != '(')
          XERROR(Str("within parens"))
      }
      else if ((st.token+1)->str!=NULL && st.token->prec < (st.token+1)->prec)
        *--st.pushp = st.token++;
      else *st.revp++ = st.token++;
    }
    while (st.pushp < st.endlist)
      *st.revp++ = *st.pushp++;

    st.endlist = st.revp;                       /* count of pol operators */
    if (O.odebug) putoklist();
    for (st.revp=tokenlist, polcnt=0;  st.revp<st.endlist; )
      if ((*st.revp++)->prec < TERMS)           /*  is no w. prec < TERMS */
        polcnt++;
    if (!polcnt) {                              /* if no real operators,  */
      strcpy(tokenstring,tokenlist[0]->str);    /* cpy arg to beg str     */
      return(-1);                               /*  and return this info  */
    }
    if (polcnt >= st.polmax) {
      st.polmax = polcnt+POLMAX;
      polish = (POLISH*) mrealloc(&cenviron, polish,st.polmax*sizeof(POLISH));
/*       printf(Str("Extending Polish array length %ld\n"), st.polmax); */
/*      XERROR("polish storage POLMAX exceeded"); */
    }
    pp = &polish[polcnt-1];
    op = pp->opcod;
    for (st.revp=st.argp=tokenlist; st.revp<st.endlist; ) { /* for all tokens:  */
      char buffer[1024];
      if ((prec = (*st.revp)->prec) >= TERMS) {
        *st.argp++ = *st.revp++;                     /* arg: push back    */
        continue;                                    /*      till later   */
      }
      argcnt = st.argp - tokenlist;
      if (prec == FCALL && argcnt >= 1) {            /*   function call:  */
        pp->incount = 1;                             /*     takes one arg */
        pp->arg[1] = copystring((*--st.argp)->str);
        c = argtyp(pp->arg[1]);                      /* whose aki type */
        if (c == 'B' || c == 'b')
          XERROR(Str("misplaced relational op"))
        if (c != 'a' && c != 'k')
          c = 'i';                                   /*   (simplified)  */
        sprintf(op, "%s.%c", (*st.revp)->str, c);    /* Type at end now */
        if (strcmp(op,"i.k") == 0) {
          outype = 'i';                              /* i(karg) is irreg. */
          if (pp->arg[1][0] == '#' && pp->arg[1][1] == 'k') {
            /* IV - Jan 15 2003: input arg should not be a k-rate expression */
            if (O.expr_opt) {
              XERROR(Str("i() with expression argument not "
                         "allowed with --expression-opt"));
            }
            else {
              printf(Str("WARNING: i() should not be used with "
                         "expression argument\n"));
            }
          }
        }
        else if (strcmp(op,"a.k") == 0)
          outype = 'a';                     /* a(karg) is irreg. */
        else outype = c;                    /* else outype=intype */
      }
      else if (prec >= BITOPS && argcnt >= 2) { /* bit op:    */
        if ((c = *(*st.revp)->str) == BITSET)
          strcpy(op,"or");
        else if (c == BITFLP)
          strcpy(op,"xor");
        else if (c == BITCLR)
          strcpy(op,"and");
        else printf(Str("Expression got lost\n"));
        goto common_ops;
      }
      else if (prec >= BITOPS && argcnt == 1) { /* bit op:    */
        if ((c = *(*st.revp)->str) == '¬')
          strcpy(op,"not");
        else printf(Str("Expression got lost\n"));
        pp->incount = 1;                    /*   copy 1 arg txts */
        pp->arg[1] = copystring((*--st.argp)->str);
        e = argtyp(pp->arg[1]);
        if (e == 'B' || e == 'b')
          XERROR(Str("misplaced relational op"))
/*              printf("op=%s e=%c c=%c\n", op, e, c); */
        if (e == 'a') {                     /*   to complete optxt*/
          if (c=='¬')
            strcat(op,".a");
          outype = 'a';
        }
        else if (e == 'k') {
          if (c == '¬') strcat(op,".k");
          outype = 'k';
        }
        else {
          if (c == '¬') strcat(op,".i");
          outype = 'i';
        }
      }
      else if (prec >= AOPS && argcnt >= 2) { /* arith op:    */
        if ((c = *(*st.revp)->str) == '+')
          strcpy(op,"add");
        else if (c == '-')
          strcpy(op,"sub");               /*   create op text */
        else if (c == '*')
          strcpy(op,"mul");
        else if (c == '/')
          strcpy(op,"div");
        else if (c == '%')
          strcpy(op,"mod");
        else if (c == '^')
          strcpy(op,"pow");
        else printf(Str("Expression got lost\n"));
      common_ops:
        pp->incount = 2;                    /*   copy 2 arg txts */
        pp->arg[2] = copystring((*--st.argp)->str);
        pp->arg[1] = copystring((*--st.argp)->str);
        e = argtyp(pp->arg[1]);
        d = argtyp(pp->arg[2]);             /*   now use argtyps */
        if (e == 'B' || e == 'b' || d == 'B' || d == 'b' )
          XERROR(Str("misplaced relational op"))
/*              printf("op=%s e=%c c=%c d=%c\n", op, e, c, d); */
        if (e == 'a') {                     /*   to complet optxt*/
          if (c=='^' && (d == 'c' || d == 'k'|| d == 'i' || d == 'p'))
            strcat(op,".a");
          else if (d == 'a') strcat(op,".aa");
          else               strcat(op,".ak");
          outype = 'a';
        }
        else if (d == 'a') {
          strcat(op,".ka");
          outype = 'a';
        }
        else if (e == 'k' || d == 'k') {
          if (c == '^') strcat(op,".k");
          else          strcat(op,".kk");
          outype = 'k';
        }
        else {
          if (c == '^') strcat(op,".i");
          else          strcat(op,".ii");
          outype = 'i';
        }
      }
      else if (prec >= RELOPS && argcnt >= 2) { /* relationals:   */
        strcpy(op,(*st.revp)->str);            /*   copy rel op    */
        if (strcmp(op,"=") == 0)
          strcpy(op,"==");
        pp->incount = 2;                    /*   & 2 arg txts   */
        pp->arg[2] = copystring((*--st.argp)->str);
        pp->arg[1] = copystring((*--st.argp)->str);
        c = argtyp(pp->arg[1]);
        d = argtyp(pp->arg[2]);             /*   now use argtyps */
        if (c == 'a' || d == 'a')           /*   to determ outs  */
          XERROR(Str("audio relational"))
        if (c == 'B' || c == 'b' || d == 'B' || d == 'b' )
          XERROR(Str("misplaced relational op"))
        if (c == 'k' || d == 'k')
          outype = 'B';
        else outype = 'b';
      }
      else if (prec >= LOGOPS && argcnt >= 2) { /* logicals:    */
        strcpy(op,(*st.revp)->str);            /*   copy rel op  */
        pp->incount = 2;                    /*   & 2 arg txts */
        pp->arg[2] = copystring((*--st.argp)->str);
        pp->arg[1] = copystring((*--st.argp)->str);
        c = argtyp(pp->arg[1]);
        d = argtyp(pp->arg[2]);             /*   now use argtyps */
        if (c == 'b' && d == 'b')           /*   to determ outs  */
          outype = 'b';
        else if ((c == 'B' || c == 'b') &&
                 (d == 'B' || d == 'b'))
          outype = 'B';
        else XERROR(Str("incorrect logical argumemts"))
               }
      else if (prec == CONDVAL && argcnt >= 3) { /* cond vals:     */
        strcpy(op,": ");                    /*   init op as ': ' */
        pp->incount = 3;                    /*   & cpy 3 argtxts */
        pp->arg[3] = copystring((*--st.argp)->str);
        pp->arg[2] = copystring((*--st.argp)->str);
        pp->arg[1] = copystring((*--st.argp)->str);
        b = argtyp(pp->arg[1]);
        c = argtyp(pp->arg[2]);
        d = argtyp(pp->arg[3]);
        if ((b != 'B' && b != 'b') ||       /*   chk argtypes, */
            c == 'B' || c == 'b'   || d == 'B' || d == 'b' ||
            (c == 'a' && d != 'a') || (d == 'a' && c != 'a'))
          XERROR(Str("incorrect cond value format"))
        outype = 'i';                       /*   determine outyp */
        if (b == 'B' || c == 'k' || d == 'k')
          outype = 'k';
        if (c == 'a' || d == 'a')
          outype = 'a';
        *(op+1) = outype;                   /*   & complet opcod */
      }
      else XERROR(Str("insufficient terms"))
      s = &buffer[0] /* pp->arg[0] */;      /* now create outarg acc. to type */
      if (!O.expr_opt) {
        /* IV - Jan 08 2003: old code: should work ... */
        if (outype=='a') sprintf(s,"#a%d",st.acount++);
        else if (outype=='k') sprintf(s,"#k%d",st.kcount++);
        else if (outype=='B') sprintf(s,"#B%d",st.Bcount++);
        else if (outype=='b') sprintf(s,"#b%d",st.bcount++);
        else sprintf(s,"#i%d",st.icount++);
      }
      else {
        int ndx = (int) (st.argp - tokenlist);     /* argstack index */
        if (opcode_is_assign == 1       &&
            (int) ndx == 0              &&
            (int) outype == assign_type &&
            strchr("aki", assign_type) != NULL) {
          /* IV - Jan 08 2003: if the expression is an input arg to the '=' */
          /* opcode, the output type is appropriate, and the argument stack */
          /* is empty, there is no need for a temporary variable, */
          strcpy(s, assign_outarg); /* instead just use the opcode outarg */
          /* note: this is not safe if the expression contains the output */
          /* variable; if that is the case, opcode_is_assign is set to 2 */
        }
        else {
          int cnt = ndx;
          /* IV - Jan 08 2003: make optimal use of temporary variables by */
          /* using the argstack pointer as index. This is not reliable with */
          /* i-rate variables, so we limit the optimisation to a- and k-rate */
          /* operations only. */
          /* If there are multiple expressions on the same line, use */
          /* different indexes for the tmp variables of each expression. */
/*        if (!cnt) */
            cnt += argcnt_offs;         /* IV - Jan 15 2003 */
          if (outype == 'a')        sprintf(s, "#a%d", cnt);
          else if (outype == 'k')   sprintf(s, "#k%d", cnt);
          else if (outype == 'B')   sprintf(s, "#B%d", st.Bcount++);
          else if (outype == 'b')   sprintf(s, "#b%d", st.bcount++);
          else                      sprintf(s, "#i%d", st.icount++);
          /* IV - Jan 08 2003: count max. stack depth in order to allow */
          /* generating different indexes for temporary variables of */
          /* separate expressions on the same line (see also below). */
          /* N.B. argcnt_offs is reset in rdorch.c when a new line is read. */
          if (ndx > argcnt_max) argcnt_max = ndx;
        }
      }
      (*st.argp++)->str = pp->arg[0] = copystring(s);/* & point argstack there */
      st.revp++;
      pp--;   op = pp->opcod;                     /* prep for nxt pol */
    }
    if (st.argp - tokenlist == 1) {
      /* IV - Jan 08 2003: do not re-use temporary variables between */
      /* expressions of the same line */
      argcnt_offs += (argcnt_max + 1);
      /* if wrote to output arg of '=' last time, '=' can be omitted */
      if (opcode_is_assign != 0         &&      /* only if optimising */
          (int) outype == assign_type   &&
          strchr("aki", assign_type) != NULL) {
        /* replace outarg if necessary */
        if (strcmp(tokenlist[0]->str, assign_outarg)) {
          /* now the last op will use the output of '=' directly */
          /* the pp + 1 is because of the last pp-- */
          tokenlist[0]->str = (pp + 1)->arg[0] = copystring(assign_outarg);
        }
        /* mark as optimised away */
        opcode_is_assign = -1;
      }
      return(polcnt);                           /* finally, return w. polcnt */
    }
    XERROR(Str("term count"))

 error:
    synterr(Str("expression syntax"));    /* or gracefully report error*/
    printf(" %s: %s\n",st.xprmsg,sorig);
    strcpy(tokenstring,"1");
    return(-1);
}

static int nontermin(int c)
{
    if (c == '(' || c == ')' || c == '\0'|| c == '^' || c == '+' ||
        c == '-' || c == '*' || c == '/' || c == '%' || c == '>' ||
        c == '<' || c == '=' || c == '!' || c == '&' || c == '|' ||
        c == '?' || c == ':' )
      return(0);
    else return(1);
}

static void putokens(void)      /* for debugging check only */
{
    TOKEN       *tp = tokens;
    while (tp->str != NULL)
      printf("%s\t", (tp++)->str);
    printf("\n");
}

static void putoklist(void)     /*      ditto           */
{
    TOKEN       **tpp = tokenlist;
    while (tpp < st.endlist)
      printf("%s\t", (*tpp++)->str);
    printf("\n");
}

