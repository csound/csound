/*
    pmidi.c:

    Copyright (C) 2004 John ffitch after Barry Vercoe
              (C) 2005 Istvan Varga
              (C) 2008 Andres Cabrera

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

/* Realtime MIDI using Portmidi library */

#include "csdl.h"                               /*      PMIDI.C         */
#include "csGblMtx.h"
#include "midiops.h"
#include <portmidi.h>
#include <porttime.h>

/* Stub for compiling this file with MinGW and linking
   with portmidi.lib built with MSVC AND with Windows
   libraries from MinGW (missing __wassert).
*/
#if defined(WIN32) && !defined(MSVC)

void _wassert(wchar_t *condition)
{
}

#endif

typedef struct _pmall_data {
  PortMidiStream *midistream;
  int32_t multiport_flag;
  struct _pmall_data *next;
} pmall_data;

static  const   int32_t     datbyts[8] = { 2, 2, 2, 2, 1, 1, 2, 0 };

static int32_t portMidiErrMsg(CSOUND *csound, const char *msg, ...)
{
    va_list args;
    va_start(args, msg);
    csound->ErrMsgV(csound, " *** PortMIDI: ", msg, args);
    va_end(args);
    return -1;
}

static int32_t portMidi_getDeviceCount(int32_t output)
{
    int32_t           i, cnt1, cnt2;
    PmDeviceInfo  *info;

    cnt1 = (int32_t)Pm_CountDevices();
    if (UNLIKELY(cnt1 < 1))
      return cnt1;      /* no devices */
    cnt2 = 0;
    for (i = 0; i < cnt1; i++) {
      info = (PmDeviceInfo*)Pm_GetDeviceInfo((PmDeviceID) i);
      if (output && info->output)
        cnt2++;
      else if (!output && info->input)
        cnt2++;
    }
    return cnt2;
}

static int32_t portMidi_getRealDeviceID(int32_t dev, int32_t output)
{
    int32_t           i, j, cnt;
    PmDeviceInfo  *info;

    cnt = (int32_t)Pm_CountDevices();
    i = j = -1;
    while (++i < cnt) {
      info = (PmDeviceInfo*)Pm_GetDeviceInfo((PmDeviceID) i);
      if ((output && !(info->output)) || (!output && !(info->input)))
        continue;
      if (++j == dev)
        return i;
    }
    return -1;
}

static int32_t portMidi_getPackedDeviceID(int32_t dev, int32_t output)
{
    int32_t           i, j, cnt;
    PmDeviceInfo  *info;

    cnt = (int32_t)Pm_CountDevices();
    i = j = -1;
    while (++i < cnt) {
      info = (PmDeviceInfo*)Pm_GetDeviceInfo((PmDeviceID) i);
      if ((output && info->output) || (!output && info->input))
        j++;
      if (i == dev)
        return j;
    }
    return -1;
}

static PmDeviceInfo *portMidi_getDeviceInfo(int32_t dev, int32_t output)
{
    int32_t i;

    i = portMidi_getRealDeviceID(dev, output);
    if (UNLIKELY(i < 0))
      return NULL;
    return ((PmDeviceInfo*)Pm_GetDeviceInfo((PmDeviceID) i));
}
/* reference count for PortMidi initialisation */

static unsigned long portmidi_init_cnt = 0UL;

static int32_t stop_portmidi(CSOUND *csound, void *userData)
{
    (void) csound;
    (void) userData;
#if !defined(WIN32)
    csoundLock();
#endif
    if (portmidi_init_cnt) {
      if (--portmidi_init_cnt == 0UL) {
        Pm_Terminate();
        Pt_Stop();
      }
    }
#if !defined(WIN32)
    csoundUnLock();
#endif
    return 0;
}

