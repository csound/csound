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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
    02111-1307 USA
*/

/* Realtime MIDI using coremidi */


#include <CoreMIDI/CoreMIDI.h>
#include <CoreAudio/HostTime.h>
#include <CoreFoundation/CoreFoundation.h>
#include "csdl.h"                               /*      CMIDI.C         */
#include "csGblMtx.h"
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

/* user data for MIDI callbacks */
typedef struct _cdata {
  MIDIdata *mdata;
  int p; int q;
  MIDIClientRef mclient;
} cdata;


/* coremidi callback, called when MIDI data is available */
void ReadProc(const MIDIPacketList *pktlist, void *refcon, void *srcConnRefCon)
{
    cdata *data = (cdata *)refcon;
    MIDIdata *mdata = data->mdata;
    int *p = &data->p, i, j;
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

static int listDevices(CSOUND *csound, CS_MIDIDEVICE *list, int isOutput)
{
    int k, endpoints;
    MIDIEndpointRef endpoint;
    CFStringRef name = NULL;
    CFStringEncoding defaultEncoding = CFStringGetSystemEncoding();
    char tmp[64];
    char *drv = (char*) (csound->QueryGlobalVariable(csound, "_RTMIDI"));
    if (isOutput) return 0;
    endpoints = MIDIGetNumberOfSources();
    if (list == NULL) return endpoints;
    for(k=0; k < endpoints; k++) {
      endpoint = MIDIGetSource(k);
      MIDIObjectGetStringProperty(endpoint, kMIDIPropertyName, &name);
      char *sn = CFStringGetCStringPtr(name, defaultEncoding);
      if(sn != NULL)
       strncpy(list[k].device_name, sn, 63);
      snprintf(tmp, 64, "%d", k);
      strncpy(list[k].device_id, tmp, 63);
      list[k].isOutput = isOutput;
      strcpy(list[k].interface_name, "");
      strncpy(list[k].midi_module, drv, 63);
    }
    if (name) CFRelease(name);
    return endpoints;
}


/* csound MIDI input open callback, sets the device for input */
static int MidiInDeviceOpen(CSOUND *csound, void **userData, const char *dev)
{
    int k, endpoints, dest;
    CFStringRef name = NULL, cname = NULL, pname = NULL;
    CFStringEncoding defaultEncoding = CFStringGetSystemEncoding();
    MIDIClientRef mclient = (MIDIClientRef) 0;
    MIDIPortRef mport =  (MIDIPortRef) 0;
    MIDIEndpointRef endpoint;
    MIDIdata *mdata = (MIDIdata *) malloc(DSIZE*sizeof(MIDIdata));
    OSStatus ret;
    cdata *refcon = (cdata *) malloc(sizeof(cdata));
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
        endpoints = MIDIGetNumberOfSources();
        csound->Message(csound, Str("%d MIDI sources in system \n"), endpoints);
        if (!strcmp(dev,"all")) {
          csound->Message(csound, Str("receiving from all sources \n"));
          for(k=0; k < endpoints; k++){
            endpoint = MIDIGetSource(k);
            long srcRefCon = (long) endpoint;
            MIDIPortConnectSource(mport, endpoint, (void *) srcRefCon);
            MIDIObjectGetStringProperty(endpoint, kMIDIPropertyName, &name);
            csound->Message(csound, Str("connecting midi device %d: %s \n"), k,
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
            csound->Message(csound, Str("connecting midi device %d: %s \n"), k,
                            CFStringGetCStringPtr(name, defaultEncoding));
          }
          else
            csound->Message(csound,
                            Str("MIDI device number %d is out-of-range, "
                                "not connected \n"), k);
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

static int MidiOutDeviceOpen(CSOUND *csound, void **userData, const char *dev)
{
    /*stub for the moment */
    csound->Message(csound, Str("output not implemented yet in CoreMIDI \n"));
    return 0;
}

/* used to distinguish between 1 and 2-byte messages */
static  const   int     datbyts[8] = { 2, 2, 2, 2, 1, 1, 2, 0 };

/* csound MIDI read callback, called every k-cycle */
static int MidiDataRead(CSOUND *csound, void *userData,
                         unsigned char *mbuf, int nbytes)
{
    cdata *data = (cdata *)userData;
    MIDIdata *mdata = data->mdata;
    int *q = &data->q, st, d1, d2, n = 0;

    /* check if there is new data in circular queue */
    while (mdata[*q].flag) {
      st = (int) mdata[*q].status;
      d1 = (int) mdata[*q].data1;
      d2 = (int) mdata[*q].data2;

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
static int MidiInDeviceClose(CSOUND *csound, void *userData)
{
    cdata * data = (cdata *)userData;
    if (data != NULL) {
      MIDIClientDispose(data->mclient);
      free(data->mdata);
      free(data);
    }
    return 0;
}

static int MidiDataWrite(CSOUND *csound, void *userData,
                          const unsigned char *mbuf, int nbytes)
{
    /* stub at the moment */
    /*
    */
    return nbytes;
}



static int MidiOutDeviceClose(CSOUND *csound, void *userData)
{
    /* stub at the mement */
    return 0;
}

/* module interface functions */

PUBLIC int csoundModuleCreate(CSOUND *csound)
{
    /* nothing to do, report success */
    //csound->Message(csound, Str("CoreMIDI real time MIDI plugin for Csound\n"));
    return 0;
}

PUBLIC int csoundModuleInit(CSOUND *csound)
{
    char    *drv;
    csound->module_list_add(csound, "coremidi", "midi");
    drv = (char*) (csound->QueryGlobalVariable(csound, "_RTMIDI"));
    if (drv == NULL)
      return 0;
    if (!(strcmp(drv, "coremidi") == 0 || strcmp(drv, "CoreMidi") == 0 ||
          strcmp(drv, "CoreMIDI") == 0 || strcmp(drv, "cm") == 0))
      return 0;
    csound->Message(csound, Str("rtmidi: CoreMIDI module enabled\n"));
    csound->SetExternalMidiInOpenCallback(csound, MidiInDeviceOpen);
    csound->SetExternalMidiReadCallback(csound, MidiDataRead);
    csound->SetExternalMidiInCloseCallback(csound, MidiInDeviceClose);
    csound->SetExternalMidiOutOpenCallback(csound, MidiOutDeviceOpen);
    csound->SetExternalMidiWriteCallback(csound, MidiDataWrite);
    csound->SetExternalMidiOutCloseCallback(csound, MidiOutDeviceClose);
    csound->SetMIDIDeviceListCallback(csound,listDevices);
    return 0;
}

PUBLIC int csoundModuleInfo(void)
{
    /* does not depend on MYFLT type */
    return ((CS_APIVERSION << 16) + (CS_APISUBVER << 8));
}
