/*
    express.c:

    Copyright (C) 1991 Barry Vercoe, John ffitch, Istvan Varga

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

#include "csoundCore.h"                         /*       EXPRESS.C       */
#include "namedins.h"

#define BITSHL  0x19
#define BITSHR  0x18
#define BITSET  0x17
#define BITCLR  0x16
#define BITFLP  0x15

#define POLMAX  30L             /* This one is OK */
#define XERROR(CAUSE)   { strncpy(xprmsg,CAUSE,128);  goto error; }

static  const char    strminus1[] = "-1";
static  const char    strmult[] = "*";

static  void    putokens(CSOUND*), putoklist(CSOUND*);
static  int     nontermin(int);

#define copystring(s) strsav_string(csound, s)

static CS_NOINLINE char *extend_tokenstring(CSOUND *csound, size_t len)
{
    size_t  newLen;
    char    *tt;
    TOKEN   *ttt;

    newLen = (size_t) csound->toklen;
    do {
      newLen = ((newLen + (newLen >> 2)) | (size_t) 127) + (size_t) 1;
    } while (newLen <= len);
    tt = (char*) mrealloc(csound, csound->tokenstring, newLen + (size_t) 128);
    /* Adjust all previous tokens */
    if (LIKELY(csound->token)) {
      for (ttt = csound->tokens; ttt <= csound->token; ttt++)
        ttt->str += (tt - csound->tokenstring);
    }
    csound->tokenstring = tt;               /* Reset string and length */
    csound->toklen = (int32) newLen;
    csound->stringend = csound->tokenstring + csound->toklen;
    /* if (UNLIKELY(newLen != (size_t) 128)) */
    /*   csound->Message(csound, Str("Token length extended to %ld\n"), */
    /*                           csound->toklen); */
    return &(csound->tokenstring[len]);
}

