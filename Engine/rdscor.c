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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
    02111-1307 USA
*/

#include "csoundCore.h"         /*                  RDSCORSTR.C */
#include "corfile.h"

char* get_arg_string(CSOUND *csound, MYFLT p)
{
    int32 n;
    char *ss = csound->ids->insdshead->strarg;  /* look at this instr's strarg */
    union {
      MYFLT d;
      int32 i;
    } ch;
    ch.d = p; n = ch.i&0xffff;
    while (n-- > 0) ss += strlen(ss)+1;
    return ss;
}

static void dumpline(CSOUND *);

static void flushline(CSOUND *csound)   /* flush scorefile to next newline */
{
    int     c;
    while ((c = corfile_getc(csound->scstr)) != '\0' && c != '\n')
        ;
}

static int scanflt(CSOUND *csound, MYFLT *pfld)
{   /* read a MYFLT from scorefile; return 1 if OK, else 0 */
    int     c;

    while ((c = corfile_getc(csound->scstr)) == ' ' ||
           c == '\t')  /* skip leading white space */
        ;
    if (c == ';') {             /* Comments terminate line */
      flushline(csound);
      return 0;
    }
    if (c == '"') {                             /* if find a quoted string  */
      char *sstrp;
      int n = csound->scnt;
      if ((sstrp = csound->sstrbuf) == NULL)
        sstrp = csound->sstrbuf = csound->Malloc(csound, csound->strsiz=SSTRSIZ);
      while (n--!=0) sstrp += strlen(sstrp)+1;
      n = sstrp-csound->sstrbuf;
      while ((c = corfile_getc(csound->scstr)) != '"') {
        //if (c=='\\') c = corfile_getc(csound->scstr);
        *sstrp++ = c;
        n++;
        if (n > csound->strsiz-10) {
          csound->sstrbuf = csound->ReAlloc(csound, csound->sstrbuf,
                                     csound->strsiz+=SSTRSIZ);
          sstrp = csound->sstrbuf+n;
        }
      }
      *sstrp++ = '\0';
      {
        union {
          MYFLT d;
          int32 i;
        } ch;
        ch.d = SSTRCOD; ch.i += csound->scnt++;
        *pfld = ch.d;           /* set as string with count */
      }
      csound->sstrlen = sstrp - csound->sstrbuf;  /*    & overall length  */
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
    int     c;
    while ((c = corfile_getc(csound->scstr)) != '\0' && c != '\n') {
      csound->Message(csound, "%c", c);
    }
    csound->Message(csound, Str("\n\tremainder of line flushed\n"));
}

int rdscor(CSOUND *csound, EVTBLK *e) /* read next score-line from scorefile */
                                      /*  & maintain section warped status   */
{                                     /*      presumes good format if warped */
    MYFLT   *pp, *plim;
    int     c;
    e->pinstance = NULL;

    if (csound->scstr == NULL ||
        csound->scstr->body[0] == '\0') {   /* if no concurrent scorefile  */
      e->opcod = 'f';             /*     return an 'f 0 3600'    */
      e->p[1] = FL(0.0);
      e->p[2] = FL(INF);
      e->p2orig = FL(INF);
      e->pcnt = 2;
      return(1);
    }
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
      unwarped:   e->opcod = c;         /*  UNWARPED scorefile:         */
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
            csound->Message(csound, Str("ERROR: too many pfields: "));
            dumpline(csound);
            break;
          }
        }
        e->p2orig = e->p[2];                 /* now go count the pfields */
        e->p3orig = e->p[3];
        e->c.extra = NULL;
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
        free(e->c.extra);
        e->c.extra = NULL;
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
                         scanflt(csound, ++pp))
                    /* p4....  */
                    if (pp >= plim) {
                      MYFLT *new;
                      MYFLT *q;
                      int c=1;
                      csound->DebugMsg(csound, "Extra p-fields (%d %d %d %d)\n",
                                       (int)e->p[1],(int)e->p[2],
                                       (int)e->p[3],(int)e->p[4]);
                      new = (MYFLT*)realloc(e->c.extra,sizeof(MYFLT)*PMAX);
                      if (new==NULL) {
                        fprintf(stderr, Str("Out of Memory\n"));
                        exit(7);
                      }
                      e->c.extra = new;
                      e->c.extra[0] = PMAX-2;
                      q = e->c.extra;
                      while ((corfile_getc(csound->scstr) != '\n') &&
                             (scanflt(csound, &q[c++]))) {
                        if (c > (int) e->c.extra[0]) {
                          csound->DebugMsg(csound,
                                           "and more extra p-fields [%d](%d)%d\n",
                                           c, (int) e->c.extra[0],
                                           (int)sizeof(MYFLT)*
                                                   ((int)e->c.extra[0]+PMAX));
                          new =
                            (MYFLT *)realloc(e->c.extra,
                                             sizeof(MYFLT)*((int) e->c.extra[0]+
                                                            PMAX));
                          if (new==NULL) {
                            fprintf(stderr, "Out of Mdemory\n");
                            exit(7);
                          }
                          q = e->c.extra = new;
                          e->c.extra[0] = e->c.extra[0]+PMAX-1;
                        }
                      }
                      e->c.extra[0] = c;
                      /* flushline(csound); */
                      goto setp;
                    }
      setp:
        if (!csound->csoundIsScorePending_ && e->opcod == 'i') {
          /* FIXME: should pause and not mute */
          csound->sstrlen = 0;
          e->opcod = 'f'; e->p[1] = FL(0.0); e->pcnt = 2; e->scnt = 0;
          return 1;
        }
        e->pcnt = pp - &e->p[0];                   /* count the pfields */
        if (e->pcnt>=PMAX) e->pcnt += e->c.extra[0]; /* and overflow fields */
        if (csound->sstrlen) {        /* if string arg present, save it */
          e->strarg = csound->Malloc(csound, csound->sstrlen); /* FIXME:       */
          memcpy(e->strarg, csound->sstrbuf, csound->sstrlen); /* leaks memory */
          e->scnt = csound->scnt;
          csound->sstrlen = 0;
        }
        else { e->strarg = NULL; e->scnt = 0; } /* is this necessary?? */
        return 1;
      }
    }
    corfile_rm(&(csound->scstr));
    return 0;
}
