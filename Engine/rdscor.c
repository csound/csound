/*
    rdscor.c:

    Copyright (C) 2011 John ffitch (after Barry Vercoe)

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

#include "csoundCore.h"         /*                  RDSCORSTR.C */
#include "corfile.h"
#include "udo.h"

char* csoundGetString(CSOUND *csound, MYFLT p)
{
    int32 n;
    char *ss;
    INSDS* ip = csound->ids->insdshead;
    while (ip->opcod_iobufs != NULL) {
      ip = ((OPCOD_IOBUFS*)ip->opcod_iobufs)->parent_ip;
    }
    ss = ip->strarg;  /* look at this instr's strarg */

#ifdef USE_DOUBLE
    {
      union {
        MYFLT d;
        int32 i[2];
      } ch;
      ch.d = p;
      if (byte_order()==0)
        n = ch.i[1]&0xffff;
      else
        n = ch.i[0]&0xffff;
      //printf("UNION %.8x %.8x\n", ch.i[0], ch.i[1]);
    }
#else
    {
      union {
        MYFLT d;
        int32 j;
      } ch;
      ch.d = p; n = ch.j&0xffff;
      //printf("SUNION %.8x \n", ch.j);
    }
#endif
    while (n-- > 0) {
      ss += strlen(ss)+1;
    }
    //printf(" *** -> %s\n", ss);
    return ss;
}

char* csoundGetString_from_evt(CSOUND *csound, MYFLT p, EVTBLK *evt)
{
    int32 n;
    char *ss = evt->strarg;
#ifdef USE_DOUBLE
    {
      union {
        MYFLT d;
        int32 i[2];
      } ch;
      ch.d = p;
      if (byte_order()==0)
        n = ch.i[1]&0xffff;
      else
        n = ch.i[0]&0xffff;
      //printf("UNION %.8x %.8x\n", ch.i[0], ch.i[1]);
    }
#else
    {
      union {
        MYFLT d;
        int32 j;
      } ch;
      ch.d = p; n = ch.j&0xffff;
      //printf("SUNION %.8x \n", ch.j);
    }
#endif
    while (n-- > 0) {
      ss += strlen(ss)+1;
    }
    //printf(" *** -> %s\n", ss);
    return ss;
}


void set_evt_strarg(CSOUND *csound, EVTBLK *e, int32_t pcnt, const
                    char *str) {
  int32_t scnt = e->scnt;
  size_t strsiz = strlen(str);
  if (e->strarg == NULL) {
    e->strarg = csound->Malloc(csound, strsiz+1);
    memcpy(e->strarg, str, strsiz);
  }
  else {
    strsiz = strlen(e->strarg);
    e->strarg = csound->ReAlloc(csound, e->strarg,
                                strsiz+strlen(str)+1);
    memcpy(e->strarg+strsiz, str, strlen(str));
  }

#ifdef USE_DOUBLE              
  int32_t sel = (byte_order()+1)&1;
  union {
    MYFLT d;
    int32 i[2];
  } ch;
  ch.d = SSTRCOD; ch.i[sel] += scnt++;
  e->p[pcnt] = ch.d;           /* set as string with count */
#else
  union {
    MYFLT d;
    int32 i;
  } ch;
  ch.d = SSTRCOD; ch.i += scnt++;
  e->p[pcnt] = ch.d;           /* set as string with count */
#endif
 e->scnt = scnt;
}



static void dumpline(CSOUND *);

static void flushline(CSOUND *csound)   /* flush scorefile to next newline */
{
    int32_t     c;
    while ((c = corfile_getc(csound->scstr)) != '\0' && c != '\n')
        ;
}

