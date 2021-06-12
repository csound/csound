/*
    midifile.c:

    Copyright (C) 2005 Istvan Varga

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

#include "csoundCore.h"
#include "midifile.h"
#include <errno.h>

static const char *midiFile_ID = "MThd";
static const char *midiTrack_ID = "MTrk";
/* default tempo in beats per minute */
static const double default_tempo = 120.0;

typedef struct tempoEvent_s {
    unsigned long   kcnt;               /* time (in ticks while reading     */
                                        /*   MIDI file, will be converted   */
                                        /*   to kperiods once file is read) */
    double          tempoVal;           /* tempo value in beats per minute  */
} tempoEvent_t;

typedef struct midiEvent_s {
    unsigned long   kcnt;               /* time (in ticks while reading     */
                                        /*   MIDI file, will be converted   */
                                        /*   to kperiods once file is read) */
#if 0
    unsigned char   *data;              /* pointer to sysex or meta event   */
                                        /*   data (currently not used)      */
#endif
    unsigned char   st;                 /* status byte (0x80-0xFF)          */
    unsigned char   d1;                 /* data byte 1 (0x00-0x7F)          */
    unsigned char   d2;                 /* data byte 2 (0x00-0x7F)          */
} midiEvent_t;

typedef struct midiFile_s {
    /* static file data, not changed at performance */
    double          timeCode;           /* > 0: ticks per beat              */
                                        /* < 0: ticks per second            */
    unsigned long   totalKcnt;          /* total duration of file           */
                                        /*   (in ticks while reading file,  */
                                        /*   converted to kperiods)         */
    int             nEvents;            /* number of events                 */
    int             maxEvents;          /* event array size                 */
    int             nTempo;             /* number of tempo changes          */
    int             maxTempo;           /* tempo change array size          */
    midiEvent_t     *eventList;         /* array of MIDI events             */
    tempoEvent_t    *tempoList;         /* array of tempo changes           */
    /* performance time state variables */
    double          currentTempo;       /* current tempo in BPM             */
    int             eventListIndex;     /* index of next MIDI event in list */
    int             tempoListIndex;     /* index of next tempo change       */
} midiFile_t;

#define MIDIFILE    (csound->midiGlobals->midiFileData)
#define MF(x)       (((midiFile_t*) MIDIFILE)->x)

static int getCh(CSOUND *csound, FILE *f, int *bytesLeft)
{
    int c;

    if (f == NULL)
      return -1;
    c = getc(f);
    if (UNLIKELY(c == EOF)) {
      csound->Message(csound, Str(" *** unexpected end of MIDI file\n"));
      return -1;
    }
    if (bytesLeft != NULL) {
      if (UNLIKELY(--(*bytesLeft) < 0)) {
        csound->Message(csound, Str(" *** unexpected end of MIDI track\n"));
        return -1;
      }
    }
    return (c & 0xFF);
}

static int getVLenData(CSOUND *csound, FILE *f, int *bytesLeft)
{
    int c, n, cnt;

    n = cnt = 0;
    do {
      if (UNLIKELY(++cnt > 4)) {
        csound->Message(csound,
                        Str(" *** invalid dynamic length data in MIDI file\n"));
        return -1;
      }
      c = getCh(csound, f, bytesLeft);
      if (c < 0)
        return -1;
      n = (n << 7) | (c & 0x7F);
    } while (c & 0x80);
    return n;
}

/* return the number of bytes to read for status byte 'c' */
/* return value is 0, 1, 2, or -1 in the case of an unknown */
/* or special message */

static int msgDataBytes(int c)
{
    switch (c & 0xF0) {
      case 0x80:        /* note off */
      case 0x90:        /* note on */
      case 0xA0:        /* polyphonic pressure */
      case 0xB0:        /* control change */
      case 0xE0:        /* pitch bend */
        return 2;
      case 0xC0:        /* program change */
      case 0xD0:        /* channel pressure */
        return 1;
      case 0xF0:
        switch (c) {
          case 0xF2:    /* song position */
            return 2;
          case 0xF1:    /* MTC quarter frame */
          case 0xF3:    /* song select */
            return 1;
          case 0xF0:    /* system exclusive */
          case 0xF7:    /* escape sequence */
          case 0xFF:    /* meta event */
          case 0xF4:    /* unknown */
          case 0xF5:    /* unknown */
          case 0xF9:    /* unknown */
          case 0xFD:    /* unknown */
            return -1;
          default:      /* tune request, real time system messages */
            return 0;
        }
    }
    /* anything else is unknown */
    return -1;
}

