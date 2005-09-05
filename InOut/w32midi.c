/*
    w32midi.c:

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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
    02111-1307 USA
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#include "csdl.h"

#define MBUFSIZE    1024

typedef struct rtmidi_mme_globals_ {
    HMIDIIN             inDev;
    int                 inBufWritePos;
    int                 inBufReadPos;
    DWORD               inBuf[MBUFSIZE];
    CRITICAL_SECTION    threadLock;
} RTMIDI_MME_GLOBALS;

static const unsigned char msg_bytes[32] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 2, 2, 3, 3, 0, 1
};

static void CALLBACK midi_in_handler(HMIDIIN hmin, UINT wMsg, DWORD dwInstance,
                                     DWORD dwParam1, DWORD dwParam2)
{
    RTMIDI_MME_GLOBALS  *p = (RTMIDI_MME_GLOBALS*) dwInstance;
    int                 new_pos;

    (void) hmin;
    (void) dwParam2;
    if (wMsg != MIM_DATA)       /* ignore non-data messages */
      return;
    EnterCriticalSection(&(p->threadLock));
    new_pos = (p->inBufWritePos + 1) & (MBUFSIZE - 1);
    if (new_pos != p->inBufReadPos) {
      p->inBuf[p->inBufWritePos] = dwParam1;
      p->inBufWritePos = new_pos;
    }
    LeaveCriticalSection(&(p->threadLock));
}

static int midi_in_open(CSOUND *csound, void **userData, const char *devName)
{
    int                 ndev, devnum = 0;
    RTMIDI_MME_GLOBALS  *p;
    MIDIINCAPS          caps;

    *userData = NULL;
    ndev = (int) midiInGetNumDevs();
    if (ndev < 1) {
      csound->ErrorMsg(csound, Str("rtmidi: no input devices are available"));
      return -1;
    }
    if (devName != NULL && devName[0] != '\0' &&
        strcmp(devName, "default") != 0) {
      if (devName[0] < '0' || devName[0] > '9') {
        csound->ErrorMsg(csound, Str("rtmidi: must specify a device number, "
                                     "not a name"));
        return -1;
      }
      devnum = (int) atoi(devName);
    }
    if (devnum < 0 || devnum >= ndev) {
      csound->Message(csound, Str("The available MIDI input devices are:\n"));
      for (devnum = 0; devnum < ndev; devnum++) {
        midiInGetDevCaps((unsigned int) devnum, &caps, sizeof(MIDIINCAPS));
        csound->Message(csound, "%3d: %s\n", devnum, &(caps.szPname[0]));
      }
      csound->ErrorMsg(csound,
                       Str("rtmidi: input device number is out of range"));
      return -1;
    }
    p = (RTMIDI_MME_GLOBALS*) calloc((size_t) 1, sizeof(RTMIDI_MME_GLOBALS));
    if (p == NULL) {
      csound->ErrorMsg(csound, Str("rtmidi: memory allocation failure"));
      return -1;
    }
    InitializeCriticalSection(&(p->threadLock));
    *userData = (void*) p;
    midiInGetDevCaps((unsigned int) devnum, &caps, sizeof(MIDIINCAPS));
    csound->Message(csound, Str("Opening MIDI input device %d (%s)\n"),
                            devnum, &(caps.szPname[0]));
    if (midiInOpen(&(p->inDev), (unsigned int) devnum,
                   (DWORD) midi_in_handler, (DWORD) p, CALLBACK_FUNCTION)
        != MMSYSERR_NOERROR) {
      p->inDev = (HMIDIIN) 0;
      csound->ErrorMsg(csound, Str("rtmidi: could not open input device"));
      return -1;
    }
    midiInStart(p->inDev);

    return 0;
}

static int midi_in_read(CSOUND *csound,
                        void *userData, unsigned char *buf, int nbytes)
{
    RTMIDI_MME_GLOBALS  *p = (RTMIDI_MME_GLOBALS*) userData;
    unsigned int        tmp;
    unsigned char       s, d1, d2, len;
    int                 nread = 0;

    (void) csound;
    EnterCriticalSection(&(p->threadLock));
    while (p->inBufReadPos != p->inBufWritePos) {
      tmp = (unsigned int) p->inBuf[p->inBufReadPos++];
      p->inBufReadPos &= (MBUFSIZE - 1);
      s = (unsigned char) tmp; tmp >>= 8;
      d1 = (unsigned char) tmp; tmp >>= 8;
      d2 = (unsigned char) tmp;
      len = msg_bytes[(int) s >> 3];
      if (nread + len > nbytes)
        break;
      if (len) {
        buf[nread++] = s;
        if (--len) {
          buf[nread++] = d1;
          if (--len)
            buf[nread++] = d2;
        }
      }
    }
    LeaveCriticalSection(&(p->threadLock));

    return nread;
}

