/*
  pmidi.c:

  Copyright (C) 2004 John ffitch after Barry Vercoe

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

/* Realtime MIDI using Portmidi library */

#include "cs.h"                                       /*    PMIDI.C    */
#include "midiops.h"
#include "oload.h"
#include <portmidi.h>
#include <porttime.h>
#include <errno.h>

extern  int     csoundIsExternalMidiEnabled(void *);
extern  void    csoundExternalMidiDeviceOpen(void *);
extern  void    csoundExternalMidiDeviceClose(void *);
extern  long    csoundExternalMidiRead(void*, unsigned char*, int);

static  PortMidiStream  *midistream = NULL;
static  int             not_started = 1;

static  const   int     datbyts[8] = { 2, 2, 2, 2, 1, 1, 2, 0 };

void OpenMIDIDevice(ENVIRON *csound)
{
    int i;
    int devnum = atoi(O.Midiname);
    int cntdev = Pm_CountDevices();
    PmEvent buffer;
    PmError retval;

    if (not_started) {
      Pm_Initialize();
      Pt_Start(1, NULL, NULL);
    }
    not_started = 0;
    /* list device information */
    printf("**** Pm_CountDevices()=%d\n", cntdev);
    if (cntdev<=devnum) {
      die("Not sufficient MIDI devices\n");
    }
    if ((O.msglevel & 7) == 7) {
      for (i = 0; i < Pm_CountDevices(); i++) {
        const PmDeviceInfo *info = Pm_GetDeviceInfo(i);
        if ((info->input) | (info->output)) {
          printf("%d: %s, %s", i, info->interf, info->name);
          if (info->input) printf(" (input)");
          if (info->output) printf(" (output)");
          printf("\n");
        }
      }
    }
    printf("**** opening MIDI device %d\n", devnum);

    retval = Pm_OpenInput(&midistream,
                          devnum,             /* Device number */
                          NULL,
                          MBUFSIZ,
                          ((long (*)(void *)) Pt_Time),
                          NULL);

    if (retval != 0) {
      printf("PortMIDI Error: %s\n", Pm_GetErrorText(retval));
    }

    Pm_SetFilter(midistream, PM_FILT_ACTIVE | PM_FILT_CLOCK);
    while (Pm_Poll(midistream)) { /* empty the buffer after setting filter */
      Pm_Read(midistream, &buffer, 1);
    }
}

int GetMIDIData(ENVIRON *csound, unsigned char *mbuf, int nbytes)
{
    int     n, retval, st, d1, d2;
    PmEvent mev;
    /*
     * Reads from user-defined MIDI input.
     */
    if (csoundIsExternalMidiEnabled(csound)) {
      n = csoundExternalMidiRead(csound, mbuf, nbytes);
      return n;
    }
    retval = Pm_Poll(midistream);
    if (retval == FALSE)
      return 0;
    if (retval < 0) {
      printf(Str("sensMIDI: retval errno %d\n"), errno);
      return 0;
    }
    n = 0;
    while ((retval = Pm_Read(midistream, &mev, 1L)) > 0) {
      st = (int) Pm_MessageStatus(mev.message);
      d1 = (int) Pm_MessageData1(mev.message);
      d2 = (int) Pm_MessageData2(mev.message);
      /* unknown message or sysex data: ignore */
      if (st < 0x80)
        continue;
      /* ignore most system messages */
      if (st >= 0xF0 &&
          !(st == 0xF8 || st == 0xFA || st == 0xFB || st == 0xFC || st == 0xFF))
        continue;
      nbytes -= (datbyts[(st - 0x80) >> 4] + 1);
      if (nbytes < 0) {
        csoundMessage(csound, Str(" *** buffer overflow in MIDI input\n"));
        break;
      }
      /* channel messages */
      n += (datbyts[(st - 0x80) >> 4] + 1);
      switch (datbyts[(st - 0x80) >> 4]) {
        case 0:
          *mbuf++ = (unsigned char) st;
          break;
        case 1:
          *mbuf++ = (unsigned char) st;
          *mbuf++ = (unsigned char) d1;
          break;
        case 2:
          *mbuf++ = (unsigned char) st;
          *mbuf++ = (unsigned char) d1;
          *mbuf++ = (unsigned char) d2;
          break;
      }
    }
    if (retval < 0)
      csoundMessage(csound, Str(" *** PortMIDI: read error %d\n"), n);
    return n;
}

void CloseMIDIDevice(ENVIRON *csound)
{
    if (midistream != NULL) {
      Pm_Close(midistream);
      midistream = NULL;
    }
}

