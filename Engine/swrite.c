/*
    swrite.c:

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

#include "csoundCore.h"                                  /*    SWRITE.C  */
#include <math.h>
#include <stdlib.h>
#include <ctype.h>

static SRTBLK *nxtins(SRTBLK *), *prvins(SRTBLK *);
static char   *pfout(CSOUND *,SRTBLK *, char *, int, int);
static char   *nextp(CSOUND *,SRTBLK *, char *, int, int);
static char   *prevp(CSOUND *,SRTBLK *, char *, int, int);
static char   *ramp(CSOUND *,SRTBLK *, char *, int, int);
static char   *expramp(CSOUND *,SRTBLK *, char *, int, int);
static char   *randramp(CSOUND *,SRTBLK *, char *, int, int);
static char   *pfStr(CSOUND *,char *, int, int);
static char   *fpnum(CSOUND *,char *, int, int);

#define fltout(n) fprintf(csound->scoreout, "%.6f", n)

void swrite(CSOUND *csound)
{
    SRTBLK *bp;
    char   *p, c, isntAfunc;
    int    lincnt, pcnt=0;

    if (UNLIKELY((bp = csound->frstbp) == NULL))
      return;
    lincnt = 0;
    if ((c = bp->text[0]) != 'w'
        && c != 's' && c != 'e') {      /*   if no warp stmnt but real data,  */
      fprintf(csound->scoreout, "w 0 60\n");  /* create warp-format indicator */
      lincnt++;
    }
 nxtlin:
    lincnt++;                           /* now for each line:           */
    p = bp->text;
    c = *p++;
    isntAfunc = 1;
    switch (c) {
    case 'f':
      isntAfunc = 0;
    case 'q':
    case 'i':
    case 'a':
      putc(c, csound->scoreout);
      putc(*p++, csound->scoreout);
      while ((c = *p++) != SP && c != LF)
        putc(c, csound->scoreout);              /* put p1       */
      putc(c, csound->scoreout);
      if (c == LF)
        break;
      fltout(bp->p2val);                        /* put p2val,   */
      putc(SP, csound->scoreout);
      fltout(bp->newp2);                        /*   newp2,     */
      while ((c = *p++) != SP && c != LF)
        ;
      putc(c, csound->scoreout);                /*   and delim  */
      if (c == LF)
        break;
      if (isntAfunc) {
        fltout(bp->p3val);                      /* put p3val,   */
        putc(SP, csound->scoreout);
        fltout(bp->newp3);                      /*   newp3,     */
        while ((c = *p++) != SP && c != LF)
          ;
      }
      else { /*make sure p3s (table length) are ints */
        char temp[256];
        sprintf(temp,"%d ",(int32)bp->p3val);   /* put p3val  */
        fpnum(csound,temp, lincnt, pcnt);
        putc(SP, csound->scoreout);
        sprintf(temp,"%d ",(int32)bp->newp3);   /* put newp3  */
        fpnum(csound,temp, lincnt, pcnt);
        while ((c = *p++) != SP && c != LF)
          ;
      }
      pcnt = 3;
      while (c != LF) {
        pcnt++;
        putc(SP, csound->scoreout);
        p = pfout(csound,bp,p,lincnt,pcnt);     /* now put each pfield  */
        c = *p++;
      }
      putc('\n', csound->scoreout);
      break;
    case 's':
    case 'e':
      if (bp->pcnt > 0)
        fprintf(csound->scoreout, "f 0 %f %f\n", bp->p2val, bp->newp2);
      putc(c, csound->scoreout);
      putc(LF, csound->scoreout);
      break;
    case 'w':
    case 't':
      putc(c, csound->scoreout);
      while ((c = *p++) != LF)        /* put entire line      */
        putc(c, csound->scoreout);
      putc(LF, csound->scoreout);
      break;
    default:
      csound->Message(csound,
                      Str("swrite: unexpected opcode, section %d line %d\n"),
                      csound->sectcnt, lincnt);
      break;
    }
    if ((bp = bp->nxtblk) != NULL)
      goto nxtlin;
}

