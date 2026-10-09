/*
  cmidi.c:

  Copyright (C) 2011 V Lazzarini

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

/* Realtime MIDI using coremidi */


#include <CoreMIDI/CoreMIDI.h>
#ifndef IOS
#include <CoreAudio/HostTime.h>
#endif
#include <CoreFoundation/CoreFoundation.h>
#include "csdl.h"                               /*      CMIDI.C         */
#include "midiops.h"


/* MIDI message queue size */
#define DSIZE 4096

/* MIDI data struct */
typedef struct {
  Byte status;
  Byte data1;
  Byte data2;
  Byte flag;
} MIDIdata;

/* user data for MIDI input callbacks */
typedef struct _cdata {
  MIDIdata *mdata;
  int32_t p; int32_t q;
  MIDIClientRef mclient;
} cdata;

/* user data for MIDI output */
typedef struct _odata {
  MIDIClientRef mclient;
  MIDIPortRef mport;
  MIDIEndpointRef *dest;
  int32_t ndest;
  int32_t multiport;
} odata;


/* copy the name of a CoreMIDI endpoint into a fixed-size buffer */
static void getEndpointName(MIDIEndpointRef endpoint, char *buf, size_t size)
{
  CFStringRef name = NULL;
  buf[0] = '\0';
  if (MIDIObjectGetStringProperty(endpoint, kMIDIPropertyName, &name) == noErr
      && name != NULL) {
    CFStringGetCString(name, buf, (CFIndex) size, kCFStringEncodingUTF8);
    CFRelease(name);
  }
}


/* coremidi callback, called when MIDI data is available */
static void ReadProc(const MIDIPacketList *pktlist, void *refcon, void *srcConnRefCon)
{
  IGN(srcConnRefCon);
  cdata *data = (cdata *)refcon;
  MIDIdata *mdata = data->mdata;
  int32_t *p = &data->p;
  UInt32 i, j;
  MIDIPacket *packet = &((MIDIPacketList *)pktlist)->packet[0];
  Byte *curpack;

  for (i = 0; i < pktlist->numPackets; i++) {
    for (j=0; j < packet->length; j+=3) {
      curpack = packet->data+j;
      memcpy(&mdata[*p], curpack, 3);
      mdata[*p].flag = 1;
      (*p)++;
      if (*p == DSIZE) *p = 0;
    }
    packet = MIDIPacketNext(packet);
  }

}


static int32_t listDevices(CSOUND *csound, CS_MIDIDEVICE *list, int32_t isOutput)
{
  int32_t k, endpoints;
  MIDIEndpointRef endpoint;
  char tmp[64];
  char *drv = (char*) (csound->QueryGlobalVariable(csound, "_RTMIDI"));
  endpoints = isOutput ? (int32_t) MIDIGetNumberOfDestinations()
                       : (int32_t) MIDIGetNumberOfSources();
  if (list == NULL) return endpoints;
  for(k=0; k < endpoints; k++) {
    endpoint = isOutput ? MIDIGetDestination(k) : MIDIGetSource(k);
    getEndpointName(endpoint, list[k].device_name,
                    sizeof(list[k].device_name));
    snprintf(tmp, 64, "%d", k);
    snprintf(list[k].device_id, sizeof(list[k].device_id), "%s", tmp);
    list[k].isOutput = isOutput;
    list[k].interface_name[0] = '\0';
    list[k].midi_module[0] = '\0';
    if (drv != NULL)
      snprintf(list[k].midi_module, sizeof(list[k].midi_module), "%s", drv);
  }
  return endpoints;
}


