/*
    BelaCsound.cpp:

    Copyright (C) 2017 V Lazzarini

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
#include <Bela.h>
#include <Midi.h>
#include <csound/csound.hpp>
#include <vector>
#include <sstream>

#define ANCHNS 8

static int OpenMidiInDevice(CSOUND *csound, void **userData, const char *dev);
static int CloseMidiInDevice(CSOUND *csound, void *userData);
static int ReadMidiData(CSOUND *csound, void *userData, unsigned char *mbuf,
			int nbytes);

struct CsChan {
  std::vector<MYFLT> data;
  std::stringstream name;
};

struct CsData {
  Csound *csound;
  int blocksize;
  int res;
  int count;
  CsChan channel[ANCHNS];
};
  
static CsData gCsData;

bool setup(BelaContext *context, void *Data)
{
  Csound *csound;
  const char *csdfile = "my.csd"; /* CSD name */
  const char *midiDev = "-Mhw:1,0,0"; /* MIDI device */
  const char *args[] = { "csound", csdfile, "-iadc", "-odac", "-+rtaudio=null",
			 "--realtime", "--daemon", midiDev };
  int numArgs = (int) (sizeof(args)/sizeof(char *));

  if(context->audioInChannels != context->audioOutChannels) {
    printf("Error: number of audio inputs != number of audio outputs.\n");
    return false;
  }

  /* setup Csound */
  csound = new Csound();
  gCsData.csound = csound;
  csound->SetHostImplementedAudioIO(1,0);
  csound->SetHostImplementedMIDIIO(1);
  csound->SetExternalMidiInOpenCallback(OpenMidiInDevice);
  csound->SetExternalMidiReadCallback(ReadMidiData);
  csound->SetExternalMidiInCloseCallback(CloseMidiInDevice);
  if((gCsData.res = csound->Compile(numArgs, args)) != 0) {
    printf("Error: Csound could not compile CSD file.\n");
    return false;
  }
  gCsData.blocksize = csound->GetKsmps()*csound->GetNchnls();
  gCsData.count = 0;
  
  /* set up the channels */
  for(int i; i < ANCHNS; i++) {
    gCsData.channel[i].data.resize(csound->GetKsmps());
    gCsData.channel[i].name << "analogue" << i+1;
  }
  
  return true;
}

void render(BelaContext *context, void *Data)
{
  if(gCsData.res == 0) {
    int n,i,k,count, frmcount,blocksize,res;
    Csound *csound = gCsData.csound;
    MYFLT scal = csound->Get0dBFS();
    MYFLT* audioIn = csound->GetSpin();
    MYFLT* audioOut = csound->GetSpout();
    int nchnls = csound->GetNchnls();
    int chns = nchnls < context->audioOutChannels ?
      nchnls : context->audioOutChannels;
    int an_chns = context->analogInChannels > ANCHNS ?
      ANCHNS : context->analogInChannels;
    CsChan *channel = &(gCsData.channel[0]);
    float frm = 0.f, incr = ((float) context->analogFrames)/context->audioFrames;
    count = gCsData.count;
    blocksize = gCsData.blocksize;
      
    /* processing loop */
    for(n = 0; n < context->audioFrames; n++, frm+=incr, count+=nchnls){
      if(count == blocksize) {
	/* set the channels */
	for(i = 0; i < an_chns; i++) {
          csound->SetChannel(channel[i].name.str().c_str(),
			     &(channel[i].data[0]));
	}
	/* run csound */
	if((res = csound->PerformKsmps()) == 0) count = 0;
	else break;
      }
      /* read/write audio data */
      for(i = 0; i < chns; i++){
	audioIn[count+i] = audioRead(context,n,i);
	audioWrite(context,n,i,audioOut[count+i]/scal);
      }
      /* read analogue data 
         analogue frame pos frm gets incremented according to the
         ratio analogFrames/audioFrames.
      */
      frmcount = count/nchnls;
      for(i = 0; i < an_chns; i++) {
	k = (int) frm;
        channel[i].data[frmcount] = analogRead(context,k,i);
      }	
    }
    gCsData.res = res;
    gCsData.count = count;
  }
}

void cleanup(BelaContext *context, void *Data)
{
  delete gCsData.csound;
}

/** MIDI Input functions 
 */
int OpenMidiInDevice(CSOUND *csound, void **userData, const char *dev) {
  Midi *midi = new Midi;
  if(midi->readFrom(dev) == 1) {
    midi->enableParser(false);
    *userData = (void *) midi;
    return 0;
  }
  csound->Warning(csound, "Could not open Midi device %s", dev);
  delete Midi;
  return -1;
}

int CloseMidiInDevice(CSOUND *csound, void *userData) {
  if(userData) delete (Midi *) userData;
}

int ReadMidiData(CSOUND *csound, void *userData,
		 unsigned char *mbuf, int nbytes) {
  int n = 0, byte;
  if(userData) {
  Midi *midi = (Midi *) userData;
  
  while((byte = midi->getInput()) >= 0) {
    *mbuf++ = (unsigned char) byte;
    if(++n == nbytes) break;
  }
  return n;
  }
  return 0;
}
