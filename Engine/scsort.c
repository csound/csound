/*
    scsort.c:

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
    Foundation, Inc., 31 Milk Street, #960789, Boston, MA, 02196, USA
*/

#include "csoundCore.h"                                  /*   SCSORT.C  */
#include "corfile.h"
#include "sread.h"
#include "prototyp.h"
#include "namedins.h"
#include <ctype.h>


/* Did the section just read end with a timed e? swritestr() writes it as
   an f 0, with no e line to end the score as a bare e's does, and the lines
   after it, which the manual says are ignored, were sorted and played. */
static int32_t ends_with_timed_e(const CSOUND *csound)
{
    const SRTBLK *bp;
    if (csound->sread.op != 'e')
      return 0;
    for (bp = csound->frstbp; bp != NULL; bp = bp->nxtblk)
      if (bp->text[0] == 'e')
        return bp->pcnt > 0;
    return 0;
}

/* reads,sorts,timewarps each score sect in turn, into sco */
static void sort_sections(CSOUND *csound, CORFIL *scin, CORFIL *sco,
                          int32_t first)
{
    int32_t     n;

    csound->sectcnt = 0;
    sread_initstr(csound, scin);

    while ((n = sread(csound)) > 0) {
      if (csound->frstbp->text[0] == 's') { // ignore empty segment
        // should this free memory?
        //printf("repeated 's'\n");
        continue;
      }
      sort(csound);
      twarp(csound);
      swritestr(csound, sco, first);
      //printf("sorted: >>>%s<<<\n", sco->body);
      if (ends_with_timed_e(csound))
        break;
    }
}

/* The scores sorted before the start after the first one, which
   scsortstr_merge() merges into the score played: merged one by one, each
   would copy the score so far. Slot 0 is the score played's. */
#define LATER_SCORES "::scsortstr::later"
#define ENDLESS_TIME "800000000000.0" /* ~25367 years */
/* The first score when it has no statement: it keeps a performance going */
#define ENDLESS "f0 " ENDLESS_TIME "\ne\n"
/* its f 0 as a merge ends with it, after the last section's statements */
#define ENDLESS_END "f 0 " ENDLESS_TIME " " ENDLESS_TIME "\ne\n"
typedef struct {
    CORFIL  **scores;
    int32_t count, size;
} LATER;

static void keep_later(CSOUND *csound, CORFIL *scin)
{
    LATER *later;
    CORFIL *sorted;

    csound->scoreout = NULL;
    sorted = csound->sread.sorted = corfile_create_w(csound);
    sort_sections(csound, scin, sorted, 1);
    corfile_flush(csound, sorted);
    sfree(csound);
    csound->sread.sorted = NULL;
    later = csound->QueryGlobalVariable(csound, LATER_SCORES);
    if (later == NULL) {
      csound->CreateGlobalVariable(csound, LATER_SCORES, sizeof(LATER));
      later = csound->QueryGlobalVariable(csound, LATER_SCORES);
      later->count = 1;
    }
    if (later->count >= later->size) {
      later->size = 2 * later->count;
      later->scores = csound->ReAlloc(csound, later->scores,
                                      later->size * sizeof(CORFIL *));
    }
    later->scores[later->count++] = sorted;
}

/* Drops the kept scores past the first n slots, and the list at n = 1:
   slot 0 is the score played's, never the list's. */
static void drop_later(CSOUND *csound, int32_t n)
{
    LATER *later = csound->QueryGlobalVariable(csound, LATER_SCORES);
    if (later == NULL) return;
    while (later->count > n)
      corfile_rm(csound, &later->scores[--later->count]);
    if (n == 1) {
      csound->Free(csound, later->scores);
      csound->DestroyGlobalVariable(csound, LATER_SCORES);
    }
}

/* Forgets the score scsortstr() kept last: a compile refused it. */
void scsortstr_forget(CSOUND *csound)
{
    LATER *later = csound->QueryGlobalVariable(csound, LATER_SCORES);
    if (later != NULL && later->count > 1)
      drop_later(csound, later->count - 1);
}

/* The score in csound->scorestr sorted as realtime events, as a copy the
   caller frees: on a started engine, or for a score queued for it. */
