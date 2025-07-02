/*
    scot.c:

    Copyright (C) 1991 Alan deLespinasse

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
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
    02110-1301 USA
*/

/*                                                      SCOT.C       */
/* aldel Jul 91 */

#include "scot.h"

#define Str(x)  (x)
#if defined(HAVE_GCC3)
#  define UNLIKELY(x)   __builtin_expect(!!(x),0)
#else
#  define UNLIKELY(x)   x
#endif

static  char    curline[MAXLINE + 1];   /* current line of infile */
static  int     inx,                    /* column # */
                iny,                    /* line # */
                errcount;               /* errors so far */
static  FILE   *infile, *outfile;
static  char   *infilename;

static  int     naturals[PITCHCLASSES] = { 0, 2, 4, 5, 7, 9, 11 };

static  Macro   *gmac;                  /* global macro list */

/* main externally-visible procedure */

int scot(FILE *inf, FILE *outf, char *fil)
{
    char    s[128];
    Inst   *insttop, *ip;

    initf(inf, outf, fil);
    if (findword(s) || strcmp(s, "orchestra"))
      scotferror(Str("Score must start with orchestra section"));
    readorch(&insttop);
    for (;;) {
      if (findword(s))
        break;
      if (!strcmp(s, "functions"))
        readfunctions();
      else if (!strcmp(s, "score"))
        readscore(insttop);
      else
        scotferror(Str("Expected score or functions section"));
    }
    fputs("e\n", outfile);
    while (insttop) {
      ip = insttop;
      insttop = insttop->next;
      free(ip->name);
      free((char *) ip);
    }
    if (errcount)
      reporterrcount();
    return errcount;
}