/* csound MIDI input open callback, sets the device for input */
static int32_t MidiInDeviceOpen(CSOUND *csound, void **userData, const char *dev)
{
  int32_t k, endpoints;
  CFStringRef name = NULL, cname = NULL, pname = NULL;
  CFStringEncoding defaultEncoding = CFStringGetSystemEncoding();
  MIDIClientRef mclient = (MIDIClientRef) 0;
  MIDIPortRef mport =  (MIDIPortRef) 0;
  MIDIEndpointRef endpoint;
  MIDIdata *mdata = (MIDIdata *) csound->Malloc(csound, DSIZE*sizeof(MIDIdata));
  OSStatus ret;
  cdata *refcon = (cdata *) csound->Malloc(csound, sizeof(cdata));
  memset(mdata, 0, sizeof(MIDIdata)*DSIZE);
  refcon->mdata = mdata;
  refcon->p = 0;
  refcon->q = 0;
  /* MIDI client */
  cname = CFStringCreateWithCString(NULL, "my client", defaultEncoding);
  ret = MIDIClientCreate(cname, NULL, NULL, &mclient);
  if (!ret){
    /* MIDI input port */
    pname = CFStringCreateWithCString(NULL, "inport", defaultEncoding);
    ret = MIDIInputPortCreate(mclient, pname, ReadProc, refcon, &mport);
    if (!ret){
      /* sources, we connect to all available input sources */
      endpoints = (int32_t) MIDIGetNumberOfSources();
      const OPARMS *O;
      O = csound->GetOParms(csound);
      if(O->msglevel || O->odebug)
        csound->Message(csound, Str("%d MIDI sources in system\n"), endpoints);
      if (!strcmp(dev,"all")) {
        if(O->msglevel || O->odebug)
          csound->Message(csound, "%s", Str("receiving from all sources\n"));
        for(k=0; k < endpoints; k++){
          endpoint = MIDIGetSource(k);
          long srcRefCon = (long) endpoint;
          MIDIPortConnectSource(mport, endpoint, (void *) srcRefCon);
          MIDIObjectGetStringProperty(endpoint, kMIDIPropertyName, &name);
          if(O->msglevel || O->odebug)
            csound->Message(csound, Str("connecting midi device %d: %s\n"), k,
                            CFStringGetCStringPtr(name, defaultEncoding));
        }
      }
      else{
        k = atoi(dev);
        if (k < endpoints){
          endpoint = MIDIGetSource(k);
          long srcRefCon = (long) endpoint;
          MIDIPortConnectSource(mport, endpoint, (void *) srcRefCon);
          MIDIObjectGetStringProperty(endpoint, kMIDIPropertyName, &name);
          if(O->msglevel || O->odebug)
            csound->Message(csound, Str("connecting midi device %d: %s\n"), k,
                            CFStringGetCStringPtr(name, defaultEncoding));
        }
        else {
          if(O->msglevel || O->odebug)
            csound->Message(csound,
                            Str("MIDI device number %d is out-of-range, "
                                "not connected\n"), k);
        }
      }

    }
  }
  refcon->mclient = mclient;
  *userData = (void*) refcon;
  if (name) CFRelease(name);
  if (pname) CFRelease(pname);
  if (cname) CFRelease(cname);
  /* report success */
  return 0;
}