char *scsortstr_events(CSOUND *csound, CORFIL *scin)
{
    CORFIL *sco;
    char   *str;

    csound->scoreout = NULL;
    sco = csound->sread.sorted = corfile_create_w(csound);
    sort_sections(csound, scin, sco, 0);
    corfile_flush(csound, sco);
    sfree(csound);
    str = csoundStrdup(csound, sco->body);
    //printf("body >>%s<<\n", str);
    csound->sread.sorted = NULL;
    corfile_rm(csound, &(sco));
    return str;
}

/* called from smain.c or some other main. Before the start, the first score
   sorted is the score to be played, csound->scstr, whose text is returned,
   and a later one is kept for scsortstr_merge(), with NULL returned. On a
   started engine, the score is returned as realtime events. */
char *scsortstr(CSOUND *csound, CORFIL *scin)
{
    CORFIL *sco;
    int32_t i = 0;

    if (csound->engineStatus & CS_STATE_COMP)
      return scsortstr_events(csound, scin);
    if (csound->scstr != NULL) {
      keep_later(csound, scin);
      return NULL;
    }
    csound->scoreout = NULL;
    sco = csound->scstr = corfile_create_w(csound);
    sort_sections(csound, scin, sco, 1);
    //printf("**** body = >>%s<<\n", sco->body);
    while (isspace(sco->body[i])) i++;
    if (sco->body[i] == 'e' && sco->body[i+1] == '\n' && sco->body[i+2] != 'e') {
      corfile_rewind(sco);
      corfile_puts(csound, ENDLESS, sco);
    }
    else corfile_puts(csound, "e\n", sco);
    //printf("body >>%s<<\n", sco->body);
    corfile_flush(csound, sco);
    sfree(csound);
    return sco->body;
}

/* A statement of a sorted score, and what sort() orders it by */
typedef struct {
    const char *text, *end;     /* from its opcode to past its newline */
    cs_float    time, dur;      /* newp2 and newp3 */
    int32_t     insno;
    char        op, preced;
} SORTED_LINE;

/* A sorted score in a merge, at its next statement */
typedef struct {
    const char  *p;             /* NULL past its last section */
    SORTED_LINE line;
} SORTED_SCORE;

/* Past the newline that ends the line at p: one in a quoted string, which
   an escape can put there, is the string's. */
static const char *line_end(const char *p)
{
    int32_t quoted = 0;
    for ( ; *p != '\0'; p++) {
      if (*p == '"') quoted = !quoted;
      else if (*p == '\\' && quoted && p[1] != '\0') p++;
      else if (*p == '\n' && !quoted) return p + 1;
    }
    return p;
}

/* The field at *p, on its line, of *len characters (none at the line's end),
   and *p past it. A quoted field is one. */
static const char *next_field(const char **p, size_t *len)
{
    const char *f = *p, *q;
    while (*f == ' ' || *f == '\t') f++;
    q = f;
    if (*q == '"') {
      for (q++; *q != '\0' && *q != '"'; q++)
        if (*q == '\\' && q[1] != '\0') q++;
      if (*q == '"') q++;
    }
    else while (*q != '\0' && !isspace((unsigned char) *q)) q++;
    *len = (size_t) (q - f);
    *p = q;
    return f;
}

/* Reads what sort() orders the statement in l by: its time, then its kind,
   then for an i its instrument and its duration. Its p2 and p3 are each
   followed by the warped value, which the performance plays by unless it
   plays in beats (-t), by the p2 and p3 as written. */