static                          /* reads from one $instrument to the next */
void readinstsec(Inst *inst,
                 Nextp **nextlist,
                 Rat *grpmul,
                 Rat *timesig,
                 Rat *curtime,
                 Rat *lastbar,
                 Rat *lastnote,
                 Note **notetop,
                 Note **ln,
                 Tempo **tempop,
                 int *accidentals,
                 int *octaves,
                 int *vertical,
                 int *key, int *barkey, int *transpose, char *endchar)
{
    static Rat durdiv = { 4L, 1L };

    int     c, z, lastpitchclass;
    char    s[128], *sp;
    Rat     ratstack, rattmp;
    Note   *pn, *nn, *pn2 = NULL;
    Strlist *ps;
    Nextp  *nextpp;

#ifdef DEBUG
    printf("Reading instrument section: %s\n", inst->name);
#endif

    pn = (*notetop);
    for (;;) {
      findchar(&c);
      if (strchr(endchar, c))
        break;

#ifdef DEBUG
      printf("Processing char: %c\n", c);
#endif

      switch (c) {
      case 't':
        if (findint(&c)) {
          scoterror(Str("Tempo must be specified"));
          break;
        }
        if ((*tempop)->next) {
          scoterror(Str("Redefinition of tempo"));
          break;
        }
        (*tempop)->next = (Tempo *) malloc(sizeof(Tempo));
        *tempop = (*tempop)->next;
        (*tempop)->next = NULL;
        ratass(&((*tempop)->time), curtime);
        (*tempop)->val = c;
        break;
      case '!':
        efindword(s);
        if ((c = (int) strlen(s)) < 2)
          scoterror(Str("Must specify 2 or more letters of keyword"));
        if (!strncmp(s, "accidentals", c)) {
          if (findonoff(accidentals))
            scoterror(Str("Must be \"on\" or \"off\""));

#ifdef DEBUG
          printf(" accidentals %s\n", accidentals ? "on" : "off");
#endif

        }
        else if (!strncmp(s, "octaves", c)) {
          if (findonoff(octaves))
            scoterror(Str("Must be \"on\" or \"off\""));

#ifdef DEBUG
          printf(" ocatves %s\n", *octaves ? "on" : "off");
#endif

        }
        else if (!strncmp(s, "vertical", c)) {
          if (findonoff(vertical))
            scoterror(Str("Must be \"on\" or \"off\""));

#ifdef DEBUG
          printf(" vertical %s\n", *vertical ? "on" : "off");
#endif

        }
        else if (!strncmp(s, "timesignature", c)) {
          efindword(s);

          if ((sscanf(s, "%lu/%lu", &timesig->num, &timesig->denom) != 2)
              ||
              (&(timesig->denom) == 0) ) {
            scoterror(Str("Invalid time signature"));
            timesig->num = 0;
            timesig->denom = 1;
          }

#ifdef DEBUG
          printf(" time sig=%lu/%lu\n", timesig->num, timesig->denom);
#endif

          ratstack.num = 4;
          ratstack.denom = 1;
          ratmul(timesig, timesig, &ratstack);

#ifdef DEBUG
          printf(" measure length=%f\n", ratval(timesig));
#endif

        }
        else if (!strncmp(s, "key", c)) {
          int     y;

          efindword(s);
          for (z = 0; z < PITCHCLASSES; z++)
            key[z] = 0;
          c = y = 0;
          for (z = 0; s[z] != (char) 0; z++)
            switch ((int) s[z]) {
            case '#':
              c = y + 1;
              y++;
              break;
            case '-':
              c = y - 1;
              y--;
              break;
            default:
              if (!isalpha(s[z]))
                scoterror(Str("Bad key signature"));
              key[letterval((int) s[z])] = c;
              y = 0;
            }
          for (z = 0; z < PITCHCLASSES; z++)
            barkey[z] = key[z];
        }
        else if (!strncmp(s, "transpose", c)) {
          efindword(s);
          *transpose = 0;
          for (z = 0; s[z]; z++) {
            switch (s[z]) {
            case ',':
              (*transpose) -= NOTESPEROCT;
              break;
            case '\'':
              (*transpose) += NOTESPEROCT;
              break;
            case '=':
              (*transpose) = 0;
              break;
            case '#':
              (*transpose)++;
              break;
            case '-':
              (*transpose)--;
              break;
            default:
              (*transpose) += naturals[letterval((int) s[z])];
            }
          }
        }
        else if (!strncmp(s, "next", c)) {
          efindword(s);
          if (sscanf(s, "p%d", &c) != 1) {
            scoterror(Str("Invalid field"));
            efindword(s);
            break;
          }
          efindword(s);
          if (sscanf(s, "p%d", &z) != 1) {
            scoterror(Str("Invalid field"));
            break;
          }
          if (*nextlist == NULL) {
            *nextlist = (Nextp *) malloc(sizeof(Nextp));
            nextpp = (*nextlist);
            nextpp->next = NULL;
          }
          else {
            nextpp = (*nextlist);
            if ((c == nextpp->dst) || (z == nextpp->src))
              scoterror(Str("Nested next-parameter passing"));
            while (nextpp->next) {
              nextpp = nextpp->next;
              if ((c == nextpp->dst) || (z == nextpp->src))
                scoterror(Str("Nested next-parameter passing"));
            }
            nextpp->next = (Nextp *) malloc(sizeof(Nextp));
            nextpp = nextpp->next;
            nextpp->next = NULL;
          }
          nextpp->src = c;
          nextpp->dst = z;
        }
        else
          scoterror(Str("Unrecognised keyword"));
        break;
      case '{':
        findint(&c);
        expectchar(':');
        if (!c) {
          ratstack.num = 2L;
          ratstack.denom = 3L;
        }
        else {
          ratstack.denom = (unsigned long) c;
          findint(&c);
          if (!c) {
            for (z = 1; (unsigned long) z < ratstack.denom; z *= 2);
            z /= 2;
            ratstack.num = (unsigned long) z;
          }
          else
            ratstack.num = (unsigned long) c;
          expectchar(':');
        }
        ratmul(grpmul, grpmul, &ratstack);
        readinstsec(inst, nextlist, grpmul, timesig, curtime,
                    lastbar, lastnote, notetop, ln, tempop, accidentals,
                    octaves, vertical, key, barkey, transpose, ":");
        ratdiv(grpmul, grpmul, &ratstack);
        expectchar(':');
        expectchar('}');
        break;
      case '(':
        ratass(&ratstack, curtime);
        if (pn == (*notetop)) {
          readinstsec(inst, nextlist, grpmul, timesig, curtime,
                      lastbar, lastnote, notetop, ln, tempop, accidentals,
                      octaves, vertical, key, barkey, transpose, ")");
          pn = (*notetop);
        }
        else {
          readinstsec(inst, nextlist, grpmul, timesig, curtime,
                      lastbar, lastnote, &pn2->next, ln, tempop, accidentals,
                      octaves, vertical, key, barkey, transpose, ")");
          pn = pn2->next;
        }
        expectchar(')');
        ratass(lastnote, &ratstack);
        break;
      case '/':
        ratadd(lastbar, lastbar, timesig);
        if ((timesig->num) && (ratcmp(lastbar, curtime))) {
          scoterror(Str("Wrong number of beats in bar"));
          ratass(lastbar, curtime);
        }
        for (z = 0; z < PITCHCLASSES; z++)
          barkey[z] = key[z];
        break;
      case '<':
        if (pn == NULL) {
          scoterror(Str("Syntax error: cannot back up"));
          break;
        }
        if (pn->next == NULL) {
          pn->next = (Note *) malloc(sizeof(Note));
          initnote(pn->next);
          pn->next->instrum = pn->instrum + 0.01;
        }
        pn2 = pn;
        pn = pn->next;
        ratass(curtime, lastnote);
        break;
      default:

#ifdef DEBUG
        printf("Reading note\n");
        printf(" time=%lu/%lu\n", curtime->num, curtime->denom);
        printf(" =%f\n", ratval(curtime));
#endif

        scotungetc();
        nn = (Note *) malloc(sizeof(Note));
        nn->p = NULL;
        nn->written = FALSE;
        if (*notetop == NULL) {
          pn = (*ln) = (*notetop) = (Note *) malloc(sizeof(Note));
          initnote(*notetop);
          (*notetop)->instrum = (double) inst->number + 0.01;
        }
        else if (ratcmp(curtime, lastnote))
          pn = (*notetop);
        nn->instrum = pn->instrum;

#ifdef DEBUG
        printf(" instrument #%f\n", nn->instrum);
#endif

        if (*vertical)
          strlistcopy(&nn->carryp, &(*ln)->carryp);
        else
          strlistcopy(&nn->carryp, &pn->carryp);
        for (nextpp = (*nextlist); nextpp; nextpp = nextpp->next) {
          sp = findparam(nextpp->dst, &nn->carryp);
          if (!strcmp(sp, "."))
            strcpy(sp, NEXTP);
        }
        ratass(&nn->start, curtime);
        if (!findint(&c)) {
          ratstack.num = (unsigned long) c;
          ratstack.denom = 1L;
          ratdiv(&nn->dur, &durdiv, &ratstack);
          ratass(&ratstack, &nn->dur);
          rattmp.num = 1L;
          rattmp.denom = 2L;
          for (;;) {
            findchar(&c);
            if (c != '.')
              break;
            ratmul(&ratstack, &ratstack, &rattmp);
            ratadd(&nn->dur, &nn->dur, &ratstack);
          }
        }
        else {
          if (*vertical)
            ratass(&nn->dur, &((*ln)->lastdur));
          else
            ratass(&nn->dur, &pn->lastdur);
          findchar(&c);
        }
        ratass(&nn->lastdur, &nn->dur);
        ratmul(&nn->dur, &nn->dur, grpmul);

#ifdef DEBUG
        printf(" duration=%f\n", ratval(&nn->dur));
        printf(" c=%c\n", c);
#endif

        if (c == '=') {
          nn->octave = 8;
          lastpitchclass = 0;
        }
        else {
          nn->octave = pn->octave;
          lastpitchclass = pn->pitchclass;
          scotungetc();
        }
        for (;;) {
          findchar(&c);
          if (c == '\'')
            nn->octave++;
          else if (c == ',')
            nn->octave--;
          else
            break;
        }
        if (c == 'r') {
          ratass(lastnote, curtime);
          ratmul(&rattmp, &nn->lastdur, grpmul);
          ratadd(curtime, curtime, &rattmp);
          ratass(&(*ln)->lastdur, &nn->lastdur);
          ratass(&pn->lastdur, &nn->lastdur);
          freenote(nn);
          break;
        }
        else {
          nn->pitchclass = letterval(c);
          if (*octaves) {
            c = nn->pitchclass - lastpitchclass;
            if (c < -(PITCHCLASSES / 2))
              nn->octave++;
            else if (c > PITCHCLASSES / 2)
              nn->octave--;
          }
        }
        nn->accid = 0;
        nn->accmod = FALSE;
        for (;;) {
          findchar(&c);
          if (c == '#') {
            nn->accid++;
            nn->accmod = TRUE;
          }
          else if (c == '-') {
            nn->accid--;
            nn->accmod = TRUE;
          }
          else if (c == 'n') {
            nn->accid = 0;
            nn->accmod = TRUE;
          }
          else
            break;
        }
        if (!nn->accmod)
          nn->accid = barkey[nn->pitchclass];
        else if (*accidentals)
          barkey[nn->pitchclass] = nn->accid;

#ifdef DEBUG
        printf(" transpose=%d\n", *transpose);
        printf(" octave=%d pitchclass=%d accid=%d transpose=%d pitch=%f\n",
               nn->octave, nn->pitchclass, nn->accid, *transpose,
               pitchval(nn->octave, nn->pitchclass, nn->accid, *transpose));
#endif

        if (c == '_') {
          findchar(&c);
          if (c == '_') {
            nn->tie = TRUE;
            nn->slur = 0;
            findchar(&c);
          }
          else {
            nn->slur = 1;
            nn->tie = FALSE;
          }
        }
        else {
          nn->slur = 0;
          nn->tie = FALSE;
        }
        if (pn->slur & 1)
          nn->slur += 2;

#ifdef DEBUG
        printf(" slur=%d tie=%d\n", nn->slur, nn->tie);
#endif

        if (pn->tie) {
          ratadd(&rattmp, &pn->start, &pn->dur);
          if (ratcmp(&rattmp, curtime))
            scoterror(Str("Improper tie"));
          if (((nn->octave != pn->octave) ||
               (nn->pitchclass != pn->pitchclass) ||
               ((nn->accid != pn->accid) && (nn->accmod))) &&
              (pitchval(nn->octave, nn->pitchclass, nn->accid, *transpose) !=
               pitchval(pn->octave, pn->pitchclass, pn->accid, *transpose)))
            scoterror(Str("Tie between different pitches"));
          ratadd(&pn->dur, &pn->dur, &nn->dur);
          ratass(&pn->lastdur, &nn->lastdur);
          pn->slur += nn->slur;
          pn->tie = nn->tie;
          freenote(nn);
          nn = pn;
          if (c == (char) '[')
            scoterror(Str("Warning: params changed on tie"));
        }
        else {
          ps = nn->p = (Strlist *) malloc(sizeof(Strlist));
          for (z = 0; z < 4; z++) {
            ps->next = (Strlist *) malloc(sizeof(Strlist));
            ps = ps->next;
          }
          ps->next = NULL;
        }
        ps = nn->p;
        snprintf(ps->str, 32, "%.02f", nn->instrum);
        ps = ps->next;
        snprintf(ps->str, 32, "%g", ratval(&nn->start));
        ps = ps->next;
        snprintf(ps->str, 32, "%g", ratval(&nn->dur));
        ps = ps->next;
        snprintf(ps->str, 32, "%d", nn->slur);
        ps = ps->next;
        snprintf(ps->str, 32, "%.02f",
                pitchval(nn->octave, nn->pitchclass, nn->accid, *transpose));
        if (c == '[') {
          char   *pars;
          int     pnum;

          pars = readparams(inst);

#ifdef DEBUG
          printf("Params: %s\n", pars);
#endif

          z = 0;
          pnum = 6;
          while (strchr(" \t\r\n", (int) pars[z]))
            z++;
          for (;;) {
            if (pars[z] == (char) ']')
              break;
            c = 0;
            while (!strchr(" \t\r\n:]", (int) pars[z]))
              s[c++] = pars[z++];
            s[c] = (char) 0;

#ifdef DEBUG
            printf("Read: %s\n", s);
#endif

            while (strchr(" \t\r\n", (int) pars[z]))
              z++;
            if (pars[z] == (char) ':') {
              pnum = atoi(s);
              if (pnum < 6)
                scoterror(Str("Parameter number out of range"));
              z++;
              while (strchr(" \t\r\n", (int) pars[z]))
                z++;
              continue;
            }

#ifdef DEBUG
            printf("Param #%d: %s\n", pnum, s);
#endif

            if (s[0] == (char) '\'') {
              addparam(pnum, &s[1], &nn->p);
              addparam(pnum, ".", &nn->carryp);
            }
            else {
              addparam(pnum, s, &nn->p);
              addparam(pnum, s, &nn->carryp);
            }
            pnum++;
          }
          free(pars);
        }
        else
          scotungetc();
        if ((nn != pn) && (!pn->written)) {

#ifdef DEBUG
          printf("  doing nextp stuff:\n");
#endif

          for (nextpp = (*nextlist); nextpp; nextpp = nextpp->next) {

#ifdef DEBUG
            printf("   carrying p%d to p%d?\n", nextpp->src, nextpp->dst);
#endif

            if (!strcmp(findparam(nextpp->dst, &pn->carryp), NEXTP)) {
              sp = findparam(nextpp->dst, &pn->p);
              if (!strcmp(sp, ".")) {
                char   *sp2;

                sp2 = findparam(nextpp->src, &nn->p);
                if (!strcmp(sp2, "."))
                  sp2 = findparam(nextpp->src, &nn->carryp);
                strcpy(sp, sp2);

#ifdef DEBUG
                printf("   Yes.\n");
#endif

              }
            }
          }
          writenote(pn);
        }
        if ((!(*nextlist)) && (!nn->tie))
          writenote(nn);
        if (nn != pn) {
          if (!pn->written)
            scoterror(Str("Lost previous note: not written"));

#ifdef DEBUG
          if (pn->next == nn)
            printf("* pn->next==nn\n");
#endif

          nn->next = pn->next;

#ifdef DEBUG
          if (pn2 == nn)
            printf("* pn2==nn\n");
#endif

          if (pn == *notetop)
            *notetop = nn;
          else
            pn2->next = nn;
          freenote(pn);
          pn = nn;

#ifdef DEBUG
          if (nn->next == nn)
            printf("* Circular list created\n");
#endif

        }

#ifdef DEBUG
        printf(" nn linked into note list\n");
        printf(" curtime=%lu/%lu\n", curtime->num, curtime->denom);
        printf(" nn->dur=%lu/%lu\n", nn->dur.num, nn->dur.denom);
#endif

        *ln = nn;
        ratass(lastnote, curtime);
        ratmul(&rattmp, &nn->lastdur, grpmul);
        ratadd(curtime, curtime, &rattmp);

#ifdef DEBUG
        printf(" curtime=%lu/%lu\n", curtime->num, curtime->denom);
        printf(" Done with note\n");
#endif

      }
    }
    scotungetc();
}

