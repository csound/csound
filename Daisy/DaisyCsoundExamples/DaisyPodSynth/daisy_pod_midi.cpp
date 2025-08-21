/*
  daisy_pod_midi.cpp:

  Copyright (C) 2025 V Lazzarini
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

#include "daisy_pod.h"
#include "daisysp.h"
#include <stdio.h>
#include "daisy_pod_midi.h"
#include "midiBuffer.h"
#include <csound.h>
#include <plugin.h>
#include <string>
#include <vector>

using namespace daisy;

static void set_audio_midi_io(CSOUND *csound);
std::vector<uint8_t> ConvertMidiEventToBytes(const MidiEvent &event);

static CSOUND *csound;
struct PodSynth {
  DaisyPod hw;
  MidiUsbHandler midi;
  MidiBuffer midi_buffer;
  Parameter pot1, pot2;
  bool toggle1, toggle2, encoder_toggle;
  int encoder_value;
};

void AudioCallback(AudioHandle::InterleavingInputBuffer in,
		   AudioHandle::InterleavingOutputBuffer out,
                   size_t size) {
  const int max_encoder = 10000;
  auto synth = static_cast<PodSynth *>(csoundGetHostData(csound));
  const MYFLT *spout = csoundGetSpout(csound);
  int32_t res;

  synth->hw.ProcessDigitalControls();

  if(synth->hw.button1.RisingEdge())
    synth->toggle1 = !synth->toggle1;
  if(synth->hw.button2.RisingEdge())
    synth->toggle2 = !synth->toggle2;
  if(synth->hw.encoder.RisingEdge())
    synth->encoder_toggle = !synth->encoder_toggle;
  synth->encoder_value += synth->hw.encoder.Increment(); 

  csoundSetControlChannel(csound, "toggle1", synth->toggle1 ? 1.f : 0.f);
  csoundSetControlChannel(csound, "toggle2", synth->toggle2 ? 1.f : 0.f);
  csoundSetControlChannel(csound, "etoggle", synth->toggle2 ? 1.f : 0.f);
  csoundSetControlChannel(csound, "pressed1",
			  synth->hw.button1.Pressed() ? 1.f : 0.f);
  csoundSetControlChannel(csound, "pressed2",
			  synth->hw.button2.Pressed() ? 1.f : 0.f);
  csoundSetControlChannel(csound, "epressed",
			  synth->hw.encoder.Pressed() ? 1.f : 0.f);
  csoundSetControlChannel(csound, "encoder", (MYFLT)
			  DSY_CLAMP(synth->encoder_value, 0, max_encoder)
			  /max_encoder);
  csoundSetControlChannel(csound, "pot1", synth->pot1.Process());
  csoundSetControlChannel(csound, "pot2", synth->pot2.Process());
  
  res = csoundPerformKsmps(csound);
  if(res == 0) memcpy(out, spout, sizeof(MYFLT)*size);
  else memset(out, 0, sizeof(MYFLT)*size);

  synth->hw.led1.Set(synth->toggle1 ? 1.f : 0.f, 0, 0);
  synth->hw.led2.Set(synth->toggle2 ? 1.f : 0.f, 0, 0);
  synth->hw.UpdateLeds();
}

int main() {
  
  PodSynth synth;
  
  synth.hw.Init();
  synth.pot1.Init(synth.hw.knob1,0.f,1.f,Parameter::LINEAR);
  synth.pot2.Init(synth.hw.knob2,0.f,1.f,Parameter::LINEAR);
  
  csound = csoundCreate(&synth, NULL);
  if(csound) {
    set_audio_midi_io(csound);
    csoundSetOption(csound, "-n -M0 -dm0");
    if(csoundCompileCSD(csound, csd_text.c_str(), 1, 0) == 0){
      if(csoundStart(csound) == 0) {
	
	MidiUsbHandler::Config midi_cfg;
	midi_cfg.transport_config.periph = MidiUsbTransport::Config::INTERNAL;
	synth.midi.Init(midi_cfg);
	synth.hw.SetAudioBlockSize(csoundGetKsmps(csound));
	synth.hw.SetAudioSampleRate((SaiHandle::Config::SampleRate)
			      csoundGetSr(csound));
	synth.hw.StartAdc();
	synth.hw.StartAudio(AudioCallback);

	synth.hw.led1.Set(0, 0, 0);
        synth.hw.led2.Set(0, 0, 0);
        synth.hw.UpdateLeds();
	
	while(1){
	  synth.midi.Listen();
	  while(synth.midi.HasEvents()) {
	    auto msg = synth.midi.PopEvent();
	    auto rawBytes = ConvertMidiEventToBytes(msg);
	    synth.midi_buffer.write(rawBytes);
	  }
	}
      }
    }
    csoundDestroy(csound);
  }   
  return 0;
}

int32_t close_midi_device(CSOUND *csound, void *userData){
  return 0;
}


int32_t open_midi_device(CSOUND *csound, void **userData, const char *dev){
  *userData = (void *) csoundGetHostData(csound);
  return 0;
}


int32_t read_midi_data(CSOUND *csound, void *userData, unsigned char *mbuf,
		       int32_t nbytes) {
  auto synth = static_cast<PodSynth *>(userData);
  if(synth->midi_buffer.isAvailable){
    return synth->midi_buffer.read(mbuf, nbytes);
  }
  return 0;
}

void set_audio_midi_io(CSOUND *csound) {
  csoundSetHostAudioIO(csound);
  csoundSetHostMIDIIO(csound);
  csoundSetExternalMidiInOpenCallback(csound, open_midi_device);
  csoundSetExternalMidiReadCallback(csound, read_midi_data);
  csoundSetExternalMidiInCloseCallback(csound, close_midi_device);
}

std::vector<uint8_t> ConvertMidiEventToBytes(const MidiEvent &event) {
  std::vector<uint8_t> rawBytes;
  uint8_t statusByte = 0;

  switch(event.type) {
  case NoteOff: statusByte = 0x80; break;
  case NoteOn: statusByte = 0x90; break;
  case PolyphonicKeyPressure: statusByte = 0xA0; break;
  case ControlChange: statusByte = 0xB0; break;
  case ProgramChange: statusByte = 0xC0; break;
  case ChannelPressure: statusByte = 0xD0; break;
  case PitchBend: statusByte = 0xE0; break;
  default: statusByte = 0; break;
  }

  statusByte |= (event.channel & 0x0F);
  rawBytes.push_back(statusByte);
  rawBytes.push_back(event.data[0]);

  if(event.type != ProgramChange && event.type != ChannelPressure) {
    rawBytes.push_back(event.data[1]);
  }

  return rawBytes;
}