static int midi_in_close(CSOUND *csound, void *userData)
{
    RTMIDI_MME_GLOBALS  *p = (RTMIDI_MME_GLOBALS*) userData;

    (void) csound;
    if (p == NULL)
      return 0;
    if (p->inDev == (HMIDIIN) 0) {
      midiInStop(p->inDev);
      midiInReset(p->inDev);
      midiInClose(p->inDev);
    }
    DeleteCriticalSection(&(p->threadLock));
    free(p);

    return 0;
}

static int midi_out_open(CSOUND *csound, void **userData, const char *devName)
{
    int         ndev, devnum = 0;
    MIDIOUTCAPS caps;
    HMIDIOUT    outDev = (HMIDIOUT) 0;

    *userData = NULL;
    ndev = (int) midiOutGetNumDevs();
    if (ndev < 1) {
      csound->ErrorMsg(csound, Str("rtmidi: no output devices are available"));
      return -1;
    }
    if (devName != NULL && devName[0] != '\0' &&
        strcmp(devName, "default") != 0) {
      if (devName[0] < '0' || devName[0] > '9') {
        csound->ErrorMsg(csound, Str("rtmidi: must specify a device number, "
                                     "not a name"));
        return -1;
      }
      devnum = (int) atoi(devName);
    }
    if (devnum < 0 || devnum >= ndev) {
      csound->Message(csound, Str("The available MIDI output devices are:\n"));
      for (devnum = 0; devnum < ndev; devnum++) {
        midiOutGetDevCaps((unsigned int) devnum, &caps, sizeof(MIDIOUTCAPS));
        csound->Message(csound, "%3d: %s\n", devnum, &(caps.szPname[0]));
      }
      csound->ErrorMsg(csound,
                       Str("rtmidi: output device number is out of range"));
      return -1;
    }
    midiOutGetDevCaps((unsigned int) devnum, &caps, sizeof(MIDIOUTCAPS));
    csound->Message(csound, Str("Opening MIDI output device %d (%s)\n"),
                            devnum, &(caps.szPname[0]));
    if (midiOutOpen(&outDev, (unsigned int) devnum,
                    (DWORD) 0, (DWORD) 0, CALLBACK_NULL) != MMSYSERR_NOERROR) {
      csound->ErrorMsg(csound, Str("rtmidi: could not open output device"));
      return -1;
    }
    *userData = (void*) outDev;

    return 0;
}

static int midi_out_write(CSOUND *csound,
                          void *userData, const unsigned char *buf, int nbytes)
{
    HMIDIOUT        outDev = (HMIDIOUT) userData;
    unsigned int    tmp;
    unsigned char   s, len;
    int             pos = 0;

    (void) csound;
    while (pos < nbytes) {
      s = buf[pos++];
      len = msg_bytes[(int) s >> 3];
      if (!len)
        continue;
      tmp = (unsigned int) s;
      if (--len) {
        if (pos >= nbytes)
          break;
        tmp |= ((unsigned int) buf[pos++] << 8);
        if (--len) {
          if (pos >= nbytes)
            break;
          tmp |= ((unsigned int) buf[pos++] << 16);
        }
      }
      midiOutShortMsg(outDev, (DWORD) tmp);
    }

    return pos;
}

static int midi_out_close(CSOUND *csound, void *userData)
{
    (void) csound;
    if (userData != NULL) {
      HMIDIOUT  outDev = (HMIDIOUT) userData;
      midiOutReset(outDev);
      midiOutClose(outDev);
    }

    return 0;
}

/* module interface functions */

PUBLIC int csoundModuleCreate(CSOUND *csound)
{
    /* nothing to do, report success */
    csound->Message(csound, "Windows MME real time MIDI plugin for Csound\n");
    return 0;
}

PUBLIC int csoundModuleInit(CSOUND *csound)
{
    char    *drv;

    drv = (char*) (csound->QueryGlobalVariable(csound, "_RTMIDI"));
    if (drv == NULL)
      return 0;
    if (!(strcmp(drv, "WinMM") == 0 || strcmp(drv, "winmm") == 0 ||
          strcmp(drv, "MME") == 0 || strcmp(drv, "mme") == 0))
      return 0;
    csound->Message(csound, "rtmidi: WinMM module enabled\n");
    csound->SetExternalMidiInOpenCallback(csound, midi_in_open);
    csound->SetExternalMidiReadCallback(csound, midi_in_read);
    csound->SetExternalMidiInCloseCallback(csound, midi_in_close);
    csound->SetExternalMidiOutOpenCallback(csound, midi_out_open);
    csound->SetExternalMidiWriteCallback(csound, midi_out_write);
    csound->SetExternalMidiOutCloseCallback(csound, midi_out_close);
    return 0;
}

PUBLIC int csoundModuleInfo(void)
{
    /* does not depend on MYFLT type */
    return ((CS_APIVERSION << 16) + (CS_APISUBVER << 8));
}