static                          /* puts parameter in plist */
void addparam(int n,            /* number of param to change */
              char *s,          /* parameter */
              Strlist **ptop)
{                               /* top of list */
    char   *ps;

    ps = findparam(n, ptop);
    if (strcmp(s, "."))
      strncpy(ps, s, 31);
}

static                          /* returns pointer to */
char   *findparam(int n,        /* nth parameter */
                  Strlist **ptop)
{                               /* in plist */
    int     z;
    Strlist *p;

    if (!(*ptop)) {
      *ptop = (Strlist *) malloc(sizeof(Strlist));
      (*ptop)->next = NULL;
      strcpy((*ptop)->str, ".");
    }
    p = (*ptop);
    for (z = 1; z < n; z++) {
      if (!p->next) {
        p->next = (Strlist *) malloc(sizeof(Strlist));
        p = p->next;
        p->next = NULL;
        strcpy(p->str, ".");
      }
      else
        p = p->next;
    }
    return p->str;
}

static                          /* reads parameter list and */
char   *readparams(Inst *n)
{                               /* substitutes macros for local macro list */
    char   *s;
    int     z;

    s = (char *) malloc(300);
    z = 0;
    for (;;) {
      if ((s[z] = (char) getccom()) == (char) ']')
        break;
      z++;
    }
    s[z + 1] = (char) 0;
    while (applymacs(&s, n));
    return s;
}