static int32_t start_portmidi(CSOUND *csound)
{
    const char  *errMsg = NULL;

#if !defined(WIN32)
    csoundLock();
#endif
    if (!portmidi_init_cnt) {
      if (UNLIKELY(Pm_Initialize() != pmNoError))
        errMsg = Str(" *** error initialising PortMIDI");
      else if (UNLIKELY(Pt_Start(1, NULL, NULL) != ptNoError))
        errMsg = Str(" *** error initialising PortTime");
      }

    if (errMsg == NULL)
      portmidi_init_cnt++;
#if !defined(WIN32)
    csoundUnLock();
#endif
    if (UNLIKELY(errMsg != NULL)) {
      csound->ErrorMsg(csound, "%s", Str(errMsg));
      return -1;
    }
    //#if !defined(WIN32)
    //csound_global_mutex_unlock();
    //#endif
    return csound->RegisterResetCallback(csound, NULL, stop_portmidi);
}

static int32_t listDevices(CSOUND *csound, CS_MIDIDEVICE *list, int32_t isOutput){
  int32_t i, cnt;
  PmDeviceInfo  *info;
  char tmp[64];
  char *drv = (char*) (csound->QueryGlobalVariable(csound, "_RTMIDI"));

  if (UNLIKELY(start_portmidi(csound) != 0))
    return 0;

  cnt = portMidi_getDeviceCount(isOutput);
  if (list == NULL) return cnt;
  for (i = 0; i < cnt; i++) {
    info = portMidi_getDeviceInfo(i, isOutput);
    if (info->name != NULL)
      strncpy(list[i].device_name, info->name, 63);
    snprintf(tmp, 64, "%d", i);
    strncpy(list[i].device_id, tmp, 63);
    list[i].isOutput = isOutput;
    if (info->interf != NULL)
      strncpy(list[i].interface_name, info->interf, 63);
    else strcpy(list[i].interface_name, "");
    strncpy(list[i].midi_module, drv, 63);
  }
  return cnt;
}

static void portMidi_listDevices(CSOUND *csound, int32_t output)
{
    int32_t i,n = listDevices(csound, NULL, output);
    CS_MIDIDEVICE *devs =
      (CS_MIDIDEVICE *) csound->Malloc(csound, n*sizeof(CS_MIDIDEVICE));
    listDevices(csound, devs, output);
    {
    for(i=0; i < n; i++)
      csound->ErrorMsg(csound, "%s: %s (%s)\n",
                      devs[i].device_id, devs[i].device_name, devs[i].midi_module);
    }
    csound->Free(csound, devs);
}