static int alloc_event(CSOUND *csound, unsigned long kcnt, unsigned char *data,
                                       int st, int d1, int d2)
{
    midiEvent_t *tmp;
    IGN(data);
    /* expand array if necessary */
    if (MF(nEvents) >= MF(maxEvents)) {
      MF(maxEvents) += (MF(maxEvents) >> 3);
      MF(maxEvents) = (MF(maxEvents) + 64) & (~63);
      tmp = (midiEvent_t*) csound->ReAlloc(csound, MF(eventList),
                                    sizeof(midiEvent_t) * MF(maxEvents));
      MF(eventList) = tmp;
      tmp = &(MF(eventList)[MF(nEvents)]);
      memset(tmp, 0, sizeof(midiEvent_t) * (MF(maxEvents) - MF(nEvents)));
    }
    /* store new event */
    tmp = &(MF(eventList)[MF(nEvents)]);
    MF(nEvents)++;
    tmp->kcnt = kcnt;
 /* tmp->data = data; */        /* not used yet */
    tmp->st = (unsigned char) st;
    tmp->d1 = (unsigned char) d1;
    tmp->d2 = (unsigned char) d2;
    /* done */
    return 0;
}

static int alloc_tempo(CSOUND *csound, unsigned long kcnt, double tempoVal)
{
    tempoEvent_t *tmp;
    /* expand array if necessary */
    if (MF(nTempo) >= MF(maxTempo)) {
      MF(maxTempo) += (MF(maxTempo) >> 3);
      MF(maxTempo) = (MF(maxTempo) + 64) & (~63);
      tmp = (tempoEvent_t*) csound->ReAlloc(csound, MF(tempoList),
                                     sizeof(tempoEvent_t) * MF(maxTempo));
      MF(tempoList) = tmp;
      tmp = &(MF(tempoList)[MF(nTempo)]);
      memset(tmp, 0, sizeof(tempoEvent_t) * (MF(maxTempo) - MF(nTempo)));
    }
    /* store new event */
    tmp = &(MF(tempoList)[MF(nTempo)]);
    MF(nTempo)++;
    tmp->kcnt = kcnt; tmp->tempoVal = tempoVal;
    /* done */
    return 0;
}

static int readEvent(CSOUND *csound, FILE *f, int *tlen,
                     unsigned long tickCnt, int st, int *saved_st);

static int checkRealTimeEvent(CSOUND *csound, FILE *f, int *tlen,
                              unsigned long tickCnt, int st, int *saved_st)
{
    if (st & 0x80) {
      if (UNLIKELY(st < 0xF8 || st > 0xFE)) {
        csound->Message(csound, Str(" *** unexpected event 0x%02X\n"),
                                (unsigned int) st);
        return -1;
      }
      /* handle real time message (return code -2) */
      if (readEvent(csound, f, tlen, tickCnt, st, saved_st) != 0)
        return -1;
      return -2;
    }
    return st;
}