int express(CSOUND *csound, char *s)
{
    POLISH      *pp;
    char        xprmsg[128];
    char        b, c, d, e, nextc, *t, *op, outype = '\0', *sorig;
    int         open, prec, polcnt, argcnt;
    int         argcnt_max = 0;

    if (*s == '"')                 /* if quoted string, not an exprssion */
      return (0);
    if (UNLIKELY(csound->tokens == NULL)) {
      csound->tokens    = (TOKEN*) mmalloc(csound, TOKMAX * sizeof(TOKEN));
      csound->tokend    = csound->tokens + TOKMAX;
      csound->tokenlist = (TOKEN**) mmalloc(csound, TOKMAX * sizeof(TOKEN*));
      csound->polish    = (POLISH*) mmalloc(csound, POLMAX * sizeof(POLISH));
      csound->polmax    = POLMAX;
      csound->tokenstring = NULL;
      csound->stringend = NULL;
      csound->toklen    = 0L;
    }
    sorig = s;
    if (UNLIKELY(strlen(s) >= (size_t) csound->toklen))
      extend_tokenstring(csound, strlen(s));

    csound->token = csound->tokens;
    csound->token->str = t = csound->tokenstring;
    open = 1;
    while ((c = *s++)) {
      if (open) {                   /* if unary possible here,   */
        if (c == '+')               /*   look for signs:         */
          continue;
        if (c == '-') {             /* neg const:  get past sign */
          /* the check for 'd' is for correctly parsing "-0dbfs" */
          if (*s == '.' || (*s >= '0' && *s <= '9' && s[1] != 'd'))
            *t++ = c;
          else {                    /* neg symbol: prv / illegal */
            if (UNLIKELY(csound->token > csound->tokens &&
                         *(csound->token-1)->str == '/'))
              XERROR(Str("divide by unary minus"))
            csound->token->str = (char*) strminus1; csound->token++;
            csound->token->str = (char*) strmult; csound->token++;
            csound->token->str = t; /* else -1 * symbol */
          }
          c = *s++;                 /* beg rem of token */
        }
        else if (UNLIKELY(c == '*' || c == '/' || c == '%'))  /* unary mlt, div */
          XERROR(Str("unary mult or divide"))       /*   illegal      */
            open = 0;
      }
      *t++ = c;                     /* copy this character or    */
      if (((nextc = *s) == c && (c == '&' || c == '|')) ||  /* double op  */
          (nextc == '=' && (c=='<' || c=='>' || c=='=' || c=='!'))) {
        *t++ = c = *s++;
        open = 1;
      }
      else if (nextc == c && (c == '<' || c == '>')) {
        *(t - 1) = (char) (c == '<' ? BITSHL : BITSHR);
        s++; open = 1;
      }
      /* Would it be better to use strchr("(+-*%/><=&|?:#~\254",c)!=NULL ? */
      else if (c == '(' || c == '+' || c == '-' || c == '*' || c == '/' ||
               c == '%' || c == '>' || c == '<' || c == '=' || c == '&' ||
               c == '|' || c == '?' || c == ':' || c == '#' || c == '\254' ||
               c == '~') {
        if (c == '&') *(t-1) = BITCLR;
        else if (c == '|') *(t-1) = BITSET;
        else if (c == '#') *(t-1) = BITFLP;
        open = 1;                     /* decl if unary can follow  */
      }
      else if (nontermin(c)) {        /* if not just a termin char */
        if (c == '.' || (c >= '0' && c <= '9')) {
          char  *tmp = --s;
          double tt = strtod(s, &tmp);     /*      copy constant   */
          while (++s < tmp)
            *t++ = *s;
          /* also copy any trailing characters after a constant,   */
          /* which will be a syntax error later (in constndx())    */
          /* for anything other than "0dbfs" */
        }
        while (nontermin(*s))
          *t++ = *s++;                /*      copy entire token    */
      }
      *t++ = '\0';                    /* terminate this token      */
      if (t >= csound->stringend) {       /* Extend token length as required */
        t = extend_tokenstring(csound, (size_t) (t - csound->tokenstring));
      }
      if ((csound->tokend - csound->token) <= 4) {
        /* Extend token array and friends */
        int n = csound->token - csound->tokens;
        csound->tokens = (TOKEN*) mrealloc(csound, csound->tokens,
                                                   (csound->toklength + TOKMAX)
                                                     * sizeof(TOKEN));
        csound->tokenlist = (TOKEN**) mrealloc(csound, csound->tokenlist,
                                               (csound->toklength + TOKMAX)
                                                 * sizeof(TOKEN*));
        csound->toklength += TOKMAX;
        csound->token  = csound->tokens + n;
        csound->tokend = csound->tokens + csound->toklength;
      }
      /* IV - Jan 08 2003: check if the output arg of an '=' opcode is */
      /* used in the expression (only if optimisation is enabled) */
      if (csound->opcode_is_assign == 1)
        if (!strcmp(csound->token->str,
                    csound->assign_outarg))   /* if yes, mark as */
          csound->opcode_is_assign = 2;       /* dangerous case  */
      (++csound->token)->str = t;             /* & record begin of nxt one */
    }
    csound->token->str = NULL;        /* expr end:  terminate tokens array */
    if (csound->token - csound->tokens <= 1)    /*    & return if no expr  */
      return(0);

    csound->token = csound->tokens;
    while ((s = csound->token->str) != NULL) {  /* now for all tokens found, */
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
      case BITSET:      prec = 6;       break;
      case BITFLP:      prec = 7;       break;
      case BITCLR:      prec = 8;       break;
      case BITSHL:
      case BITSHR:      prec = 9;       break;
      case '+':
      case '-':         prec = (s[1] == '\0' ? 10 : 18); break;
      case '*':
      case '/':
      case '%':         prec = 11;      break;
      case '^':         prec = 12;      break;
      case '~':
      case '\254':      prec = 13;      break;
      case '(':         prec = 15;      break;
      default:
        if (((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) &&
            (t = (csound->token + 1)->str) != NULL && *t == '(') {
          prec = 14;            /* function call */
        }
        else {
          c = argtyp(csound, s);
          /* terms: precedence depends on type (a < k < i) */
          if      (c == 'a')    prec = 16;
          else if (c == 'k')    prec = 17;
          else    prec = 18;
        }
        break;
      }
      (csound->token++)->prec = prec;
    }
    if (csound->oparms->odebug) putokens(csound);

#define CONDVAL 2
#define LOGOPS  3
#define RELOPS  5
#define BITOPS  6
#define AOPS    10
#define BITNOT  13
#define FCALL   14
#define TERMS   16

    csound->token = csound->tokens;
    csound->revp  = csound->tokenlist;
    csound->pushp = csound->endlist = csound->tokenlist + csound->toklength;
                                                /* using precedence vals,    */
    while (csound->token->str != NULL) {        /*  put tokens rev pol order */
      if (*csound->token->str == '(') {
        csound->token->prec = -1;
        *--csound->pushp = csound->token++;
      }
      else if (csound->pushp < csound->endlist &&
               (*csound->pushp)->prec >= csound->token->prec) {
        if (*csound->token->str == ':' && *(*csound->pushp)->str == '?')
          *csound->pushp = csound->token++;     /* replace ? with : */
        else *csound->revp++ = *csound->pushp++;
      }
      else if (*csound->token->str == ')') {
        if (csound->token++ && *(*csound->pushp++)->str != '(')
          XERROR(Str("within parens"))
      }
      else if ((csound->token + 1)->str != NULL &&
               csound->token->prec < (csound->token + 1)->prec)
        *--csound->pushp = csound->token++;
      else *csound->revp++ = csound->token++;
    }
    while (csound->pushp < csound->endlist)
      *csound->revp++ = *csound->pushp++;

    csound->endlist = csound->revp;             /* count of pol operators */
    if (csound->oparms->odebug) putoklist(csound);
    for (csound->revp = csound->tokenlist, polcnt = 0;
         csound->revp < csound->endlist; )
      if ((*csound->revp++)->prec < TERMS)      /*  is no w. prec < TERMS */
        polcnt++;
    if (!polcnt) {                              /* if no real operators,  */
      strcpy(csound->tokenstring,
             csound->tokenlist[0]->str);        /*  cpy arg to beg str    */
      return(-1);                               /*  and return this info  */
    }
    if (polcnt >= csound->polmax) {
      csound->polmax = polcnt + POLMAX;
      csound->polish = (POLISH*) mrealloc(csound, csound->polish,
                                          csound->polmax * sizeof(POLISH));
    }
    pp = &(csound->polish[polcnt - 1]);
    op = pp->opcod;
    for (csound->revp = csound->argp = csound->tokenlist;
         csound->revp < csound->endlist; ) {    /* for all tokens: */
      char buffer[1024];
      if ((prec = (*csound->revp)->prec) >= TERMS) {
        *csound->argp++ = *csound->revp++;      /* arg: push back    */
        continue;                               /*      till later   */
      }
      argcnt = csound->argp - csound->tokenlist;
      if (prec == FCALL && argcnt >= 1) {       /*   function call:  */
        pp->incount = 1;                        /*     takes one arg */
        pp->arg[1] = copystring((*--csound->argp)->str);
        c = argtyp(csound, pp->arg[1]);         /* whose aki type */
        if (c == 'B' || c == 'b')
          XERROR(Str("misplaced relational op"))
        if (c != 'a' && c != 'k')
          c = 'i';                              /*   (simplified)  */
        sprintf(op, "%s.%c", (*csound->revp)->str, c); /* Type at end now */
        if (strcmp(op,"i.k") == 0) {
          outype = 'i';                          /* i(karg) is irreg. */
          if (UNLIKELY(pp->arg[1][0] == '#' && pp->arg[1][1] == 'k')) {
            /* IV - Jan 15 2003: input arg should not be a k-rate expression */
            if (csound->oparms->expr_opt) {
              XERROR(Str("i() with expression argument not "
                         "allowed with --expression-opt"));
            }
            else {
              csound->Message(csound,
                              Str("WARNING: i() should not be used with "
                                  "expression argument\n"));
            }
          }
        }
        else if (strcmp(op,"a.k") == 0)     /* a(karg) is irreg. */
          outype = 'a';
        else if (strcmp(op,"k.i") == 0)     /* k(iarg) is irreg. */
          outype = 'K';         /* K-type is k-rate assigned at i-time only */
        else outype = c;                    /* else outype=intype */
      }
      else if (prec >= BITOPS && prec < AOPS && argcnt >= 2) {  /* bit op:  */
        c = *(*csound->revp)->str;
        switch (c) {
          case BITSHL:  strcpy(op, "shl");  break;
          case BITSHR:  strcpy(op, "shr");  break;
          case BITSET:  strcpy(op, "or");   break;
          case BITFLP:  strcpy(op, "xor");  break;
          case BITCLR:  strcpy(op, "and");  break;
          default:      csound->Message(csound, Str("Expression got lost\n"));
        }
        goto common_ops;
      }
      else if (prec == BITNOT && argcnt == 1) { /* bit op:    */
        c = *(*csound->revp)->str;
        pp->incount = 1;                    /*   copy 1 arg txts    */
        pp->arg[1] = copystring((*--csound->argp)->str);
        e = argtyp(csound, pp->arg[1]);
        if (UNLIKELY(e == 'B' || e == 'b'))
          XERROR(Str("misplaced relational op"));
        if (LIKELY(c == '\254' || c == '~')) {
          strcpy(op, "not");                /*   to complete optxt  */
          switch (e) {
          case 'a':   strncat(op, ".a", 12);   outype = 'a';   break;
          case 'k':   strncat(op, ".k", 12);   outype = 'k';   break;
          default:    strncat(op, ".i", 12);   outype = 'i';   break;
          }
        }
        else
          csound->Message(csound, Str("Expression got lost\n"));
      }
      else if (prec >= AOPS && prec < BITNOT && argcnt >= 2) {  /* arith op: */
        c = *(*csound->revp)->str;
        switch (c) {                                    /*   create op text */
          case '+': strncpy(op, "add", 12);  break;
          case '-': strncpy(op, "sub", 12);  break;
          case '*': strncpy(op, "mul", 12);  break;
          case '/': strncpy(op, "div", 12);  break;
          case '%': strncpy(op, "mod", 12);  break;
          case '^': strncpy(op, "pow", 12);  break;
          default:  csound->Message(csound, Str("Expression got lost\n"));
        }
 common_ops:
        pp->incount = 2;                    /*   copy 2 arg txts */
        pp->arg[2] = copystring((*--csound->argp)->str);
        pp->arg[1] = copystring((*--csound->argp)->str);
        e = argtyp(csound, pp->arg[1]);
        d = argtyp(csound, pp->arg[2]);     /*   now use argtyps */
        if (UNLIKELY(e == 'B' || e == 'b' || d == 'B' || d == 'b' ))
          XERROR(Str("misplaced relational op"))
/*      csound->Message(csound, "op=%s e=%c c=%c d=%c\n", op, e, c, d); */
        if (e == 'a') {                     /*   to complet optxt*/
          if (c=='^' && (d == 'c' || d == 'k'|| d == 'i' || d == 'p'))
            strncat(op,".a",12);
          else if (d == 'a') strncat(op,".aa",12);
          else               strncat(op,".ak",12);
          outype = 'a';
        }
        else if (d == 'a') {
          strncat(op,".ka",12);
          outype = 'a';
        }
        else if (e == 'k' || d == 'k') {
          if (c == '^') strncat(op,".k",12);
          else          strncat(op,".kk",12);
          outype = 'k';
        }
        else {
          if (c == '^') strncat(op,".i",12);
          else          strncat(op,".ii",12);
          outype = 'i';
        }
      }
      else if (prec == RELOPS && argcnt >= 2) { /* relationals:     */
        strncpy(op, (*csound->revp)->str, 12);   /*   copy rel op    */
        if (strcmp(op, "=") == 0)
          strncpy(op, "==", 12);
        pp->incount = 2;                        /*   & 2 arg txts   */
        pp->arg[2] = copystring((*--csound->argp)->str);
        pp->arg[1] = copystring((*--csound->argp)->str);
        c = argtyp(csound, pp->arg[1]);
        d = argtyp(csound, pp->arg[2]);     /*   now use argtyps */
        if (UNLIKELY(c == 'a' || d == 'a'))           /*   to determ outs  */
          XERROR(Str("audio relational"))
            if (UNLIKELY(c == 'B' || c == 'b' || d == 'B' || d == 'b' ))
          XERROR(Str("misplaced relational op"))
        if (c == 'k' || d == 'k')
          outype = 'B';
        else outype = 'b';
      }
      else if (prec >= LOGOPS && prec < RELOPS && argcnt >= 2) { /* logicals: */
        strncpy(op, (*csound->revp)->str, 12);   /*   copy rel op  */
        pp->incount = 2;                    /*   & 2 arg txts */
        pp->arg[2] = copystring((*--csound->argp)->str);
        pp->arg[1] = copystring((*--csound->argp)->str);
        c = argtyp(csound, pp->arg[1]);
        d = argtyp(csound, pp->arg[2]);     /*   now use argtyps */
        if (c == 'b' && d == 'b')           /*   to determ outs  */
          outype = 'b';
        else if ((c == 'B' || c == 'b') &&
                 (d == 'B' || d == 'b'))
          outype = 'B';
        else XERROR(Str("incorrect logical arguments"))
      }
      else if (prec == CONDVAL && argcnt >= 3) { /* cond vals:     */
        strncpy(op, ": ", 12);                   /*   init op as ': ' */
        pp->incount = 3;                    /*   & cpy 3 argtxts */
        pp->arg[3] = copystring((*--csound->argp)->str);
        pp->arg[2] = copystring((*--csound->argp)->str);
        pp->arg[1] = copystring((*--csound->argp)->str);
        b = argtyp(csound, pp->arg[1]);
        c = argtyp(csound, pp->arg[2]);
        d = argtyp(csound, pp->arg[3]);
        if (UNLIKELY((b != 'B' && b != 'b') ||       /*   chk argtypes, */
            c == 'B' || c == 'b'   || d == 'B' || d == 'b' ||
                     (c == 'a' && d != 'a') || (d == 'a' && c != 'a')))
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
      if (!csound->oparms->expr_opt) {
        /* IV - Jan 08 2003: old code: should work ... */
        /* Should use snprintf for safety */
        switch (outype) {
          case 'a': sprintf(s, "#a%d", csound->acount++); break;
          case 'K':
          case 'k': sprintf(s, "#k%d", csound->kcount++); break;
          case 'B': sprintf(s, "#B%d", csound->Bcount++); break;
          case 'b': sprintf(s, "#b%d", csound->bcount++); break;
          default:  sprintf(s, "#i%d", csound->icount++); break;
        }
      }
      else {
        int ndx = (int) (csound->argp - csound->tokenlist); /* argstack index */
        if (csound->opcode_is_assign == 1       &&
            (int) ndx == 0                      &&
            (int) outype == csound->assign_type &&
            strchr("aki", csound->assign_type) != NULL) {
          /* IV - Jan 08 2003: if the expression is an input arg to the '=' */
          /* opcode, the output type is appropriate, and the argument stack */
          /* is empty, there is no need for a temporary variable, */
          /* instead just use the opcode outarg */
          strncpy(s, csound->assign_outarg, 1024);
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
            cnt += csound->argcnt_offs;     /* IV - Jan 15 2003 */
          switch (outype) {
            case 'a': sprintf(s, "#a%d", cnt); break;
            case 'K': sprintf(s, "#k_%d", csound->kcount++); break;
            case 'k': sprintf(s, "#k%d", cnt); break;
            case 'B': sprintf(s, "#B%d", csound->Bcount++); break;
            case 'b': sprintf(s, "#b%d", csound->bcount++); break;
            default:  sprintf(s, "#i%d", csound->icount++); break;
          }
          /* IV - Jan 08 2003: count max. stack depth in order to allow */
          /* generating different indexes for temporary variables of */
          /* separate expressions on the same line (see also below). */
          /* N.B. argcnt_offs is reset in rdorch.c when a new line is read. */
          if (ndx > argcnt_max) argcnt_max = ndx;
        }
      }
      /* & point argstack there */
      (*csound->argp++)->str = pp->arg[0] = copystring(s);
      csound->revp++;
      pp--;   op = pp->opcod;                     /* prep for nxt pol */
    }
    if (csound->argp - csound->tokenlist == 1) {
      /* IV - Jan 08 2003: do not re-use temporary variables between */
      /* expressions of the same line */
      csound->argcnt_offs += (argcnt_max + 1);
      /* if wrote to output arg of '=' last time, '=' can be omitted */
      if (csound->opcode_is_assign != 0       &&    /* only if optimising */
          (int) outype == csound->assign_type &&
          strchr("aki", csound->assign_type) != NULL) {
        /* replace outarg if necessary */
        if (strcmp(csound->tokenlist[0]->str, csound->assign_outarg)) {
          /* now the last op will use the output of '=' directly */
          /* the pp + 1 is because of the last pp-- */
          csound->tokenlist[0]->str =
                   (pp + 1)->arg[0] = copystring(csound->assign_outarg);
        }
        /* mark as optimised away */
        csound->opcode_is_assign = -1;
      }
      return(polcnt);                           /* finally, return w. polcnt */
    }
    XERROR(Str("term count"))

 error:
    synterr(csound, Str("expression syntax"));  /* or gracefully report error*/
    csound->Message(csound, " %s: %s\n", xprmsg, sorig);
    strcpy(csound->tokenstring, "1");
    return -1;
}

static int nontermin(int c)
{
    switch (c) {
      case '(':
      case ')':
      case '\0':
      case '^':
      case '+':
      case '-':
      case '*':
      case '/':
      case '%':
      case '>':
      case '<':
      case '=':
      case '!':
      case '&':
      case '|':
      case '#':
      case '\254':
      case '~':
      case '?':
      case ':':
        return 0;
      default:
        return 1;
    }
}

static void putokens(CSOUND *csound)        /* for debugging check only */
{
    TOKEN *tp = csound->tokens;

    while (tp->str != NULL) {
      if (tp->str[0] != '\0' && tp->str[1] == '\0') {
        switch (tp->str[0]) {
          case BITSHL:  csound->Message(csound, "<<\t");  break;
          case BITSHR:  csound->Message(csound, ">>\t");  break;
          case BITCLR:  csound->Message(csound, "&\t");   break;
          case BITFLP:  csound->Message(csound, "#\t");   break;
          case BITSET:  csound->Message(csound, "|\t");   break;
          default:      csound->Message(csound, "%s\t", tp->str);
        }
      }
      else
        csound->Message(csound, "%s\t", tp->str);
      tp++;
    }
    csound->Message(csound, "\n");
}

static void putoklist(CSOUND *csound)       /*      ditto           */
{
    TOKEN **tpp = csound->tokenlist;

    while (tpp < csound->endlist) {
      if ((*tpp)->str[0] != '\0' && (*tpp)->str[1] == '\0') {
        switch ((*tpp)->str[0]) {
          case BITSHL:  csound->Message(csound, "<<\t");  break;
          case BITSHR:  csound->Message(csound, ">>\t");  break;
          case BITCLR:  csound->Message(csound, "&\t");   break;
          case BITFLP:  csound->Message(csound, "#\t");   break;
          case BITSET:  csound->Message(csound, "|\t");   break;
          default:      csound->Message(csound, "%s\t", (*tpp)->str);
        }
      }
      else
        csound->Message(csound, "%s\t", (*tpp)->str);
      tpp++;
    }
    csound->Message(csound, "\n");
}