static int32_t MidiOutDeviceOpen(CSOUND *csound, void **userData, const char *dev)
{
  int32_t k, ndest, devnum = 0, openall = 0, multiport = 0;
  CFStringRef cname = NULL, pname = NULL;
  CFStringEncoding defaultEncoding = CFStringGetSystemEncoding();
  const OPARMS *O = csound->GetOParms(csound);
  odata *data = (odata *) csound->Malloc(csound, sizeof(odata));
  OSStatus ret;

  data->mclient = (MIDIClientRef) 0;
  data->mport = (MIDIPortRef) 0;
  data->dest = NULL;
  data->ndest = 0;
  data->multiport = 0;

  ndest = (int32_t) MIDIGetNumberOfDestinations();

  if (dev == NULL || dev[0] == '\0')
    devnum = 0;
  else if (!strcmp(dev, "all") || !strcmp(dev, "a"))
    openall = 1;
  else if (!strcmp(dev, "m")) {
    openall = 1;
    multiport = 1;
  }
  else if (dev[0] >= '0' && dev[0] <= '9')
    devnum = atoi(dev);
  else {
    if(O->msglevel || O->odebug)
      csound->Message(csound,
                      Str("CoreMIDI: invalid output device '%s', "
                          "no device opened\n"), dev);
    *userData = (void*) data;
    return 0;
  }

  if (openall) {
    if (ndest < 1) {
      if(O->msglevel || O->odebug)
        csound->Message(csound,
                        Str("CoreMIDI: no MIDI output destinations available\n"));
      *userData = (void*) data;
      return 0;
    }
  }
  else if (devnum < 0 || devnum >= ndest) {
    if(O->msglevel || O->odebug)
      csound->Message(csound,
                      Str("MIDI output device number %d is out-of-range, "
                          "not connected\n"), devnum);
    *userData = (void*) data;
    return 0;
  }

  /* MIDI client and output port */
  cname = CFStringCreateWithCString(NULL, "csound out client",
                                    defaultEncoding);
  ret = MIDIClientCreate(cname, NULL, NULL, &data->mclient);
  if (!ret) {
    pname = CFStringCreateWithCString(NULL, "outport", defaultEncoding);
    ret = MIDIOutputPortCreate(data->mclient, pname, &data->mport);
  }
  if (ret) {
    csound->Message(csound, "%s", Str("CoreMIDI: could not open output port\n"));
    if (data->mclient) MIDIClientDispose(data->mclient);
    if (pname) CFRelease(pname);
    if (cname) CFRelease(cname);
    csound->Free(csound, data);
    return -1;
  }

  if (openall) {
    data->dest = (MIDIEndpointRef *)
      csound->Malloc(csound, ndest*sizeof(MIDIEndpointRef));
    for (k = 0; k < ndest; k++)
      data->dest[data->ndest++] = MIDIGetDestination(k);
    data->multiport = multiport;
    if(O->msglevel || O->odebug)
      csound->Message(csound,
                      Str("CoreMIDI: sending to all %d output destinations%s\n"),
                      data->ndest, multiport ? Str(" (port mapped)") : "");
  }
  else {
    char name[128];
    data->dest = (MIDIEndpointRef *)
      csound->Malloc(csound, sizeof(MIDIEndpointRef));
    data->dest[0] = MIDIGetDestination(devnum);
    data->ndest = 1;
    if(O->msglevel || O->odebug) {
      getEndpointName(data->dest[0], name, sizeof(name));
      csound->Message(csound, Str("selected MIDI output device %d: %s\n"),
                      devnum, name);
    }
  }

  if (pname) CFRelease(pname);
  if (cname) CFRelease(cname);
  *userData = (void*) data;
  /* report success */
  return 0;
}

/* used to distinguish between 1 and 2-byte messages */
static  const   int32_t     datbyts[8] = { 2, 2, 2, 2, 1, 1, 2, 0 };

/* csound MIDI read callback, called every k-cycle */
static int32_t MidiDataRead(CSOUND *csound, void *userData,
                            unsigned char *mbuf, int32_t nbytes)
{
  IGN(csound);
  cdata *data = (cdata *)userData;
  MIDIdata *mdata = data->mdata;
  int32_t *q = &data->q, st, d1, d2, n = 0;

  /* check if there is new data in circular queue */
  while (mdata[*q].flag) {
    st = (int32_t) mdata[*q].status;
    d1 = (int32_t) mdata[*q].data1;
    d2 = (int32_t) mdata[*q].data2;

    if (st < 0x80) goto next;

    if (st >= 0xF0 &&
        !(st == 0xF8 || st == 0xFA || st == 0xFB ||
          st == 0xFC || st == 0xFF)) goto next;

    nbytes -= (datbyts[(st - 0x80) >> 4] + 1);
    if (nbytes < 0) break;

    /* write to csound midi buffer */
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
    /* mark as read */
  next:
    mdata[*q].flag = 0;
    (*q)++;
    if (*q==DSIZE) *q = 0;

  }

  /* return the number of bytes read */
  return n;
}

/* csound close device callback */
static int32_t MidiInDeviceClose(CSOUND *csound, void *userData)
{
  cdata * data = (cdata *)userData;
  if (data != NULL) {
    MIDIClientDispose(data->mclient);
    csound->Free(csound, data->mdata);
    csound->Free(csound, data);
  }
  return 0;
}