static int readEvent(CSOUND *csound, FILE *f, int *tlen,
                     unsigned long tickCnt, int st, int *saved_st)
{
    int i, c, d, cnt, dataBytes[2];

    cnt = dataBytes[0] = dataBytes[1] = 0;
    if (st < 0x80) {
      /* repeat previous status byte */
      dataBytes[cnt++] = st;
      st = *saved_st;
      if (UNLIKELY(st < 0x80)) {
        csound->Message(csound, Str(" *** invalid MIDI file data\n"));
        return -1;
      }
    }
    c = msgDataBytes(st);
    if (c >= 0) {
      if (c > 0) {
        /* save status byte for repeat */
        *saved_st = st;
      }
      while (cnt < c) {
        /* read data byte(s) */
        d = getCh(csound, f, tlen);
        if (d < 0 || *tlen < 0) return -1;
        d = checkRealTimeEvent(csound, f, tlen, tickCnt, d, saved_st);
        if (d == -2)    /* read real time event: continue with reading data */
          continue;
        if (d < 0) return -1;
        dataBytes[cnt++] = d;
      }
      return alloc_event(csound, tickCnt, NULL, st,
                         dataBytes[0], dataBytes[1]);
    }
    /* message is of unknown or special type */
    if (st == 0xF0) {
      /* system exclusive */
      i = getVLenData(csound, f, tlen);
      if (i < 0 || *tlen < 0) return -1;
      /* read message */
      while (--i >= 0) {
        d = getCh(csound, f, tlen);
        if (d < 0 || *tlen < 0) return -1;
        if (d == 0xF7) {        /* EOX */
          if (LIKELY(!i))
            return 0;           /* should be at end of message */
          csound->Message(csound, Str(" *** unexpected end of system "
                                      "exclusive message\n"));
          return -1;
        }
        d = checkRealTimeEvent(csound, f, tlen, tickCnt, d, saved_st);
        if (d == -2)            /* if read real time event, */
          i++;                  /* continue with reading message bytes */
        else if (UNLIKELY(d < 0))
          return -1;            /* error */
      }
      /* zero length or EOX not found */
      csound->Message(csound, Str(" *** invalid system exclusive "
                                  "message in MIDI file\n"));
      return -1;
    }
    else if (st == 0xF7) {
      /* escape sequence: skip message */
      i = getVLenData(csound, f, tlen);         /* message length */
      if (i < 0 || *tlen < 0) return -1;
      while (--i >= 0) {
        c = getCh(csound, f, tlen);
        if (c < 0 || *tlen < 0) return -1;
      }
      return 0;
    }
    else if (st == 0xFF) {
      /* meta event */
      st = getCh(csound, f, tlen);              /* message type */
      if (st < 0 || *tlen < 0) return -1;
      i = getVLenData(csound, f, tlen);         /* message length */
      if (i < 0 || *tlen < 0) return -1;
      if (i > 0 &&
          ((st >= 1 && st <= 5 && (csound->oparms->msglevel & 7) == 7) ||
           (st == 3 && csound->oparms->msglevel != 0))) {
        /* print non-empty text meta events, depending on message level */
        switch (st) {
          case 0x01: csound->Message(csound, Str("  Message: ")); break;
          case 0x02: csound->Message(csound, Str("  Copyright info: ")); break;
          case 0x03: csound->Message(csound, Str("  Track name: ")); break;
          case 0x04: csound->Message(csound, Str("  Instrument name: ")); break;
          case 0x05: csound->Message(csound, Str("  Song lyric: ")); break;
        }
        while (--i >= 0) {
          c = getCh(csound, f, tlen);
          if (c < 0 || *tlen < 0) return -1;
          csound->Message(csound, "%c", c);
        }
        csound->Message(csound, "\n");
        return 0;
      }
      switch (st) {
        case 0x51:                        /* tempo change */
          d = 0;
          while (--i >= 0) {
            c = getCh(csound, f, tlen);
            if (c < 0 || *tlen < 0) return -1;
            d = (d << 8) | c;
          }
          if (UNLIKELY(d < 1)) {
            csound->Message(csound, Str(" *** invalid tempo\n"));
            return -1;
          }
          return alloc_tempo(csound, tickCnt, (60000000.0 / (double) d));
        case 0x2F:                        /* end of track */
          if (UNLIKELY(i)) {
            csound->Message(csound, Str(" *** invalid end of track event\n"));
            return -1;
          }
          if (UNLIKELY(*tlen > 0)) {
            csound->Message(csound, Str(" *** trailing garbage at end of "
                                        "MIDI track\n"));
            return -1;
          }
          /* update file length info */
          if (tickCnt > MF(totalKcnt))
            MF(totalKcnt) = tickCnt;
          return 0;
        default:                          /* skip any other meta event */
          while (--i >= 0) {
            c = getCh(csound, f, tlen);
            if (c < 0 || *tlen < 0) return -1;
          }
          return 0;
      }
    }
    csound->Message(csound, Str(" *** unknown MIDI message: 0x%02X\n"),
                            (unsigned int) st);
    return -1;
}