static int32_t scanflt(CSOUND *csound, MYFLT *pfld)
{   /* read a MYFLT from scorefile; return 1 if OK, else 0 */
    int32_t     c;

    while ((c = corfile_getc(csound->scstr)) == ' ' ||
           c == '\t')  /* skip leading white space */
        ;
    if (UNLIKELY(c == ';')) {             /* Comments terminate line */
      flushline(csound);
      return 0;
    }
    if (c == '"') {                             /* if find a quoted string  */
      char *sstrp;
      int32_t n = csound->scnt;
      if ((sstrp = csound->sstrbuf) == NULL)
        sstrp = csound->sstrbuf = csound->Malloc(csound, csound->strsiz=SSTRSIZ);
      while (n--!=0) sstrp += strlen(sstrp)+1;
      n = (int32_t) (sstrp-csound->sstrbuf);
      while ((c = corfile_getc(csound->scstr)) != '"') {
        if (c=='\\') { c = corfile_getc(csound->scstr);
          switch (c) {
          case '"': c = '"'; break;
          case '\'': c = '\''; break;
          case '\\': c = '\\'; break;
          case 'a': c = '\a'; break;
          case 'b': c = '\b'; break;
          case 'f': c = '\f'; break;
          case 'n': c = '\n'; break;
          case 'r': c = '\r'; break;
          case 't': c = '\t'; break;
          case 'v': c = '\v'; break;
          }
        }
        *sstrp++ = c;
        n++;
        if (n > csound->strsiz-10) {
          csound->sstrbuf = csound->ReAlloc(csound, csound->sstrbuf,
                                            csound->strsiz+=SSTRSIZ);
          sstrp = csound->sstrbuf+n;
        }
      }
      *sstrp++ = '\0';
#ifdef USE_DOUBLE
      {
        int32_t sel = (byte_order()+1)&1;
        union {
          MYFLT d;
          int32 i[2];
        } ch;
        ch.d = SSTRCOD;
        //printf("**** %.8x %.8x\n", ch.i[0], ch.i[1]);
        ch.i[sel] += csound->scnt++;
        *pfld = ch.d;           /* set as string with count */
        //printf("***  %.8x %.8x\n", ch.i[0], ch.i[1]);
      }
#else
      {
        union {
          MYFLT d;
          int32 j;
        } ch;
        ch.d = SSTRCOD;
        //printf("****** %.8x\n", ch.j);
        ch.j += csound->scnt++;
        *pfld = ch.d;           /* set as string with count */
      }
#endif
      csound->sstrlen = (int32_t) (sstrp - csound->sstrbuf);  /*    & overall length  */
      //printf("csound->sstrlen = %d\n", csound->sstrlen);
      return(1);
    }
    if (UNLIKELY(!((c>='0' && c<='9') || c=='+' || c=='-' || c=='.'))) {
      corfile_ungetc(csound->scstr);
      csound->Message(csound,
                      Str("ERROR: illegal character %c(%.2x) in scoreline: "),
                      c, c);
      dumpline(csound);
      return(0);
    }
    corfile_ungetc(csound->scstr);
    {
      MYFLT ans = corfile_get_flt(csound->scstr);
      *pfld = ans;
      //printf("%s(%d):%lf %lf\n", __FILE__, __LINE__, ans, *pfld);
    }
    return(1);
}

static void dumpline(CSOUND *csound)    /* print the line while flushing it */
{
    int32_t     c;
    while ((c = corfile_getc(csound->scstr)) != '\0' && c != '\n') {
      csound->Message(csound, "%c", c);
    }
    csound->Message(csound, Str("\n\tremainder of line flushed\n"));
}