static                          /* substitutes 1 or 0 macros in s */
int applymacs(char **s, Inst *n)
{                               /* returns TRUE if substituted */
    char   *news, *mv;
    int     sz, nz;

#ifdef DEBUG
    printf(" Applying macros to %s\n", *s);
#endif

    news = (char *) malloc(300);
    nz = (-1);
    for (sz = 0; (*s)[sz]; sz++) {
      if (sz >= 300) {
        scoterror(Str("Macro expansion too long -- circular macros?"));
        free(news);
        return FALSE;
      }
      news[sz] = (*s)[sz];
      if (isalpha((int) (*s)[sz])) {
        if (nz == (-1))
          nz = sz;
      }
      else if (nz != (-1)) {
        news[sz] = (char) 0;
        mv = macval(&news[nz], n);
        if (mv) {
          strcpy(&news[nz], mv);
          strcat(news, &(*s)[sz]);
          free(*s);
          *s = news;
          return TRUE;
        }
        nz = (-1);
      }
    }
    free(news);
    return FALSE;
}

static                          /* returns value of macro */
char   *macval(char *s, Inst *n)
{
    Macro  *p;

    for (p = n->lmac; p; p = p->next)
      if (!strcmp(s, p->name))
        return p->text;
    for (p = gmac; p; p = p->next)
      if (!strcmp(s, p->name))
        return p->text;
    return NULL;
}