static int readTrack(CSOUND *csound, FILE *f)
{
    unsigned int    tickCnt;
    int             i, c, tlen, st, saved_st;

    /* check for track header */
    for (i = 0; i < 4; i++) {
      c = getCh(csound, f, NULL);
      if (c < 0)
        return -1;
      if (UNLIKELY(c != (int) midiTrack_ID[i])) {
        csound->Message(csound, Str(" *** invalid MIDI track header\n"));
        return -1;
      }
    }
    /* read track length */
    tlen = 0;
    for (i = 0; i < 4; i++) {
      c = getCh(csound, f, NULL);
      if (c < 0)
        return -1;
      tlen = (tlen << 8) | c;
    }
    /* read track data */
    tickCnt = 0UL;
    saved_st = -1;
    while (tlen > 0) {
      /* get delta time */
      c = getVLenData(csound, f, &tlen);
      if (c < 0 || tlen < 0)
        return -1;
      tickCnt += (unsigned long) c;
      /* get status byte */
      st = getCh(csound, f, &tlen);
      if (st < 0 || tlen < 0)
        return -1;
      /* process event */
      if (readEvent(csound, f, &tlen, tickCnt, st, &saved_st) != 0)
        return -1;
    }
    /* successfully read track */
    return 0;
}

/**
 * Sorts an array of midiEvent_t structures using merge sort algorithm
 * so that the order of events at the same time is preserved.
 * 'p' is the array to be sorted, and 'cnt' is the number of elements
 * (should be > 1). 'tmp' is temporary space of the same length as 'p'.
 */

static CS_NOINLINE void midiEvent_sort(midiEvent_t *p, midiEvent_t *tmp,
                                       size_t cnt)
{
    size_t  p1, p2, dstp;
    size_t  n = cnt >> 1;

    if (n > (size_t) 1)
      midiEvent_sort(p, tmp, n);
    if (cnt > (size_t) 2)
      midiEvent_sort(&(p[n]), tmp, cnt - n);
    dstp = (size_t) 0;
    p1 = (size_t) 0;
    p2 = n;
    do {
      size_t  srcp;
      if (p2 >= cnt || (p1 < n && p[p1].kcnt <= p[p2].kcnt))
        srcp = p1++;
      else
        srcp = p2++;
      tmp[dstp] = p[srcp];
    } while (++dstp < cnt);
    /* copy result back to original array */
    memcpy(p, tmp, cnt * sizeof(midiEvent_t));
}

/**
 * Sorts an array of tempoEvent_t structures using merge sort algorithm
 * so that the order of events at the same time is preserved.
 * 'p' is the array to be sorted, and 'cnt' is the number of elements
 * (should be > 1). 'tmp' is temporary space of the same length as 'p'.
 */

static CS_NOINLINE void tempoEvent_sort(tempoEvent_t *p, tempoEvent_t *tmp,
                                        size_t cnt)
{
    size_t  p1, p2, dstp;
    size_t  n = cnt >> 1;

    if (n > (size_t) 1)
      tempoEvent_sort(p, tmp, n);
    if (cnt > (size_t) 2)
      tempoEvent_sort(&(p[n]), tmp, cnt - n);
    dstp = (size_t) 0;
    p1 = (size_t) 0;
    p2 = n;
    do {
      size_t  srcp;
      if (p2 >= cnt || (p1 < n && p[p1].kcnt <= p[p2].kcnt))
        srcp = p1++;
      else
        srcp = p2++;
      tmp[dstp] = p[srcp];
    } while (++dstp < cnt);
    /* copy result back to original array */
    memcpy(p, tmp, cnt * sizeof(tempoEvent_t));
}

/* sort event lists by time and convert tick times to Csound k-periods */

