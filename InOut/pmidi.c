/*
  pmidi.c:

  Copyright (C) 2004 John ffitch after Barry Vercoe
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
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
  02111-1307 USA
*/

/* Realtime MIDI using Portmidi library */

#include "csoundCore.h"                         /*    PMIDI.C    */
#include "csound.h"
#include "midiops.h"
#include "oload.h"
#include <portmidi.h>
#include <porttime.h>
#include <errno.h>

#ifdef Str
#undef Str
#endif
#define Str(x) (((ENVIRON*) csound)->LocalizeString(x))

static  int             not_started = 1;
static  const   int     datbyts[8] = { 2, 2, 2, 2, 1, 1, 2, 0 };

static int portMidi_getDeviceCount(int output)
{
    int           i, cnt1, cnt2;
    PmDeviceInfo  *info;

    cnt1 = (int) Pm_CountDevices();
    if (cnt1 < 1)
      return cnt1;      /* no devices */
    cnt2 = 0;
    for (i = 0; i < cnt1; i++) {
      info = (PmDeviceInfo*) Pm_GetDeviceInfo((PmDeviceID) i);
      if (output && info->output)
        cnt2++;
      else if (!output && info->input)
        cnt2++;
    }
    return cnt2;
}

static int portMidi_getRealDeviceID(int dev, int output)
{
    int           i, j, cnt;
    PmDeviceInfo  *info;

    cnt = (int) Pm_CountDevices();
    i = j = -1;
    while (++i < cnt) {
      info = (PmDeviceInfo*) Pm_GetDeviceInfo((PmDeviceID) i);
      if ((output && !(info->output)) || (!output && !(info->input)))
        continue;
      if (++j == dev)
        return i;
    }
    return -1;
}

static int portMidi_getPackedDeviceID(int dev, int output)
{
    int           i, j, cnt;
    PmDeviceInfo  *info;

    cnt = (int) Pm_CountDevices();
    i = j = -1;
    while (++i < cnt) {
      info = (PmDeviceInfo*) Pm_GetDeviceInfo((PmDeviceID) i);
      if ((output && info->output) || (!output && info->input))
        j++;
      if (i == dev)
        return j;
    }
    return -1;
}

static PmDeviceInfo *portMidi_getDeviceInfo(int dev, int output)
{
    int i;

    i = portMidi_getRealDeviceID(dev, output);
    if (i < 0)
      return NULL;
    return ((PmDeviceInfo*) Pm_GetDeviceInfo((PmDeviceID) i));
}

static void portMidi_listDevices(ENVIRON *csound, int output)
{
    int           i, cnt;
    PmDeviceInfo  *info;

    cnt = portMidi_getDeviceCount(output);
    if (cnt < 1)
      return;
    if (output)
      csound->Message(csound, Str("The available MIDI out devices are:\n"));
    else
      csound->Message(csound, Str("The available MIDI in devices are:\n"));
    for (i = 0; i < cnt; i++) {
      info = portMidi_getDeviceInfo(i, output);
      if (info->interf != NULL)
        csound->Message(csound, " %3d: %s (%s)\n", i, info->name, info->interf);
      else
        csound->Message(csound, " %3d: %s\n", i, info->name);
    }
}

static int start_portmidi(ENVIRON *csound)
{
    if (not_started < 0)
      return -1;
    if (!not_started)
      return 0;
    not_started = 0;
    if (Pm_Initialize() != pmNoError) {
      not_started = -1;
      csound->Message(csound, Str(" *** error initialising PortMIDI\n"));
      return -1;
    }
    if (Pt_Start(1, NULL, NULL) != ptNoError) {
      not_started = -1;
      csound->Message(csound, Str(" *** error initialising PortTime\n"));
      return -1;
    }
    return 0;
}