static int32_t MidiDataWrite(CSOUND *csound, void *userData,
                             const unsigned char *mbuf, int32_t nbytes)
{
  odata *data = (odata *)userData;
  int32_t i, pos, n = 0, mess_port;
  UInt32 bufsize;
  Byte *buf;
  MIDIPacketList *pktlist;
  MIDIPacket *packet;

  if (data == NULL || data->mport == (MIDIPortRef) 0 ||
      data->ndest < 1 || nbytes < 1)
    return nbytes;

  /* port of the message, used for multiport mapping */
  mess_port = csound->GetMidiOutPort(csound);

  /* build a packet list, not splitting MIDI messages across packets */
  bufsize = (UInt32) (sizeof(MIDIPacketList) + (size_t) nbytes +
                      ((size_t) nbytes / 256 + 1) * sizeof(MIDIPacket));
  buf = (Byte *) csound->Malloc(csound, bufsize);
  if (UNLIKELY(buf == NULL))
    return 0;
  pktlist = (MIDIPacketList *) buf;
  packet = MIDIPacketListInit(pktlist);

  pos = 0;
  while (pos < nbytes) {
    int32_t len = 0, avail = nbytes - pos;
    while (len < avail && len < 256) {
      int32_t st = (int32_t) mbuf[pos + len];
      int32_t msglen = (st >= 0x80) ? datbyts[(st - 0x80) >> 4] + 1 : 1;
      if (len + msglen > 256) break;
      len += msglen;
    }
    if (len == 0) len = (avail > 256 ? 256 : avail);
    packet = MIDIPacketListAdd(pktlist, bufsize, packet, 0,
                               (ByteCount) len, (const Byte *) (mbuf + pos));
    if (UNLIKELY(packet == NULL)) {
      csound->Free(csound, buf);
      return n;
    }
    pos += len;
  }

  for (i = 0; i < data->ndest; i++) {
    if (data->multiport && mess_port != i)
      continue;
    MIDISend(data->mport, data->dest[i], pktlist);
  }
  csound->Free(csound, buf);
  n = nbytes;

  /* return the number of bytes written */
  return n;
}



static int32_t MidiOutDeviceClose(CSOUND *csound, void *userData)
{
  odata *data = (odata *)userData;
  if (data != NULL) {
    if (data->mport) MIDIPortDispose(data->mport);
    if (data->mclient) MIDIClientDispose(data->mclient);
    if (data->dest) csound->Free(csound, data->dest);
    csound->Free(csound, data);
  }
  return 0;
}

/* module interface functions */

 int32_t csoundModuleCreate(CSOUND *csound)
{
  /* nothing to do, report success */
  //csound->Message(csound, "%s",
  //                Str("CoreMIDI real time MIDI plugin for Csound\n"));
  IGN(csound);
  return 0;
}

 int32_t csoundModuleInit(CSOUND *csound)
{
  char    *drv;
  csound->ModuleListAdd(csound, "coremidi", "midi");
  drv = (char*) (csound->QueryGlobalVariable(csound, "_RTMIDI"));
  if (drv == NULL)
    return 0;
  if (!(strcmp(drv, "coremidi") == 0 || strcmp(drv, "CoreMidi") == 0 ||
        strcmp(drv, "CoreMIDI") == 0 || strcmp(drv, "cm") == 0))
    return 0;
  csound->DebugMsg(csound, "%s\n", Str("rtmidi: CoreMIDI module enabled\n"));
  csound->SetExternalMidiInOpenCallback(csound, MidiInDeviceOpen);
  csound->SetExternalMidiReadCallback(csound, MidiDataRead);
  csound->SetExternalMidiInCloseCallback(csound, MidiInDeviceClose);
  csound->SetExternalMidiOutOpenCallback(csound, MidiOutDeviceOpen);
  csound->SetExternalMidiWriteCallback(csound, MidiDataWrite);
  csound->SetExternalMidiOutCloseCallback(csound, MidiOutDeviceClose);
  csound->SetMIDIDeviceListCallback(csound,listDevices);
  return 0;
}

 int32_t csoundModuleInfo(void)
{
  return CSOUND_MODULE_INFO;
}