static void sortEventLists(CSOUND *csound)
{
    double        timeVal, tempoVal;
    unsigned long prvTicks, curTicks, tickEvent, tickTempo;
    int           i, j;

    /* sort events by time in ascending order */
    if (MF(nEvents) > 1 || MF(nTempo) > 1) {
      void    *tmp;
      size_t  nbytes;
      nbytes = (size_t) MF(nEvents) * sizeof(midiEvent_t);
      if (((size_t) MF(nTempo) * sizeof(tempoEvent_t)) > nbytes)
        nbytes = (size_t) MF(nTempo) * sizeof(tempoEvent_t);
      tmp = csound->Malloc(csound, nbytes);
      if (MF(nEvents) > 1)
        midiEvent_sort(MF(eventList), (midiEvent_t *) tmp,
                       (size_t) MF(nEvents));
      if (MF(nTempo) > 1)
        tempoEvent_sort(MF(tempoList), (tempoEvent_t *) tmp,
                        (size_t) MF(nTempo));
      csound->Free(csound, tmp);
    }
    if (MF(timeCode) > 0.0) {
      /* tick values are in fractions of a beat */
      timeVal = 0.0;
      tempoVal = default_tempo;
      /* prvTicks = */ curTicks = 0UL;
      /* k-periods per tick */
      tempoVal = (double) csound->ekr / (tempoVal * MF(timeCode) / 60.0);
      i = j = 0;
      while (i < MF(nEvents) || j < MF(nTempo)) {
        prvTicks = curTicks;
        tickEvent = tickTempo = 0UL;
        tickEvent--; tickTempo--;   /* will be max value for unsigned long */
        if (i < MF(nEvents)) tickEvent = MF(eventList)[i].kcnt;
        if (j < MF(nTempo)) tickTempo = MF(tempoList)[j].kcnt;
        if (tickEvent < tickTempo) {
          curTicks = tickEvent;
          timeVal += ((double) ((long) (curTicks - prvTicks)) * tempoVal);
          MF(eventList)[i++].kcnt = (unsigned long) (timeVal + 0.5);
        }
        else {
          curTicks = tickTempo;
          timeVal += ((double) ((long) (curTicks - prvTicks)) * tempoVal);
          tempoVal = MF(tempoList)[j].tempoVal;     /* new tempo */
          /* k-periods per tick */
          tempoVal = (double) csound->ekr / (tempoVal * MF(timeCode) / 60.0);
          MF(tempoList)[j++].kcnt = (unsigned long) (timeVal + 0.5);
        }
      }
      /* calculate total file length in k-periods */
      timeVal += ((double) ((long) (MF(totalKcnt) - curTicks)) * tempoVal);
      MF(totalKcnt) = (unsigned long) (timeVal + 0.5);
    }
    else {
      /* simple case: time based tick values */
      tempoVal = -(MF(timeCode));
      /* k-periods per tick */
      tempoVal = (double) csound->ekr / tempoVal;
      i = -1;
      while (++i < MF(nEvents)) {
        curTicks = MF(eventList)[i].kcnt;
        timeVal = (double) curTicks * tempoVal;
        curTicks = (unsigned long) (timeVal + 0.5);
        MF(eventList)[i].kcnt = curTicks;
      }
      i = -1;
      while (++i < MF(nTempo)) {
        curTicks = MF(tempoList)[i].kcnt;
        timeVal = (double) curTicks * tempoVal;
        curTicks = (unsigned long) (timeVal + 0.5);
        MF(tempoList)[i].kcnt = curTicks;
      }
      /* calculate total file length in k-periods */
      MF(totalKcnt) = (unsigned long) ((double) MF(totalKcnt) * tempoVal + 0.5);
    }
}

 /* ------------------------------------------------------------------------ */

/* open MIDI file, read all tracks, and create event list */