static void read_line(CSOUND *csound, SORTED_LINE *l)
{
    const char *p = l->text + 1, *field[5];
    size_t len[5];
    int32_t i;
    const int32_t p2 = csound->oparms->Beatmode ? 1 : 2;
    const int32_t p3 = csound->oparms->Beatmode ? 3 : 4;

    for (i = 0; i < 5; i++)
      field[i] = next_field(&p, &len[i]);
    l->time = len[p2] ?
      (cs_float) csoundStrtod((char *) field[p2], NULL) : FL(0.0);
    l->dur = len[p3] ?
      (cs_float) csoundStrtod((char *) field[p3], NULL) : FL(0.0);
    l->insno = 0;
    switch (l->op) {
    case 'i':
    case 'd':
      if (*field[0] == '"') {   /* a name, found as setprv() finds it */
        size_t n = len[0] > 2 ? len[0] - 2 : 0;
        char buff[MAXNAME], *name = n < sizeof(buff) ? buff :
          csound->Malloc(csound, n + 1);
        int32_t sign;
        memcpy(name, field[0] + 1, n);
        name[n] = '\0';
        sign = *name == '-';
        if (!(l->insno = (int16) named_instr_find(csound, name + sign)))
          l->insno = -1;
        if (sign) l->insno = -l->insno;
        if (name != buff)
          csound->Free(csound, name);
      }
      else {
        cs_float p1 = len[0] ?
          (cs_float) csoundStrtod((char *) field[0], NULL) : FL(0.0);
        l->insno = (p1 > -FL(32768.0) && p1 < FL(32768.0)) ?
          (int16) p1 : INT16_MIN;
      }
      l->preced = l->insno < 0 ? 'b' : 'd';
      break;
    case 'f': l->preced = 'c'; break;
    case 'a': l->preced = 'e'; break;
    default:  l->preced = 'a'; break;
    }
}

/* Reads the next statement of s: its op is 's' at its section's end, and
   'e' at its score's. */
static void next_line(CSOUND *csound, SORTED_SCORE *s)
{
    const char *p = s->p;
    if (p != NULL)
      while (*p == ' ' || *p == '\t' || *p == '\n') p++;
    if (p == NULL || *p == '\0') {
      s->line.op = 'e';
      return;
    }
    s->line.text = p;
    s->line.op = *p;
    s->p = s->line.end = line_end(p);
    if (*p != 's' && *p != 'e')
      read_line(csound, &s->line);
}

/* Takes the w lines that begin the section of s at s->p: from *head to
   *end. */
static void section_head(SORTED_SCORE *s, const char **head, const char **end)
{
    const char *p = s->p;
    *head = *end = p;
    if (p == NULL) return;
    for (;;) {
      while (*p == ' ' || *p == '\t' || *p == '\n') p++;
      if (*p != 'w') break;
      *end = s->p = p = line_end(p);
    }
}

static void put_text(CSOUND *csound, const char *p, const char *end,
                     CORFIL *out)
{
    for ( ; p < end; p++) corfile_putc(csound, *p, out);
}

static void put_line(CSOUND *csound, const SORTED_LINE *l, CORFIL *out)
{
    put_text(csound, l->text, l->end, out);
    if (l->end[-1] != '\n') corfile_putc(csound, '\n', out);
}

static int32_t is_event(const SORTED_SCORE *s)
{
    return s->line.op != 's' && s->line.op != 'e';
}

/* Does a, of the score played, go before b, of the one merged into it?
   As sort()'s ordering() would answer, a's line in the score being the
   earlier. */
static int32_t goes_first(const SORTED_LINE *a, const SORTED_LINE *b)
{
    if (a->time != b->time) return a->time < b->time;
    if (a->preced != b->preced) return a->preced < b->preced;
    if (a->op == 'i' && b->op == 'i') {
      if (a->insno != b->insno) return a->insno < b->insno;
      if (a->dur != b->dur) return a->dur < b->dur;
    }
    return 1;
}

/* The merge of the sorted scores a and b, as if they had been sorted
   together. Section n of each is section n of the merge, which has as many
   as the longer: a section ends when both scores' have, as one score's does
   when its last statement has. In a section, the statements of both go by
   time, then as sort() orders those of a time (an f before an i, a lower
   instrument's i before a higher one's: a name, by the number it has now),
   and a's before b's after that. Each score's times are warped by its own
   t, and the section keeps one w line, a's if it has one. An e ends its own
   score's text, a timed one is written as an f 0 that the merge keeps, and
   the merge ends with the last section of either. */