static void initnote(Note *pn)
{
    pn->next = NULL;
    pn->p = pn->carryp = NULL;
    pn->start.num = 0L;
    pn->start.denom = 1L;
    pn->dur.num = 1L;
    pn->dur.denom = 1L;
    pn->lastdur.num = 1L;
    pn->lastdur.denom = 1L;
    pn->octave = 8;
    pn->pitchclass = 0;
    pn->slur = 0;
    pn->tie = FALSE;
    pn->written = TRUE;
}

static                          /* reads score{} section */
void readscore(Inst *insttop)
{
    char    s[128];
    Inst   *p;
    Rat     grpmul, timesig, curtime, lastbar, lastnote, rattmp;
    Tempo  *tempotop, *tempop;
    Note   *notetop, *pn, *qn, *ln;
    Nextp  *nextlist, *nextpp;
    int     accidentals,
            octaves,
            vertical, key[PITCHCLASSES], barkey[PITCHCLASSES], transpose, z;
    double  maxtime, fcurtime;

#ifdef DEBUG
    printf("Reading score section\n");
#endif

    maxtime = 0.0;
    if (expectchar('{'))
      scotferror(Str("Syntax error: no {"));
    tempotop = (Tempo *) malloc(sizeof(Tempo));
    tempotop->time.num = 0;
    tempotop->time.denom = 1;
    tempotop->val = 60;
    tempotop->next = NULL;
    for (;;) {
      tempop = tempotop;
      efindword(s);
      if (s[0] == '}')
        break;
      if (s[0] != '$')
        scotferror(Str("No instrument specified"));
      p = insttop;
      while ((p != NULL) && (strcmp(&s[1], p->name)))
        p = p->next;
      if (p == NULL)
        scotferror(Str("Instrument not defined"));
      notetop = ln = NULL;
      grpmul.num = 1;
      grpmul.denom = 1;
      timesig.num = 0;
      timesig.denom = 1;
      curtime.num = 0;
      curtime.denom = 1;
      lastbar.num = 0;
      lastbar.denom = 1;
      lastnote.num = 0;
      lastnote.denom = 1;
      accidentals = octaves = vertical = TRUE;
      for (z = 0; z < PITCHCLASSES; z++)
        key[z] = barkey[z] = 0;
      transpose = 0;
      nextlist = NULL;
      readinstsec(p, &nextlist, &grpmul, &timesig, &curtime,
                  &lastbar, &lastnote, &notetop, &ln, &tempop, &accidentals,
                  &octaves, &vertical, key, barkey, &transpose, "}$");
      for (pn = notetop; pn; pn = pn->next) {
        if (!pn->written) {
          char   *ps, *ps2;

          for (nextpp = nextlist; nextpp; nextpp = nextpp->next) {
            ps = findparam(nextpp->dst, &pn->carryp);
            if (!strcmp(ps, NEXTP)) {
              ps2 = findparam(nextpp->src, &pn->p);
              if (!strcmp(ps2, "."))
                ps2 = findparam(nextpp->src, &pn->carryp);
              strcpy(ps, ps2);
            }
          }
          writenote(pn);
        }
        if (pn->tie)
          scoterror(Str("unresolved tie"));
        if (pn->slur & 1)
          scoterror(Str("unresolved slur"));
        ratadd(&rattmp, &pn->start, &pn->dur);
        if (ratcmp(&rattmp, &curtime) > 0)
          ratass(&curtime, &rattmp);

#ifdef DEBUG
        if (pn == pn->next)
          scotferror(Str("Circular note list\n"));
#endif

      }
      while (nextlist) {
        nextpp = nextlist;
        nextlist = nextlist->next;
        free((char *) nextpp);
      }
      pn = notetop;
      while (pn) {
        qn = pn;
        pn = pn->next;
        freenote(qn);
      }
      fcurtime = ratval(&curtime);
      if (fcurtime > maxtime)
        maxtime = fcurtime;
    }
    tempop = tempotop;
    putc('t', outfile);
    for (;;) {
      tempotop = tempop;
      tempop = tempop->next;
      fprintf(outfile, "%g %d", ratval(&tempotop->time), tempotop->val);
      free((char *) tempotop);
      if (!tempop)
        break;
      putc(' ', outfile);
    }
    fprintf(outfile, "\nf0 %g\ns\n", maxtime);
}