static int32_t OpenMidiInDevice_(CSOUND *csound, void **userData, const char *dev)
{
    int32_t         cntdev, devnum, opendevs, i;
    PmEvent     buffer;
    PmError     retval;
    PmDeviceInfo *info;
//     PortMidiStream *midistream;
    pmall_data *data = NULL;
    pmall_data *next = NULL;
    int32_t port = 0;

    if (UNLIKELY(start_portmidi(csound) != 0))
      return -1;
    /* check if there are any devices available */
    cntdev = portMidi_getDeviceCount(0);
    portMidi_listDevices(csound, 0);
    /* look up device in list */
    if (dev == NULL || dev[0] == '\0')
      devnum =
        portMidi_getPackedDeviceID((int32_t)Pm_GetDefaultInputDeviceID(), 0);
    else if (UNLIKELY((dev[0] < '0' || dev[0] > '9') && dev[0] != 'a' && dev[0] != 'm')) {
      portMidiErrMsg(csound,
                     Str("error: must specify a device number (>=0),"
                         " 'a' for all (merged), or 'm' for port mapped, not a name"));
      return -1;
    }
    else if (dev[0] != 'a' && dev[0] != 'm') {
      devnum = (int32_t)atoi(dev);
      if (UNLIKELY(devnum < 0 || devnum >= cntdev)) {
        portMidiErrMsg(csound, Str("error: device number is out of range"));
        return -1;
      }
    }
    else {
    // allow to proceed if 'a' OR 'm' is given even if there are no MIDI devices
      devnum = -1;
    }

    if (UNLIKELY(cntdev < 1 && (dev==NULL || dev[0] != 'a' || dev[0] != 'm'))) {
      return portMidiErrMsg(csound, Str("no input devices are available"));
    }
    opendevs = 0;
    for (i = 0; i < cntdev; i++) {
      if (devnum == i || devnum == -1) {
        if (opendevs == 0) {
          data = (pmall_data *) csound->Malloc(csound, sizeof(pmall_data));
          next = data;
          data->next = NULL;
          opendevs++;
        }
        else {
          next->next = (pmall_data *) csound->Malloc(csound, sizeof(pmall_data));
          next = next->next;
          next->next = NULL;
          opendevs++;
        }
        info = portMidi_getDeviceInfo(i, 0);
        {

         if (info->interf != NULL) {

          csound->ErrorMsg(csound,
                          Str("PortMIDI: Activated input device %d: '%s' (%s)\n"),
                          i, info->name, info->interf);
          if(dev[0] == 'm')
            csound->ErrorMsg(csound, Str("Device mapped to channels %d to %d \n"),
                                        port*16+1, (port+1)*16);
        }
          else {
          csound->ErrorMsg(csound,
                          Str("PortMIDI: Activated input device %d: '%s'\n"),
                          i, info->name);
          if(dev[0] == 'm')
            csound->ErrorMsg(csound,Str("Device mapped to channels %d to %d \n"),
                                       port*16+1, (port+1)*16);
          }
        }
        /* set multiport mapping if asked */
        if(dev[0] == 'm') next->multiport_flag = 1;
        else next->multiport_flag = 0;
          port++;
        retval = Pm_OpenInput(&next->midistream,
                 (PmDeviceID) portMidi_getRealDeviceID(i, 0),
                         NULL, 512L, (PmTimeProcPtr) NULL, NULL);
        if (UNLIKELY(retval != pmNoError)) {
          // Prevent leaking memory from "data"
          if (data) {
            next = data->next;
            csound->Free(csound, data);
          }
          return portMidiErrMsg(csound, Str("error opening input device %d: %s"),
                                          i, Pm_GetErrorText(retval));
        }
        /* only interested in channel messages (note on, control change, etc.) */
        Pm_SetFilter(next->midistream, (PM_FILT_ACTIVE | PM_FILT_SYSEX));
        /* empty the buffer after setting filter */
        while (Pm_Poll(next->midistream) == TRUE) {
          Pm_Read(next->midistream, &buffer, 1);
        }
      }
    }
    *userData = (void*) data;
    /* report success */
    return 0;
}

static int32_t OpenMidiOutDevice_(CSOUND *csound, void **userData, const char *dev)
{
    int32_t         cntdev, devnum;
    PmError     retval;
    PmDeviceInfo *info;
    PortMidiStream *midistream;

    if (UNLIKELY(start_portmidi(csound) != 0))
      return -1;
    /* check if there are any devices available */
    cntdev = portMidi_getDeviceCount(1);
    if (UNLIKELY(cntdev < 1)) {
      return portMidiErrMsg(csound, Str("no output devices are available"));
    }
    /* look up device in list */
    portMidi_listDevices(csound, 1);
    if (dev == NULL || dev[0] == '\0')
      devnum =
        portMidi_getPackedDeviceID((int32_t)Pm_GetDefaultOutputDeviceID(), 1);
    else if (UNLIKELY(dev[0] < '0' || dev[0] > '9')) {
      portMidiErrMsg(csound, Str("error: must specify a device number (>=0), "
                                 "not a name"));
      return -1;
    }
    else
      devnum = (int32_t)atoi(dev);
    if (UNLIKELY(devnum < 0 || devnum >= cntdev)) {
      portMidiErrMsg(csound, Str("error: device number is out of range"));
      return -1;
    }
    info = portMidi_getDeviceInfo(devnum, 1);
    if (info->interf != NULL)
      csound->ErrorMsg(csound,
                      Str("PortMIDI: selected output device %d: '%s' (%s)\n"),
                      devnum, info->name, info->interf);
    else
      csound->ErrorMsg(csound,
                      Str("PortMIDI: selected output device %d: '%s'\n"),
                      devnum, info->name);


    retval = Pm_OpenOutput(&midistream,
                           (PmDeviceID) portMidi_getRealDeviceID(devnum, 1),
                           NULL, 512L, (PmTimeProcPtr) NULL, NULL, 0L);
    if (UNLIKELY(retval != pmNoError)) {
      return portMidiErrMsg(csound, Str("error opening output device %d: %s"),
                                    devnum, Pm_GetErrorText(retval));
    }
    *userData = (void*) midistream;
    /* report success */

    return 0;
}