int32_t rdscor(CSOUND *csound, EVTBLK *e) /* read next score-line from scorefile */
/*  & maintain section warped status   */
{                                     /*      presumes good format if warped */
  MYFLT   *pp, *plim;
  int32_t c;
  int msize = PMAX;
  e->pinstance = NULL;
  if (csound->scstr == NULL ||
      csound->scstr->body[0] == '\0') {   /* if no concurrent scorefile  */
    e->opcod = 'f';             /*     return an 'f 0 3600'    */
    e->p = csound->Calloc(csound, sizeof(MYFLT)*3);
    e->p[1] = FL(0.0);
    e->p[2] = FL(INF);
    e->p2orig = FL(INF);
    e->pcnt = 2;

    return(1);
  }
    
  e->p = csound->Calloc(csound, sizeof(MYFLT)*(msize+1));
  /* else read the real score */
  while ((c = corfile_getc(csound->scstr)) != '\0') {
    csound->scnt = 0;
    switch (c) {
    case ' ':
    case '\t':
    case '\n':
      continue;               /* skip leading white space */
    case ';':
      flushline(csound);
      continue;
    case 's':
    case 't':
    case 'y':
      csound->warped = 0;
      goto unwarped;
    case 'w':
      csound->warped = 1;     /* w statement is itself unwarped */
    unwarped:
      e->opcod = c;         /*  UNWARPED scorefile:         */
      pp = &e->p[0];
      plim = &e->p[PMAX];             /*    caution, irregular format */  
      while (1) {
        while ((c = corfile_getc(csound->scstr))==' ' ||
               c=='\t'); /* eat whitespace */
        if (c == ';') { flushline(csound); break; } /* comments? skip */
        if (c == '\n' || c == '\0')   break;    /* newline? done  */
        corfile_ungetc(csound->scstr);          /* pfld:  back up */
        if (!scanflt(csound, ++pp))  break;     /*   & read value */
        if (UNLIKELY(pp >= plim)) {
          size_t ofs;
          ofs = pp - e->p;
          msize += PMAX;
          e->p =  (MYFLT*) csound->ReAlloc(csound, e->p,
                                           sizeof(MYFLT)*msize);
          if (UNLIKELY(e->p==NULL)) {
            fprintf(stderr, Str("Out of Memory\n"));
            exit(7);
          }
          pp = e->p + ofs;
        }
      }
      e->p2orig = e->p[2];                 /* now go count the pfields */
      e->p3orig = e->p[3];
      goto setp;
    case 'e':
      e->opcod = c;
      e->pcnt = 0;
      return(1);
    case EOF:                          /* necessary for cscoreGetEvent */
      return(0);
    default:                                /* WARPED scorefile:       */
      if (!csound->warped) goto unwarped;
      e->opcod = c;                                       /* opcod */
      pp = &e->p[0];
      plim = &e->p[PMAX];
      if (corfile_getc(csound->scstr) != '\n' &&
          scanflt(csound, ++pp))         /* p1      */
        if (corfile_getc(csound->scstr) != '\n' &&
            scanflt(csound, &e->p2orig)) /* p2 orig */
          if (corfile_getc(csound->scstr) != '\n' &&
              scanflt(csound, ++pp))     /* p2 warp */
            if (corfile_getc(csound->scstr) != '\n' &&
                scanflt(csound, &e->p3orig)) /* p3  */
              if (corfile_getc(csound->scstr) != '\n' &&
                  scanflt(csound, ++pp)) /* p3 warp */
                while (corfile_getc(csound->scstr) != '\n' &&
                       scanflt(csound, ++pp)) { /* p4....  */
                  if(pp >= plim) {
                    size_t ofs;
                    ofs = pp - e->p;
                    msize += PMAX;
                    e->p = (MYFLT*) csound->ReAlloc(csound, e->p,
                                                     sizeof(MYFLT)*msize);
                    pp = e->p + ofs;
                    if (UNLIKELY(e->p==NULL)) {
                      fprintf(stderr, Str("Out of Memory\n"));
                      exit(7);
                    }
                  }
                }
    setp:
      if (!csound->csoundIsScorePending_ && e->opcod == 'i') {
        /* FIXME: should pause and not mute */
        csound->sstrlen = 0;
        e->opcod = 'f'; e->p[1] = FL(0.0); e->pcnt = 2; e->scnt = 0;
        return 1;
      }
      e->pcnt = (int32_t) (pp - e->p);                   /* count the pfields */
      if (csound->sstrlen) {        /* if string arg present, save it */
        e->strarg = csound->sstrbuf; csound->sstrbuf = NULL;
        e->scnt = csound->scnt;
        csound->sstrlen = 0;
      }
      else { e->strarg = NULL; e->scnt = 0; } /* is this necessary?? */
      return 1;
    }
  }
  corfile_rm(csound, &(csound->scstr));
  return 0;
}
