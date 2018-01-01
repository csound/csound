/*
    midisend.c:

    Copyright (C) 1997 Dave Philips
              (C) 2005 Istvan Varga

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

#include "csoundCore.h"                                 /*    MIDISEND.C    */
#include "midioops.h"

typedef struct midiOutFile_s {
    FILE            *f;
    void            *fd;
    unsigned int    prv_tick;
    size_t          nBytes;
    unsigned char   prv_status;
} midiOutFile_t;

static const unsigned char midiMsgBytes[32] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 2, 2, 3, 3, 0, 1
};

/* header for type 0 (1 track) MIDI file with 1/3 ms time resolution */
static const unsigned char midiOutFile_header[25] = {
    0x4D, 0x54, 0x68, 0x64,     /* "MThd"                       */
    0x00, 0x00, 0x00, 0x06,     /* header length                */
    0x00, 0x00,                 /* file type                    */
    0x00, 0x01,                 /* number of tracks             */
    0x19, 0x78,                 /* tick time (1/25 sec / 120),  */
      /*VL this was 0xE7, which was wrong, changed to 0x19 0x78 */
    0x4D, 0x54, 0x72, 0x6B,     /* "MTrk"                       */
    0x00, 0x00, 0x00, 0x00,     /* track length (updated later) */
    /* -------------------------------------------------------- */
    0xFF, 0x2F, 0x00            /* end of track                 */
};

/* write a single event to MIDI out file */

static CS_NOINLINE void
    csoundWriteMidiOutFile(CSOUND *csound, const unsigned char *evt, int nbytes)
{
    unsigned char   buf[8];
    double          s;
    midiOutFile_t   *p = (midiOutFile_t *) csound->midiGlobals->midiOutFileData;
    unsigned int   t, prv;
    int             ndx = 0;

    if (nbytes < 2)
      return;
    s = csound->icurTime/csound->esr;
    /* this check (for perf time run?) used the global pds, which has now
       been removed. My impression is that it is sufficient to check
       for csound->ids, but this might need attention if MIDI file output
       has problems
    */
    if (csound->ids == NULL)
      s -= csound->ksmps/csound->esr;
    s *=  13040.;  /* VL NOV 11: this was 3000.0, which was wrong;
                      13040.0 was arrived at by experimentation */
#ifdef HAVE_C99
    t = (unsigned int) lrint(s);
#else
    t = (unsigned int) ((int) (s + 0.5));
#endif
    t = ((int) t >= 0L ? t : 0UL);
    prv = p->prv_tick;
    p->prv_tick = t;
    t -= prv;
    if (t > 0x0000007FUL) {
      if (t > 0x00003FFFUL) {
        if (t > 0x001FFFFFUL) {
          buf[ndx++] = ((unsigned char) (t >> 21) & (unsigned char) 0x7F)
                       | (unsigned char) 0x80;
        }
        buf[ndx++] = ((unsigned char) (t >> 14) & (unsigned char) 0x7F)
                     | (unsigned char) 0x80;
      }
      buf[ndx++] = ((unsigned char) (t >> 7) & (unsigned char) 0x7F)
                   | (unsigned char) 0x80;
    }
    buf[ndx++] = (unsigned char) t & (unsigned char) 0x7F;
    {
      unsigned char st = *evt;
      if (st != p->prv_status) {
        buf[ndx++] = st;
        p->prv_status = st;
      }
    }
    buf[ndx++] = evt[1];
    if (nbytes > 2)
      buf[ndx++] = evt[2];
    p->nBytes += (size_t) ndx;
    fwrite(&(buf[0]), (size_t) 1, (size_t) ndx, p->f);
}

void send_midi_message(CSOUND *csound, int status, int data1, int data2)
{
    MGLOBAL       *p = csound->midiGlobals;
    unsigned char buf[4];
    unsigned char nbytes;

    buf[0] = (unsigned char) status;
    nbytes = midiMsgBytes[(unsigned char) status >> 3];
    buf[1] = (unsigned char) data1;
    buf[2] = (unsigned char) data2;
    if (!nbytes)
      return;
    if (csound->oparms_.Midioutname != NULL)
      p->MidiWriteCallback(csound, p->midiOutUserData, &(buf[0]), (int) nbytes);
    if (p->midiOutFileData != NULL)
      csoundWriteMidiOutFile(csound, &(buf[0]), (int) nbytes);
}