static char *pfout(CSOUND *csound, SRTBLK *bp, char *p,int lincnt, int pcnt)
{
    switch (*p) {
    case 'n':
      p = nextp(csound, bp,p, lincnt, pcnt);
      break;
    case 'p':
      p = prevp(csound, bp,p, lincnt, pcnt);
      break;
    case '<':
    case '>':
      p = ramp(csound, bp,p, lincnt, pcnt);
      break;
    case '(':
    case ')':
      p = expramp(csound, bp, p, lincnt, pcnt);
      break;
    case '~':
      p = randramp(csound, bp, p, lincnt, pcnt);
      break;
    case '"':
      p = pfStr(csound, p, lincnt, pcnt);
      break;
    default:
      p = fpnum(csound, p, lincnt, pcnt);
      break;
    }
    return(p);
}

static SRTBLK *nxtins(SRTBLK *bp) /* find nxt note with same p1 */
{
    MYFLT p1;

    p1 = bp->p1val;
    while ((bp = bp->nxtblk) != NULL
           && (bp->p1val != p1 || bp->text[0] != 'i'))
      ;
    return(bp);
}

static SRTBLK *prvins(SRTBLK *bp) /* find prv note with same p1 */
{
    MYFLT p1;

    p1 = bp->p1val;
    while ((bp = bp->prvblk) != NULL
           && (bp->p1val != p1 || bp->text[0] != 'i'))
      ;
    return(bp);
}

static char *nextp(CSOUND *csound, SRTBLK *bp, char *p, int lincnt, int pcnt)
{
    char *q;
    int n;

    q = p;
    p++;                                    /* 1st char     */
    if (UNLIKELY(*p++ != 'p'))              /* 2nd char     */
      goto error;
    n = 999;
    if (isdigit(*p))
      n = *p++ - '0';
    if (isdigit(*p))                /* n is np subscript no */
      n = 10*n + (*p++ - '0');
    if (UNLIKELY(*p != SP && *p != LF))
      goto error;
    if (LIKELY((bp = nxtins(bp)) != NULL   /* for nxtins, same p1  */
               && n <= bp->pcnt)) {
      q = bp->text;
      while (n--)
        while (*q++ != SP)          /*   go find the pfield */
          ;
      pfout(csound,bp,q,lincnt,pcnt);      /*   and put it out     */
    }
    else {
    error:
      csound->Message(csound,Str("swrite: output, sect%d line%d p%d makes"
                      " illegal reference to "),
        csound->sectcnt,lincnt,pcnt);
      while (q < p)
        csound->Message(csound,"%c", *q++);
      while (*p != SP && *p != LF)
        csound->Message(csound,"%c", *p++);
      csound->Message(csound,Str("   Zero substituted\n"));
      putc('0', csound->scoreout);
    }
    return(p);
}

static char *prevp(CSOUND *csound, SRTBLK *bp, char *p, int lincnt, int pcnt)
{
    char *q;
    int n;

    q = p;
    p++;                                    /* 1st char     */
    if (UNLIKELY(*p++ != 'p'))              /* 2nd char     */
      goto error;
    n = 999;
    if (isdigit(*p))
      n = *p++ - '0';
    if (isdigit(*p))                /* n is np subscript no */
      n = 10*n + (*p++ - '0');
    if (UNLIKELY(*p != SP && *p != LF))
      goto error;
    if (LIKELY((bp = prvins(bp)) != NULL   /* for prvins, same p1, */
               && n <= bp->pcnt)) {
      q = bp->text;
      while (n--)
        while (*q++ != SP)          /*   go find the pfield */
          ;
      pfout(csound,bp,q,lincnt,pcnt);      /*   and put it out     */
    }
    else {
    error:
      csound->Message(csound,
          Str("swrite: output, sect%d line%d p%d makes illegal reference to "),
          csound->sectcnt,lincnt,pcnt);
      while (q < p)
        csound->Message(csound,"%c", *q++);
      while (*p != SP && *p != LF)
        csound->Message(csound,"%c", *p++);
      csound->Message(csound,Str("   Zero substituted\n"));
      putc('0', csound->scoreout);
    }
    return(p);
}