static int32_t ReadMidiData_(CSOUND *csound, void *userData,
                         unsigned char *mbuf, int32_t nbytes)
{
  int32_t             n, retval, st, d1, d2;
  unsigned char port = 0, map = 0;
    PmEvent         mev;
    pmall_data *data;
    /*
     * Reads from MIDI input device linked list.
     */
    n = 0;
    data = (pmall_data *)userData;

    while (data) {
      retval = Pm_Poll(data->midistream);
      if(data->multiport_flag) map = 1;

      if (retval != FALSE) {
        if (UNLIKELY(retval < 0))
          return portMidiErrMsg(csound, Str("error polling input device"));
        while ((retval = Pm_Read(data->midistream, &mev, 1L)) > 0) {
          st = (int32_t)Pm_MessageStatus(mev.message);
          d1 = (int32_t)Pm_MessageData1(mev.message);
          d2 = (int32_t)Pm_MessageData2(mev.message);
          /* unknown message or sysex data: ignore */
          if (UNLIKELY(st < 0x80))
            continue;
          /* ignore most system messages */
          if (UNLIKELY(st >= 0xF0 &&
                       !(st == 0xF8 || st == 0xFA || st == 0xFB ||
                         st == 0xFC || st == 0xFF)))
            continue;
          nbytes -= (datbyts[(st - 0x80) >> 4] + 1 + map);
          if (UNLIKELY(nbytes < 0)) {
            portMidiErrMsg(csound, Str("buffer overflow in MIDI input"));
            break;
          }
          /* channel messages */
          n += (datbyts[(st - 0x80) >> 4] + 1 + map);
          switch (datbyts[(st - 0x80) >> 4]) {
            case 0:
              *mbuf++ = (unsigned char) st;
              /* don't add port if there is no data byte to follow */
              break;
            case 1:
              *mbuf++ = (unsigned char) st;
              /* if mapping add port to byte after status */
              if(map) *mbuf++ = (unsigned char) (0x80 | port);
              *mbuf++ = (unsigned char) d1;
              break;
            case 2:
              *mbuf++ = (unsigned char) st;
              /* if mapping add port to byte after status */
              if(map) *mbuf++ = (unsigned char) (0x80 | port);
              *mbuf++ = (unsigned char) d1;
              *mbuf++ = (unsigned char) d2;
              break;
          }
        }
        if (UNLIKELY(retval < 0)) {
          portMidiErrMsg(csound, Str("read error %d"), retval);
          if (n < 1)
            n = -1;
        }
      }
      data = data->next;
      port++; /* increment port number for mapping */
    }
    /* return the number of bytes read */
    return n;
}