void note_on(CSOUND *csound, int chan, int num, int vel)
{
    send_midi_message(csound, (chan & 0x0F) | MD_NOTEON, num, vel);
}

void note_off(CSOUND *csound, int chan, int num, int vel)
{
    send_midi_message(csound, (chan & 0x0F) | MD_NOTEOFF, num, vel);
}

void control_change(CSOUND *csound, int chan, int num, int value)
{
    send_midi_message(csound, (chan & 0x0F) | MD_CNTRLCHG, num, value);
}

void after_touch(CSOUND *csound, int chan, int value)
{
    send_midi_message(csound, (chan & 0x0F) | MD_CHANPRESS, value, 0);
}

void program_change(CSOUND *csound, int chan, int num)
{
    send_midi_message(csound, (chan & 0x0F) | MD_PGMCHG, num, 0);
}

void pitch_bend(CSOUND *csound, int chan, int lsb, int msb)
{
    send_midi_message(csound, (chan & 0x0F) | MD_PTCHBENDCHG, lsb, msb);
}

void poly_after_touch(CSOUND *csound, int chan, int note_num, int value)
{
    send_midi_message(csound, (chan & 0x0F) | MD_POLYAFTER, note_num, value);
}

void openMIDIout(CSOUND *csound)
{
    MGLOBAL       *p = csound->midiGlobals;
    midiOutFile_t *fp;
    OPARMS        *O = &(csound->oparms_);
    int           retval;

    /* open MIDI out device */
    if (O->Midioutname != NULL && !p->MIDIoutDONE) {
      if (UNLIKELY(p->MidiOutOpenCallback == NULL))
        csoundDie(csound, Str(" *** no callback for opening MIDI output"));
      if (UNLIKELY(p->MidiWriteCallback == NULL))
        csoundDie(csound, Str(" *** no callback for writing MIDI data"));
      p->MIDIoutDONE = 1;
      retval = p->MidiOutOpenCallback(csound, &(p->midiOutUserData),
                                              O->Midioutname);
      if (UNLIKELY(retval != 0)) {
        csoundDie(csound,
                  Str(" *** error opening MIDI out device: %d (%s)"),
                  retval, csoundExternalMidiErrorString(csound, retval));
      }
    }
    /* open MIDI out file */
    if (O->FMidioutname == NULL || p->midiOutFileData != NULL)
      return;
    fp = (midiOutFile_t *) csound->Calloc(csound, sizeof(midiOutFile_t));
    fp->fd = csound->FileOpen2(csound, &(fp->f), CSFILE_STD, O->FMidioutname,
                                "wb", NULL,  CSFTYPE_STD_MIDI, 0);
    if (UNLIKELY(fp->fd == NULL)) {
      csoundDie(csound, Str(" *** error opening MIDI out file '%s'"),
                        O->FMidioutname);
    }
    p->midiOutFileData = (void *) fp;
    /* write header */
    if (UNLIKELY(fwrite(&(midiOutFile_header[0]),
                        (size_t)1, (size_t)22, fp->f) != 22)) {
      csound->Die(csound, Str("Short write in MIDI\n"));
    }
}

void csoundCloseMidiOutFile(CSOUND *csound)
{
    midiOutFile_t   *p = (midiOutFile_t *) csound->midiGlobals->midiOutFileData;

    /* write end of track meta-event */
    csoundWriteMidiOutFile(csound, &(midiOutFile_header[22]), 3);
    /* update header for track length */
    if (fseek(p->f, 18L, SEEK_SET)<0)
      csound->Message(csound, Str("error closing MIDI output file\n"));
    fputc((int)(p->nBytes >> 24) & 0xFF, p->f);
    fputc((int)(p->nBytes >> 16) & 0xFF, p->f);
    fputc((int)(p->nBytes >> 8) & 0xFF, p->f);
    fputc((int)(p->nBytes) & 0xFF, p->f);
    /* close file and clean up */
    csound->midiGlobals->midiOutFileData = NULL;
    csound->FileClose(csound, p->fd);
    csound->Free(csound, p);
}