static                          /* functions{} section */
void readfunctions(void)
{
    int     c;

#ifdef DEBUG
    printf("Reading function section\n");
#endif

    if (expectchar('{'))
      scotferror(Str("Syntax error: no {"));
    for (;;) {
      if ((c = getccom()) == EOF)
        scotferror(Str("Unexpected end of file"));
      if (c == '}') {
        putc('\n', outfile);
        return;
      }
      putc(c, outfile);
    }
}

static                          /* orchestra{} section */
void readorch(Inst **insttopp)
{
    char    s[128];
    Inst   *p, *q = NULL;

#ifdef DEBUG
    printf("Reading orchestra section\n");
#endif

    gmac = NULL;
    if (expectchar('{'))
      scotferror(Str("Syntax error: no {"));
    p = (*insttopp) = (Inst *) malloc(sizeof(Inst));
    p->lmac = NULL;
    for (;;) {
      efindword(s);
      if (s[0] == (char) '}')
        break;
      if (s[0] == (char) '[') {
        if (p == (*insttopp))
          readmacros(&gmac);
        else
          readmacros(&q->lmac);
        continue;
      }
      p->name = (char *) malloc(strlen(s) + 1);
      strcpy(p->name, s);

#ifdef DEBUG
      printf("Instrument name: %s ", p->name);
#endif

      if (expectchar('='))
        scoterror(Str("Syntax error: no ="));
      if (findint((int *) &p->number))
        scoterror(Str("Syntax error: no number"));

#ifdef DEBUG
      printf(" number: %d\n", p->number);
#endif

      p->next = (Inst *) malloc(sizeof(Inst));
      q = p;
      p = p->next;
      p->lmac = NULL;
    }
    if (p == *insttopp)
      scotferror(Str("No instruments declared"));
    free((char *) p);
    q->next = NULL;
}

static                          /* reads macro list from score section */
void readmacros(Macro **mtop)
{
    Macro  *p, *q;

#ifdef DEBUG
    printf("Reading macro definitions\n");
#endif

    p = (*mtop) = (Macro *) malloc(sizeof(Macro));
    efindword(p->name);
    for (;;) {
      q = p;
      if (p->name[0] == (char) ']')
        break;
      if (expectchar('='))
        scoterror(Str("Expected ="));
      efindword(p->text);

#ifdef DEBUG
      printf(" %s = %s\n", p->name, p->text);
#endif

      p = p->next = (Macro *) malloc(sizeof(Macro));
      efindword(p->name);
    }
    q->next = NULL;
    free((char *) p);
}

static                          /* returns TRUE if ch is NOT */
int expectchar(int ch)
{                               /* the next non-whitespace char */
    int     c;

    do {
      if ((c = getccom()) == EOF)
        return TRUE;
    } while (strchr(" \t\r\n", c));
    return !(c == ch);
}