int csoundMIDIFileOpen(CSOUND *csound, const char *name)
{
    FILE    *f = NULL;
    void    *fd = NULL;
    char    *m;
    int     i, c, hdrLen, fileFormat, nTracks, timeCode, saved_nEvents;
    int     mute_track;

    if (MIDIFILE != NULL)
      return 0;         /* already opened */
    /* open file */
    if (UNLIKELY(name == NULL || name[0] == '\0'))
      return -1;
    //if (*name==3) name++;       /* Because of ETX added bt readOptions */
    if (strcmp(name, "stdin") == 0)
      f = stdin;
    else {
      fd = csound->FileOpen2(csound, &f, CSFILE_STD, name, "rb",
                             "SFDIR;SSDIR;MFDIR", CSFTYPE_STD_MIDI, 0);
      if (UNLIKELY(fd == NULL)) {
        csound->ErrorMsg(csound, Str(" *** error opening MIDI file '%s': %s"),
                                 name, strerror(errno));
        return -1;
      }
    }
    csound->Message(csound, Str("Reading MIDI file '%s'...\n"), name);
    /* check header */
    for (i = 0; i < 4; i++) {
      c = getCh(csound, f, NULL);
      if (UNLIKELY(c < 0)) goto err_return;
      if (UNLIKELY(c != (int) midiFile_ID[i])) {
        csound->Message(csound, Str(" *** invalid MIDI file header\n"));
        goto err_return;
      }
    }
    /* header length: must be 6 bytes */
    hdrLen = 0;
    for (i = 0; i < 4; i++) {
      c = getCh(csound, f, NULL);
      if (UNLIKELY(c < 0)) goto err_return;
      hdrLen = (hdrLen << 8) | c;
    }
    if (UNLIKELY(hdrLen != 6)) {
      csound->Message(csound, Str(" *** invalid MIDI file header\n"));
      goto err_return;
    }
    /* file format (only 0 and 1 are supported) */
    fileFormat = 0;
    for (i = 0; i < 2; i++) {
      c = getCh(csound, f, NULL);
      if (UNLIKELY(c < 0)) goto err_return;
      fileFormat = (fileFormat << 8) | c;
    }
    if (UNLIKELY(fileFormat != 0 && fileFormat != 1)) {
      csound->Message(csound,
                      Str(" *** MIDI file format %d is not supported\n"),
                      fileFormat);
      goto err_return;
    }
    /* number of tracks */
    nTracks = 0;
    for (i = 0; i < 2; i++) {
      c = getCh(csound, f, NULL);
      if (UNLIKELY(c < 0)) goto err_return;
      nTracks = (nTracks << 8) | c;
    }
    if (UNLIKELY(nTracks < 1)) {
      csound->Message(csound, Str(" *** invalid number of tracks\n"));
      goto err_return;
    }
    if (UNLIKELY(nTracks > 1 && !fileFormat)) {
      csound->Message(csound, Str("WARNING: format 0 MIDI file with "
                                  "multiple tracks\n"));
    }
    /* time code */
    timeCode = 0;
    for (i = 0; i < 2; i++) {
      c = getCh(csound, f, NULL);
      if (UNLIKELY(c < 0)) goto err_return;
      timeCode = (timeCode << 8) | c;
    }
    /* allocate structure */
    MIDIFILE = (void*) csound->Calloc(csound, sizeof(midiFile_t));
    /* calculate ticks per second or beat based on time code */
    if (UNLIKELY(timeCode < 1 || (timeCode >= 0x8000 && (timeCode & 0xFF) == 0))) {
      csound->Message(csound, Str(" *** invalid time code: %d\n"), timeCode);
      goto err_return;
    }
    if (timeCode < 0x8000)
      MF(timeCode) = (double) timeCode;
    else {
      switch (timeCode & 0xFF00) {
        case 0xE800:
        case 0xE700:
        case 0xE200:
          MF(timeCode) = (double) ((timeCode >> 8) - 256);
          break;
        case 0xE300:
          MF(timeCode) = -29.97;
          break;
        default:
          csound->Message(csound,
                          Str(" *** invalid time code: %d\n"), timeCode);
          goto err_return;
      }
      MF(timeCode) *= (double) (timeCode & 0xFF);
    }
    /* initialise structure data */
    MF(totalKcnt) = csound->global_kcounter;
    MF(nEvents) = 0; MF(maxEvents) = 0;
    MF(nTempo) = 0; MF(maxTempo) = 0;
    MF(eventList) = (midiEvent_t*) NULL;
    MF(tempoList) = (tempoEvent_t*) NULL;
    MF(currentTempo) = default_tempo;
    MF(eventListIndex) = 0;
    MF(tempoListIndex) = 0;
    /* read all tracks */
    m = &(csound->midiGlobals->muteTrackList[0]);
    for (i = 0; i < nTracks; i++) {
      saved_nEvents = MF(nEvents);
      mute_track = 0;
      if (*m != '\0') {             /* is this track muted ? */
        if (*m == '1')
          mute_track = 1;
        else if (UNLIKELY(*m != '0')) {
          csound->Message(csound, Str(" *** invalid mute track list format\n"));
          goto err_return;
        }
        m++;
      }
      if (!mute_track)
        csound->Message(csound, Str(" Track %2d\n"), i);
      else
        csound->Message(csound, Str(" Track %2d is muted\n"), i);
      if (readTrack(csound, f) != 0)
        goto err_return;
      if (mute_track)                   /* if track is muted, discard any */
        MF(nEvents) = saved_nEvents;    /* non-tempo events read */
    }
    if (fd != NULL)
      csound->FileClose(csound, fd);
    /* prepare event and tempo list for reading */
    sortEventLists(csound);
    /* successfully read MIDI file */
    csound->Message(csound, Str("done.\n"));
    return 0;

    /* in case of error: clean up and report error */
 err_return:
    if (fd != NULL)
      csound->FileClose(csound, fd);
    csoundMIDIFileClose(csound);
    return -1;
}