static CORFIL *merge_two(CSOUND *csound, const CORFIL *a, const CORFIL *b)
{
    CORFIL *out = corfile_create_w(csound);
    SORTED_SCORE s[2];

    s[0].p = a->body;
    s[1].p = b->body;
    while (s[0].p != NULL || s[1].p != NULL) {
      const char *head[2], *end[2];
      int32_t i;
      for (i = 0; i < 2; i++)
        section_head(&s[i], &head[i], &end[i]);
      /* a's w line, or b's. A section with no w line is one whose first
         statement was a timed s or e: swritestr() writes that as an f 0
         with p2 and its warped value, which a warped section reads as they
         are, as it reads every statement of a sorted score. */
      if (end[0] != head[0])
        put_text(csound, head[0], end[0], out);
      else if (end[1] != head[1])
        put_text(csound, head[1], end[1], out);
      for (i = 0; i < 2; i++)
        next_line(csound, &s[i]);
      while (is_event(&s[0]) || is_event(&s[1])) {
        i = is_event(&s[0]) &&
          (!is_event(&s[1]) || goes_first(&s[0].line, &s[1].line)) ? 0 : 1;
        put_line(csound, &s[i].line, out);
        next_line(csound, &s[i]);
      }
      for (i = 0; i < 2; i++)
        if (s[i].line.op == 'e') s[i].p = NULL;
      corfile_puts(csound, s[0].p != NULL || s[1].p != NULL ? "s\n" : "e\n",
                   out);
    }
    corfile_flush(csound, out);
    return out;
}

/* Takes the endless first score's f 0 off sco, where it is all of a score
   or the end of a merge, and says if it was there. */
static int32_t strip_endless(CSOUND *csound, CORFIL *sco)
{
    size_t len = strlen(sco->body);
    const size_t tail = sizeof(ENDLESS_END) - 1;

    if (strcmp(sco->body, ENDLESS) == 0)
      len = 0;
    else if (len >= tail && strcmp(sco->body + len - tail, ENDLESS_END) == 0 &&
             (len == tail || sco->body[len - tail - 1] == '\n'))
      len -= tail;
    else
      return 0;
    sco->body[len] = '\0';
    sco->p = (uint32_t) len;
    corfile_puts(csound, "e\n", sco);
    corfile_flush(csound, sco);
    return 1;
}

/* Puts the endless f 0 in place of the e that ends sco */
static void put_endless(CSOUND *csound, CORFIL *sco)
{
    size_t end = strlen(sco->body);
    while (end > 0 && isspace((unsigned char) sco->body[end - 1])) end--;
    if (end > 0 && sco->body[end - 1] == 'e' &&
        (end == 1 || sco->body[end - 2] == '\n'))
      end--;
    sco->body[end] = '\0';
    sco->p = (uint32_t) end;
    corfile_puts(csound, ENDLESS_END, sco);
    corfile_flush(csound, sco);
}

/* Merges the scores sorted before the start after the first one into the
   score to be played, csound->scstr, as if all had been sorted together:
   two at a time, the earlier on the left, until one is left, so that a
   statement is copied once for each doubling of their number. */
void scsortstr_merge(CSOUND *csound)
{
    LATER *later = csound->QueryGlobalVariable(csound, LATER_SCORES);
    CORFIL **s;
    int32_t n, i, endless;
    char *body;

    if (later == NULL) return;
    if (csound->scstr == NULL) {  /* no score to join: csoundScoreSort took it */
      drop_later(csound, 1);
      return;
    }
    s = later->scores;
    s[0] = csound->scstr;
    for (i = n = 1; i < later->count; i++) {  /* one with no statement adds none */
      const char *p = s[i]->body;
      while (isspace((unsigned char) *p)) p++;
      if (*p == 'e' || *p == '\0') corfile_rm(csound, &s[i]);
      else s[n++] = s[i];
    }
    /* an endless first score goes on after the others' last section */
    endless = n > 1 && strip_endless(csound, csound->scstr);
    while (n > 1) {
      for (i = 0; i + 1 < n; i += 2) {
        CORFIL *merged = merge_two(csound, s[i], s[i + 1]);
        if (s[i] != csound->scstr) corfile_rm(csound, &s[i]);
        corfile_rm(csound, &s[i + 1]);
        s[i / 2] = merged;
      }
      if (n & 1) s[n / 2] = s[n - 1];
      n = (n + 1) / 2;
    }
    if (s[0] != csound->scstr) {
      /* in place: the score played is this CORFIL */
      body = csound->scstr->body;
      csound->scstr->body = s[0]->body;
      csound->scstr->len = s[0]->len;
      s[0]->body = body;
      corfile_rm(csound, &s[0]);
      corfile_flush(csound, csound->scstr);
    }
    if (endless)
      put_endless(csound, csound->scstr);
    later->count = 1;     /* each other slot was merged, and freed */
    drop_later(csound, 1);
}