static int OpenMidiInDevice_(void *csound_, void **userData, const char *dev)
{
    ENVIRON     *csound;
    int         cntdev, devnum;
    PmEvent     buffer;
    PmError     retval;
    PmDeviceInfo *info;
    PortMidiStream *midistream;

    csound = (ENVIRON*) csound_;
    if (start_portmidi(csound) != 0)
      return -1;
    /* check if there are any devices available */
    cntdev = portMidi_getDeviceCount(0);
    if (cntdev < 1) {
      csound->Message(csound,
                      Str(" *** PortMIDI: no input devices are available\n"));
      return -1;
    }
    /* look up device in list */
    if (dev == NULL || dev[0] == '\0')
      devnum =
        portMidi_getPackedDeviceID((int) Pm_GetDefaultInputDeviceID(), 0);
    else if (dev[0] < '0' || dev[0] > '9') {
      csound->Message(csound, Str(" *** PortMIDI: error: must specify a "
                                  "device number (>=0), not a name\n"));
      portMidi_listDevices(csound, 0);
      return -1;
    }
    else
      devnum = (int) atoi(dev);
    if (devnum < 0 || devnum >= cntdev) {
      csound->Message(csound, Str(" *** PortMIDI: error: device number is "
                                  "out of range\n"));
      portMidi_listDevices(csound, 0);
      return -1;
    }
    info = portMidi_getDeviceInfo(devnum, 0);
    if (info->interf != NULL)
      csound->Message(csound,
                      Str("PortMIDI: selected input device %d: '%s' (%s)\n"),
                      devnum, info->name, info->interf);
    else
      csound->Message(csound, Str("PortMIDI: selected input device %d: '%s'\n"),
                              devnum, info->name);
    retval = Pm_OpenInput(&midistream,
                          (PmDeviceID) portMidi_getRealDeviceID(devnum, 0),
                          NULL, 512L, (PmTimeProcPtr) NULL, NULL);
    if (retval != pmNoError) {
      csound->Message(csound,
                      Str(" *** PortMIDI: error opening input device %d: %s\n"),
                      devnum, Pm_GetErrorText(retval));
      return -1;
    }
    *userData = (void*) midistream;
    /* only interested in channel messages (note on, control change, etc.) */
    Pm_SetFilter(midistream, (PM_FILT_REALTIME | PM_FILT_SYSTEMCOMMON));
    /* empty the buffer after setting filter */
    while (Pm_Poll(midistream) == TRUE) {
      Pm_Read(midistream, &buffer, 1);
    }
    /* report success */
    return 0;
}

static int OpenMidiOutDevice_(void *csound_, void **userData, const char *dev)
{
    ENVIRON     *csound;
    int         cntdev, devnum;
    PmError     retval;
    PmDeviceInfo *info;
    PortMidiStream *midistream;

    csound = (ENVIRON*) csound_;
    if (start_portmidi(csound) != 0)
      return -1;
    /* check if there are any devices available */
    cntdev = portMidi_getDeviceCount(1);
    if (cntdev < 1) {
      csound->Message(csound,
                      Str(" *** PortMIDI: no output devices are available\n"));
      return -1;
    }
    /* look up device in list */
    if (dev == NULL || dev[0] == '\0')
      devnum =
        portMidi_getPackedDeviceID((int) Pm_GetDefaultOutputDeviceID(), 1);
    else if (dev[0] < '0' || dev[0] > '9') {
      csound->Message(csound, Str(" *** PortMIDI: error: must specify a "
                                  "device number (>=0), not a name\n"));
      portMidi_listDevices(csound, 1);
      return -1;
    }
    else
      devnum = (int) atoi(dev);
    if (devnum < 0 || devnum >= cntdev) {
      csound->Message(csound, Str(" *** PortMIDI: error: device number is "
                                  "out of range\n"));
      portMidi_listDevices(csound, 1);
      return -1;
    }
    info = portMidi_getDeviceInfo(devnum, 1);
    if (info->interf != NULL)
      csound->Message(csound,
                      Str("PortMIDI: selected output device %d: '%s' (%s)\n"),
                      devnum, info->name, info->interf);
    else
      csound->Message(csound,
                      Str("PortMIDI: selected output device %d: '%s'\n"),
                      devnum, info->name);
    retval = Pm_OpenOutput(&midistream,
                           (PmDeviceID) portMidi_getRealDeviceID(devnum, 1),
                           NULL, 512L, (PmTimeProcPtr) NULL, NULL, 0L);
    if (retval != pmNoError) {
      csound->Message(
        csound, Str(" *** PortMIDI: error opening output device %d: %s\n"),
                devnum, Pm_GetErrorText(retval));
      return -1;
    }
    *userData = (void*) midistream;
    /* report success */
    return 0;
}