/* read MIDI file event data at performace time */

int csoundMIDIFileRead(CSOUND *csound, unsigned char *buf, int nBytes)
{
    midiFile_t  *mf;
    int         i, j, n, nRead;

    mf = (midiFile_t*) MIDIFILE;
    if (mf == NULL)
      return 0;
    i = mf->eventListIndex;
    j = mf->tempoListIndex;
    if (i >= mf->nEvents && j >= mf->nTempo) {
      /* there are no more events, */
      if ((unsigned long) csound->global_kcounter >= mf->totalKcnt &&
          !(csound->MTrkend)) {
        /* and end of file is reached: */
        csound->Message(csound, Str("end of midi track in '%s'\n"),
                                csound->oparms->FMidiname);
        csound->Message(csound, Str("%d forced decays, %d extra noteoffs\n"),
                                csound->Mforcdecs, csound->Mxtroffs);
        csound->MTrkend = 1;
        csoundMIDIFileClose(csound);
        csound->oparms->FMidiin = 0;
        if (csound->oparms->ringbell && !(csound->oparms->termifend))
          csound->Message(csound, "\a");
      }
      return 0;
    }
    /* otherwise read any events with time less than or equal to */
    /* current orchestra time */
    while (j < mf->nTempo &&
           (unsigned long) csound->global_kcounter >= mf->tempoList[j].kcnt) {
      /* tempo change */
      mf->currentTempo = mf->tempoList[j++].tempoVal;
    }
    mf->tempoListIndex = j;
    nRead = 0;
    while (i < mf->nEvents &&
           (unsigned long) csound->global_kcounter >= mf->eventList[i].kcnt) {
      n = msgDataBytes((int) mf->eventList[i].st) + 1;
      if (n < 1) {
        i++; continue;        /* unknown or system event: skip */
      }
      nBytes -= n;
      if (UNLIKELY(nBytes < 0)) {
        csound->Message(csound, Str(" *** buffer overflow while reading "
                                    "MIDI file events\n"));
        break;      /* return with whatever has been read so far */
      }
      nRead += n;
      *buf++ = mf->eventList[i].st;
      if (n > 1) *buf++ = mf->eventList[i].d1;
      if (n > 2) *buf++ = mf->eventList[i].d2;
      i++;
    }
    mf->eventListIndex = i;
    /* return the number of bytes read */
    return nRead;
}

/* destroy MIDI file event list */

int csoundMIDIFileClose(CSOUND *csound)
{
    /* nothing to do: memRESET() will free any allocated memory */
    MIDIFILE = (void*) NULL;
    return 0;
}

/* midirecv.c, resets MIDI controllers on a channel */
extern  void    midi_ctl_reset(CSOUND *csound, int16 chan);

/* called by csoundRewindScore() to reset performance to time zero */

void midifile_rewind_score(CSOUND *csound)
{
    int i;
    OPARMS *O = csound->oparms;

    if (MIDIFILE != NULL) {
      /* reset event index and tempo */
      MF(currentTempo) = default_tempo;
      MF(eventListIndex) = 0;
      MF(tempoListIndex) = 0;
      csound->MTrkend = csound->Mxtroffs = csound->Mforcdecs = 0;
      /* reset controllers on all channels */
      for (i = 0; i < MAXCHAN; i++)
        midi_ctl_reset(csound, (int16) i);
    } else if (LIKELY(O->FMidiname != NULL)) {
      csound->MTrkend = 0;
      if (UNLIKELY(csoundMIDIFileOpen(csound, O->FMidiname) != 0))
        csound->Die(csound, Str("Failed to load MIDI file."));
      O->FMidiin = 1;
    }
    else csound->Warning(csound, Str("Cannot rewind MIDI score\n"));
}

 /* ------------------------------------------------------------------------ */

/* miditempo opcode: returns the current tempo of MIDI file or score */

int midiTempoOpcode(CSOUND *csound, MIDITEMPO *p)
{
    if (MIDIFILE == NULL)
      *(p->kResult) = FL(60.0) *csound->esr / (MYFLT)(csound->ibeatTime);
    else
      *(p->kResult) = (MYFLT) MF(currentTempo);
    return OK;
}

int midiFileStatus(CSOUND *csound, MIDITEMPO *p){
  *p->kResult = csound->oparms->FMidiin;
  return OK;
}