static int32_t WriteMidiData_(CSOUND *csound, void *userData,
                          const unsigned char *mbuf, int32_t nbytes)
{
    int32_t             n, st;
    PmEvent         mev;
    PortMidiStream  *midistream;
    /*
     * Writes to user-defined MIDI output.
     */
    midistream = (PortMidiStream*) userData;
    if (UNLIKELY(nbytes < 1))
      return 0;
    n = 0;
    do {
      st = (int32_t)*(mbuf++);
      if (UNLIKELY(st < 0x80)) {
        portMidiErrMsg(csound, Str("invalid MIDI out data"));
        break;
      }
      if (UNLIKELY(st >= 0xF0 && st < 0xF8)) {
        portMidiErrMsg(csound,
                       Str("MIDI out: system message 0x%02X is not supported"),
                       (uint32_t) st);
        break;
      }
      nbytes -= (datbyts[(st - 0x80) >> 4] + 1);
      if (UNLIKELY(nbytes < 0)) {
        portMidiErrMsg(csound, Str("MIDI out: truncated message"));
        break;
      }
      mev.message = (PmMessage) 0;
      mev.timestamp = (PmTimestamp) 0;
      mev.message |= (PmMessage) Pm_Message(st, 0, 0);
      if (datbyts[(st - 0x80) >> 4] > 0)
        mev.message |= (PmMessage) Pm_Message(0, (int32_t)*(mbuf++), 0);
      if (datbyts[(st - 0x80) >> 4] > 1)
        mev.message |= (PmMessage) Pm_Message(0, 0, (int32_t)*(mbuf++));
      if (UNLIKELY(Pm_Write(midistream, &mev, 1L) != pmNoError))
        portMidiErrMsg(csound, Str("MIDI out: error writing message"));
      else {
        n += (datbyts[(st - 0x80) >> 4] + 1);
      }
    } while (nbytes > 0);
    /* return the number of bytes written */
    return n;
}

static int32_t CloseMidiInDevice_(CSOUND *csound, void *userData)
{
    PmError retval;
    pmall_data* data = (pmall_data*) userData;
    while (data) {
      retval = Pm_Close(data->midistream);
      if (UNLIKELY(retval != pmNoError)) {
        return portMidiErrMsg(csound, Str("error closing input device"));
      }
      pmall_data* olddata;
      olddata = data;
      data = data->next;
      csound->Free(csound, olddata);
    }
    return 0;
}

static int32_t CloseMidiOutDevice_(CSOUND *csound, void *userData)
{
    PmError retval;

    if (userData != NULL) {
      retval = Pm_Close((PortMidiStream*) userData);
      if (UNLIKELY(retval != pmNoError)) {
        return portMidiErrMsg(csound, Str("error closing output device"));
      }
    }
    return 0;
}

/* module interface functions */

PUBLIC int32_t csoundModuleCreate(CSOUND *csound)
{
    /* nothing to do, report success */
    // csound->ErrorMsg(csound, Str("PortMIDI real time MIDI plugin for Csound\n"));
   IGN(csound);
    return 0;
}

PUBLIC int32_t csoundModuleInit(CSOUND *csound)
{
    char    *drv;
    csound->ModuleListAdd(csound, "portmidi", "midi");
    drv = (char*) (csound->QueryGlobalVariable(csound, "_RTMIDI"));
    if (drv == NULL)
      return 0;
    if (!(strcmp(drv, "portmidi") == 0 || strcmp(drv, "PortMidi") == 0 ||
          strcmp(drv, "PortMIDI") == 0 || strcmp(drv, "pm") == 0))
      return 0;
     csound->ErrorMsg(csound, "%s", Str("rtmidi: PortMIDI module enabled\n"));
    csound->SetExternalMidiInOpenCallback(csound, OpenMidiInDevice_);
    csound->SetExternalMidiReadCallback(csound, ReadMidiData_);
    csound->SetExternalMidiInCloseCallback(csound, CloseMidiInDevice_);
    csound->SetExternalMidiOutOpenCallback(csound, OpenMidiOutDevice_);
    csound->SetExternalMidiWriteCallback(csound, WriteMidiData_);
    csound->SetExternalMidiOutCloseCallback(csound, CloseMidiOutDevice_);
    csound->SetMIDIDeviceListCallback(csound,listDevices);

    return 0;
}

PUBLIC int32_t csoundModuleDestroy(CSOUND *csound) {

  IGN(csound);
    return 0;
}

PUBLIC int32_t csoundModuleInfo(void)
{
    /* does not depend on MYFLT type */
    return ((CS_VERSION << 16) + (CS_SUBVER << 8));
}