static char *ramp(CSOUND *csound, SRTBLK *bp, char *p, int lincnt, int pcnt)
  /* NB np's may reference a ramp but ramps must terminate in valid nums */
{
    char    *q;
    char    *psav;
    SRTBLK  *prvbp, *nxtbp;
    MYFLT   pval, qval, rval, p2span;
    extern  MYFLT stof(CSOUND *, char *);
    int     pnum, n;

    psav = ++p;
    if (UNLIKELY(*psav != SP && *psav != LF))
      goto error1;
    pnum = 0;
    q = bp->text;
    while (q < p)
      if (*q++ == SP)
        pnum++;
    prvbp = bp;
 backup:
    if (LIKELY((prvbp = prvins(prvbp)) != NULL)) {
      p = prvbp->text;
      n = pnum;
      while (n--)
        while (*p++ != SP)
          ;
      if (*p == '>' || *p == '<')
        goto backup;
    }
    else goto error2;
    nxtbp = bp;
 forwrd:
    if (LIKELY((nxtbp = nxtins(nxtbp)) != NULL)) {
      q = nxtbp->text;
      n = pnum;
      while (n--)
        while (*q++ != SP)
          ;
      if (*q == '>' || *q == '<')
        goto forwrd;
    }
    else goto error2;
    pval = stof(csound, p);     /* the error msgs generated by stof     */
    qval = stof(csound, q);                         /*   are misleading */
    if (UNLIKELY((p2span = nxtbp->newp2 - prvbp->newp2) <= 0))
      goto error2;
    rval = (qval - pval) * (bp->newp2 - prvbp->newp2) / p2span + pval;
    fltout(rval);
    return(psav);

 error1:
    csound->Message(csound,
        Str("swrite: output, sect%d line%d p%d has illegal ramp symbol\n"),
        csound->sectcnt,lincnt,pcnt);
    goto put0;
 error2:
    csound->Message(csound, Str("swrite: output, sect%d line%d p%d ramp "
                                "has illegal forward or backward ref\n"),
                            csound->sectcnt, lincnt, pcnt);
 put0:
    putc('0', csound->scoreout);
    return(psav);
}

static char *expramp(CSOUND *csound, SRTBLK *bp, char *p, int lincnt, int pcnt)
  /* NB np's may reference a ramp but ramps must terminate in valid nums */
{
    char    *q;
    char    *psav;
    SRTBLK  *prvbp, *nxtbp;
    MYFLT   pval, qval, rval;
    double  p2span;
    extern  MYFLT stof(CSOUND *, char *);
    int     pnum, n;

    psav = ++p;
    if (UNLIKELY(*psav != SP && *psav != LF))
      goto error1;
    pnum = 0;
    q = bp->text;
    while (q < p)
      if (*q++ == SP)
        pnum++;
    prvbp = bp;
 backup:
    if (LIKELY((prvbp = prvins(prvbp)) != NULL)) {
      p = prvbp->text;
      n = pnum;
      while (n--)
        while (*p++ != SP)
          ;
      if (*p == '}' || *p == '{' || *p == '(' || *p == ')')
        goto backup;
    }
    else goto error2;
    nxtbp = bp;
 forwrd:
    if (LIKELY((nxtbp = nxtins(nxtbp)) != NULL)) {
      q = nxtbp->text;
      n = pnum;
      while (n--)
        while (*q++ != SP)
          ;
      if (*q == '}' || *q == '{' || *q == '(' || *q == ')')
        goto forwrd;
    }
    else goto error2;
    pval = stof(csound, p);     /* the error msgs generated by stof     */
    qval = stof(csound, q);                         /*   are misleading */
    p2span = (double)(nxtbp->newp2 - prvbp->newp2);
/*  printf("pval=%f qval=%f span = %f\n", pval, qval, p2span); */
    rval = pval * (MYFLT)pow((double)(qval/pval),
                             (double)(bp->newp2 - prvbp->newp2) / p2span);
/*  printf("rval=%f bp->newp2=%f prvbp->newp2-%f\n",
           rval, bp->newp2, prvbp->newp2); */
    fltout(rval);
    return(psav);

 error1:
    csound->Message(csound,Str("swrite: output, sect%d line%d p%d has illegal"
                   " expramp symbol\n"),
               csound->sectcnt,lincnt,pcnt);
    goto put0;
 error2:
    csound->Message(csound, Str("swrite: output, sect%d line%d p%d expramp "
                                "has illegal forward or backward ref\n"),
                            csound->sectcnt, lincnt, pcnt);
 put0:
    putc('0', csound->scoreout);
    return(psav);
}