/* all these find* functions return their found values in
   their passed-by-pointer operands. They only return TRUE
   upon failure. */

static int findchar(int *ip)
{
    int     c;

    do {
      if ((c = getccom()) == EOF)
        scotferror(Str("Unexpected end of file"));
    } while (strchr(" \t\r\n", c));
    *ip = c;
    return FALSE;
}

static int findint(int *ip)
{
    int     c, t;

    t = TRUE;
    *ip = 0;
    do {
      if ((c = getccom()) == EOF)
        scotferror(Str("Unexpected end of file"));
    } while (strchr(" \t\r\n", c));
    while (isdigit(c)) {
      t = FALSE;
      *ip *= 10;
      *ip += (c - '0');
      if ((c = getccom()) == EOF)
        scotferror(Str("Unexpected end of file"));
    }
    scotungetc();
    return t;
}

static int findonoff(int *ip)
{
    char    s[32];

    efindword(s);
    if (!strcmp(s, "on")) {
      *ip = TRUE;
      return FALSE;
    }
    if (!strcmp(s, "off")) {
      *ip = FALSE;
      return FALSE;
    }
    return TRUE;
}

static void efindword(char *s)
{
    if (findword(s))
      scotferror(Str("Unexpected end of file"));
}

static int findword(char *s)
{
    int     c, n;

    n = 0;
    do {
      if ((c = getccom()) == EOF)
        return TRUE;
    } while (strchr(" \t\r\n", c));
    if (c == '\"') {
      while ((c = scotgetc()) != '\"') {
        if (c == EOF)
          return TRUE;
        s[n++] = (char) c;
      }
      s[n] = (char) 0;
      return FALSE;
    }
    if (strchr("{}[]", c)) {
      s[0] = (char) c;
      s[1] = (char) 0;
      return FALSE;
    }
    do {
      s[n++] = (char) c;
      if ((c = getccom()) == EOF)
        return TRUE;
    } while (!strchr(" \t\r\n{}[]=\"", c));
    s[n] = (char) 0;
    scotungetc();
    return FALSE;
}

static                          /* gets a char from file, but */
int getccom(void)
{                               /* ignores comments. */
    int     c;

    c = scotgetc();
    if (c != ';')
      return c;
    while (scotgetc() != '\n');
    return '\n';
}

static int letterval(int c)
{
    switch (c) {
    case 'a':
      return 5;
    case 'b':
      return 6;
    case 'c':
      return 0;
    case 'd':
      return 1;
    case 'e':
      return 2;
    case 'f':
      return 3;
    case 'g':
      return 4;
    default:
      scoterror(Str("Invalid pitch class"));
      return 0;
    }
}

static                          /* returns octave.pitchclass */
double pitchval(int oct, int pit, int acc, int transpose)
{
#ifdef DEBUG
    printf("  Computing pitchval(%d,%d,%d,%d)\n", oct, pit, acc, transpose);
#endif

    pit = naturals[pit] + acc + transpose;

#ifdef DEBUG
    printf("  pit=%d\n", pit);
#endif

    while (pit < 0) {
      pit += NOTESPEROCT;
      oct--;

#ifdef DEBUG
      printf("  oct=%d pit=%d\n", oct, pit);
#endif

    }
    while (pit >= NOTESPEROCT) {
      pit -= NOTESPEROCT;
      oct++;

#ifdef DEBUG
      printf("  oct=%d pit=%d\n", oct, pit);
#endif

    }

#ifdef DEBUG
    printf("  pitchval: %d.%02d\n", oct, pit);
#endif

    return (double) oct + (double) pit * 0.01;
}

static                          /* just writes pfields from n->p, */
void writenote(Note *n)
{                               /* or n->carryp if others are blank "." */
    Strlist *ps, *cps;

#ifdef DEBUG
    printf(" writing: i");
#endif

    n->written = TRUE;
    putc('i', outfile);
    ps = n->p;
    cps = n->carryp;
    for (;;) {
      if (ps && strcmp(ps->str, ".")) {
        fputs(ps->str, outfile);

#ifdef DEBUG
        printf(" %s", ps->str);
#endif

      }
      else if (cps && strcmp(cps->str, ".")) {
        fputs(cps->str, outfile);

#ifdef DEBUG
        printf(" %s(c)", cps->str);
#endif

      }
      else {                    /* if (ps || cps) */

        fputs(" 0", outfile);

#ifdef DEBUG
        printf(" 0(.)");
#endif

      }
      if (ps)
        ps = ps->next;
      if (cps)
        cps = cps->next;
      if (!(ps || cps))
        break;
      putc(' ', outfile);
    }
    putc('\n', outfile);

#ifdef DEBUG
    printf("\n");
#endif

}

static                          /* deallocates a note and */
void freenote(Note *n)
{                               /* its pfield lists */
    freeps(n->p);
    freeps(n->carryp);
    free((char *) n);
}