static int ReadMidiData_(void *csound_, void *userData,
                         unsigned char *mbuf, int nbytes)
{
    ENVIRON         *csound;
    int             n, retval, st, d1, d2;
    PmEvent         mev;
    PortMidiStream  *midistream;
    /*
     * Reads from user-defined MIDI input.
     */
    csound = (ENVIRON*) csound_;
    midistream = (PortMidiStream*) userData;
    retval = Pm_Poll(midistream);
    if (retval == FALSE)
      return 0;
    if (retval < 0) {
      csound->Message(csound,
                      Str(" *** PortMIDI: error polling input device\n"));
      return -1;
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
        csound->Message(csound, Str(" *** buffer overflow in MIDI input\n"));
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
    if (retval < 0) {
      csound->Message(csound, Str(" *** PortMIDI: read error %d\n"), retval);
      if (n < 1)
        n = -1;
    }
    /* return the number of bytes read */
    return n;
}

static int WriteMidiData_(void *csound_, void *userData,
                          unsigned char *mbuf, int nbytes)
{
    ENVIRON         *csound;
    int             n, st;
    PmEvent         mev;
    PortMidiStream  *midistream;
    /*
     * Writes to user-defined MIDI output.
     */
    csound = (ENVIRON*) csound_;
    midistream = (PortMidiStream*) userData;
    if (nbytes < 1)
      return 0;
    n = 0;
    do {
      st = (int) (*mbuf++);
      if (st < 0x80) {
        csound->Message(csound, Str(" *** PortMIDI: invalid MIDI out data\n"));
        break;
      }
      if (st >= 0xF0 && st < 0xF8) {
        csound->Message(csound, Str(" *** PortMIDI: MIDI out: system message "
                                    "0x%02X is not supported\n"),
                                (unsigned int) st);
        break;
      }
      nbytes -= (datbyts[(st - 0x80) >> 4] + 1);
      if (nbytes < 0) {
        csound->Message(csound, Str(" *** PortMIDI: MIDI out: "
                                    "truncated message\n"));
        break;
      }
      mev.message = (PmMessage) 0;
      mev.timestamp = (PmTimestamp) 0;
      mev.message |= (PmMessage) Pm_Message(st, 0, 0);
      if (datbyts[(st - 0x80) >> 4] > 0)
        mev.message |= (PmMessage) Pm_Message(0, (int) (*mbuf++), 0);
      if (datbyts[(st - 0x80) >> 4] > 1)
        mev.message |= (PmMessage) Pm_Message(0, 0, (int) (*mbuf++));
      if (Pm_Write(midistream, &mev, 1L) != pmNoError)
        csound->Message(csound, Str(" *** PortMIDI: MIDI out: "
                                    "error writing message\n"));
      else
        n += (datbyts[(st - 0x80) >> 4] + 1);
    } while (nbytes > 0);
    /* return the number of bytes written */
    return n;
}

static int CloseMidiInDevice_(void *csound_, void *userData)
{
    PmError retval;
    ENVIRON *csound;

    csound = (ENVIRON*) csound_;
    if (userData != NULL) {
      retval = Pm_Close((PortMidiStream*) userData);
      if (retval != pmNoError) {
        csound->Message(csound,
                        Str(" *** PortMIDI: error closing input device\n"));
        return -1;
      }
    }
    return 0;
}

static int CloseMidiOutDevice_(void *csound_, void *userData)
{
    PmError retval;
    ENVIRON *csound;

    csound = (ENVIRON*) csound_;
    if (userData != NULL) {
      retval = Pm_Close((PortMidiStream*) userData);
      if (retval != pmNoError) {
        csound->Message(csound,
                        Str(" *** PortMIDI: error closing output device\n"));
        return -1;
      }
    }
    return 0;
}

/* module interface functions */

int csoundModuleCreate(void *csound)
{
    /* nothing to do, report success */
    ((ENVIRON*) csound)->Message(csound, "PortMIDI real time MIDI plugin "
                                         "for Csound\n");
    return 0;
}

int csoundModuleInit(void *csound)
{
    ENVIRON *p;
    char    *drv;

    p = (ENVIRON*) csound;
    drv = (char*) (p->QueryGlobalVariable(csound, "_RTMIDI"));
    if (drv == NULL)
      return 0;
    if (!(strcmp(drv, "portmidi") == 0 || strcmp(drv, "PortMidi") == 0 ||
          strcmp(drv, "PortMIDI") == 0 || strcmp(drv, "pm") == 0))
      return 0;
    p->Message(csound, "rtmidi: PortMIDI module enabled\n");
    p->SetExternalMidiInOpenCallback(csound, OpenMidiInDevice_);
    p->SetExternalMidiReadCallback(csound, ReadMidiData_);
    p->SetExternalMidiInCloseCallback(csound, CloseMidiInDevice_);
    p->SetExternalMidiOutOpenCallback(csound, OpenMidiOutDevice_);
    p->SetExternalMidiWriteCallback(csound, WriteMidiData_);
    p->SetExternalMidiOutCloseCallback(csound, CloseMidiOutDevice_);
    return 0;
}