static char *randramp(CSOUND *csound,
                      SRTBLK *bp, char *p, int lincnt, int pcnt)
  /* NB np's may reference a ramp but ramps must terminate in valid nums */
{
    char    *q;
    char    *psav;
    SRTBLK  *prvbp, *nxtbp;
    MYFLT   pval, qval, rval;
    extern  MYFLT stof(CSOUND *, char *);
    int     pnum, n;

    psav = ++p;
    if (UNLIKELY(*psav != SP && *psav != LF))
      goto error1;
    pnum = 0;
    q = bp->text;
    while (q < p)
      if (*q++ == SP)
        pnum++;
    prvbp = bp;
 backup:
    if (LIKELY((prvbp = prvins(prvbp)) != NULL)) {
      p = prvbp->text;
      n = pnum;
      while (n--)
        while (*p++ != SP)
          ;
      if (UNLIKELY(*p == '~'))
        goto backup;
    }
    else goto error2;
    nxtbp = bp;
 forwrd:
    if (LIKELY((nxtbp = nxtins(nxtbp)) != NULL)) {
      q = nxtbp->text;
      n = pnum;
      while (n--)
        while (*q++ != SP)
          ;
      if (*q == '~')
        goto forwrd;
    }
    else goto error2;
    pval = stof(csound, p);     /* the error msgs generated by stof     */
    qval = stof(csound, q);                         /*   are misleading */
    rval = (MYFLT) (((double) (csound->Rand31(&(csound->randSeed1)) - 1)
                     / 2147483645.0) * ((double) qval - (double) pval)
                    + (double) pval);
    fltout(rval);
    return(psav);

 error1:
    csound->Message(csound,Str("swrite: output, sect%d line%d p%d has illegal"
                   " expramp symbol\n"),
               csound->sectcnt,lincnt,pcnt);
    goto put0;
 error2:
    csound->Message(csound,Str("swrite: output, sect%d line%d p%d expramp has"
                               " illegal forward or backward ref\n"),
               csound->sectcnt,lincnt,pcnt);
 put0:
    putc('0', csound->scoreout);
    return(psav);
}

static char *pfStr(CSOUND *csound, char *p, int lincnt, int pcnt)
{                             /* moves quoted ascii string to SCOREOUT file */
    char *q = p;              /*   with no internal format chk              */
    putc(*p++, csound->scoreout);
    while (*p != '"')
      putc(*p++, csound->scoreout);
    putc(*p++, csound->scoreout);
    if (UNLIKELY(*p != SP && *p != LF)) {
      csound->Message(csound, Str("swrite: output, sect%d line%d p%d "
                                  "has illegally terminated string   "),
                              csound->sectcnt, lincnt, pcnt);
      while (q < p)
        csound->Message(csound,"%c", *q++);
      while (*p != SP && *p != LF)
        csound->Message(csound,"%c", *p++);
      csound->Message(csound,"\n");
    }
    return(p);
}

static char *fpnum(CSOUND *csound,
                   char *p, int lincnt, int pcnt) /* moves ascii string */
  /* to SCOREOUT file with fpnum format chk */
{
    char *q;
    int dcnt;

    q = p;
    if (*p == '+')
      p++;
    if (*p == '-')
      putc(*p++, csound->scoreout);
    dcnt = 0;
    while (isdigit(*p)) {
      putc(*p++, csound->scoreout);
      dcnt++;
    }
    if (*p == '.')
      putc(*p++, csound->scoreout);
    while (isdigit(*p)) {
      putc(*p++, csound->scoreout);
      dcnt++;
    }
    if (*p == 'E' || *p == 'e') { /* Allow exponential notation */
      putc(*p++, csound->scoreout);
      dcnt++;
      if (*p == '+' || *p == '-') {
        putc(*p++, csound->scoreout);
        dcnt++;
      }
      while (isdigit(*p)) {
        putc(*p++, csound->scoreout);
        dcnt++;
      }
    }
    if (UNLIKELY((*p != SP && *p != LF) || !dcnt)) {
      csound->Message(csound,Str("swrite: output, sect%d line%d p%d has "
                                 "illegal number  "),
                      csound->sectcnt,lincnt,pcnt);
      while (q < p)
        csound->Message(csound,"%c", *q++);
      while (*p != SP && *p != LF)
        csound->Message(csound,"%c", *p++);
      csound->Message(csound,Str("    String truncated\n"));
      if (!dcnt)
        putc('0', csound->scoreout);
    }
    return(p);
}