static                          /* deallocates a pfield list */
void freeps(Strlist *pp)
{
    Strlist *pq;

    while (pp) {
      pq = pp;
      pp = pp->next;
      free((char *) pq);
    }
}

static                          /* copies a pfield list */
void strlistcopy(Strlist **dest, Strlist **source)
{
    Strlist *sp, *dp;

    if ((*source) == NULL) {
      *dest = NULL;
      return;
    }
    sp = (*source);
    dp = (*dest) = (Strlist *) malloc(sizeof(Strlist));
    for (;;) {
      strcpy(dp->str, sp->str);
      sp = sp->next;
      if (sp == NULL)
        break;
      dp = dp->next = (Strlist *) malloc(sizeof(Strlist));
    }
    dp->next = NULL;
}

/* rational number functions */

static double ratval(Rat *r)
{                               /* evaluate r */
    if (!r->denom) {
      scoterror(Str("Division by zero"));
      return (double) 1.0;
    }
    return (double) r->num / (double) r->denom;
}

static void ratreduce(Rat *r)
{                               /* reduce r */
    unsigned long a, b, x;

    if (!r->num) {
      r->denom = 1;
      return;
    }
    if (!r->denom) {
      scoterror(Str("Division by zero"));
      return;
    }
    if (r->num > r->denom) {
      a = r->num;
      b = r->denom;
    }
    else {
      a = r->denom;
      b = r->num;
    }
    while ((x = a % b)) {
      a = b;
      b = x;
    }
    r->num /= b;
    r->denom /= b;
}

static void ratadd(Rat *d, Rat *a1, Rat *a2)
{                               /* d=a1+a2; OK if d=a1=a2 */
    unsigned long lcd;

#ifdef DEBUG
    printf("  ratadd (%lu/%lu)+(%lu/%lu)\n", a1->num, a1->denom, a2->num,
           a2->denom);
#endif

    lcd = (a1->denom) * (a2->denom);
    d->num = (a1->num) * (a2->denom) + (a2->num) * (a1->denom);
    d->denom = lcd;

#ifdef DEBUG
    printf("  unreduced result: %lu/%lu\n", d->num, d->denom);
#endif

    ratreduce(d);

#ifdef DEBUG
    printf("  reduced result: %lu/%lu\n", d->num, d->denom);
#endif

}

static void ratmul(Rat *d, Rat *a1, Rat *a2)
{                               /* d=a1*a2; OK if d=a1=a2 */
    d->num = a1->num * a2->num;
    d->denom = a1->denom * a2->denom;
    ratreduce(d);
}

static void ratdiv(Rat *d, Rat *a1, Rat *a2)
{                               /* d=a1/a2; OK if d=a1=a2 */
    if (!a2->num) {
      scoterror(Str("Division by zero"));
      ratass(d, a1);
      return;
    }
    d->num = a1->num * a2->denom;
    d->denom = a1->denom * a2->num;
    ratreduce(d);
}

static int ratcmp(Rat *a, Rat *b)
{
    unsigned long m, n;

    m = a->num * b->denom;
    n = b->num * a->denom;
    if (m < n)
      return -1;
    if (m == n)
      return 0;
    return 1;
}

static void ratass(Rat *a, Rat *b)
{                               /* sorry about the name... a=b; */
    a->num = b->num;
    a->denom = b->denom;
}

/* i/o and error routines */

static                          /* call once to start reading infile */
void initf(FILE *inf, FILE *outf, char *fil)
{
    infile = inf;
    outfile = outf;
    infilename = fil;
    if (UNLIKELY(NULL == fgets(curline, MAXLINE, infile))) {
      fprintf(stderr, "Read failure\n");
      exit(1);
    }
#ifdef DEBUG
    printf("1: %s", curline);
#endif

    inx = 0;
    iny = 1;
    errcount = 0;
}

static                          /* use like getc(infile) */
int scotgetc(void)
{
    if (inx < 0) {
      inx = 0;
      return '\n';
    }
    if (curline[inx] == (char) 0) {
      if (!fgets(curline, MAXLINE, infile))
        return EOF;
      inx = 0;
      iny++;

#ifdef DEBUG
      printf("%d: %s", iny, curline);
#endif

    }
    inx++;
    return (int) curline[inx - 1];
}

static void scotungetc(void)
{
    inx--;
}

static void scoterror(char *s)
{
    int     z;

    fprintf(stderr, "%s(%d) %s\n", infilename, iny, s);
    fputs(curline, stderr);
    for (z = 1; z < inx; z++)
      putc(' ', stderr);
    fprintf(stderr, "^\n");
    errcount++;
}

static void scotferror(char *s)         /* fatal error */
{
    scoterror(s);
    fprintf(stderr, Str("scot processing terminated\n"));
    reporterrcount();
    exit(1);
}

static void reporterrcount(void)
{
    fprintf(stderr, Str("scot: %d errors.\n"), errcount);
}

