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
void AudioCallback(AudioHandle::InterleavingInputBuffer in,
		   AudioHandle::InterleavingOutputBuffer out,
                   size_t size) {
  const MYFLT *spout = csoundGetSpout(csound);
  int32_t res;  
  res = csoundPerformKsmps(csound);
  if(res == 0) memcpy(out, spout, sizeof(MYFLT)*size);
  else memset(out, 0, sizeof(MYFLT)*size);
}

int main() {
  
  DaisyPod hw;
  MidiUsbHandler midi_usb;
  MidiUartHandler midi_uart;
  MidiBuffer midi_buffer;
  bool toggle1, toggle2, encoder_toggle;
  int encoder_value;
  const int max_encoder = 10000;
  
  hw.Init();
  toggle1 = toggle2 = encoder_toggle = false;
  encoder_value = 0;
  
  csound = csoundCreate(&midi_buffer, NULL);
  if(csound) {
    set_audio_midi_io(csound);
    csoundSetOption(csound, "-n -M0 -dm0");
    if(csoundCompileCSD(csound, csd_text.c_str(), 1, 0) == 0){
      if(csoundStart(csound) == 0) {
	
	MidiUsbHandler::Config midi_usb_cfg;
	MidiUartHandler::Config midi_uart_cfg;
	midi_usb_cfg.transport_config.periph = MidiUsbTransport::Config::INTERNAL;
	midi_usb.Init(midi_usb_cfg);
	midi_uart.Init(midi_uart_cfg);
	midi_uart.StartReceive();
	hw.SetAudioBlockSize(csoundGetKsmps(csound));
	hw.SetAudioSampleRate((SaiHandle::Config::SampleRate)
			      csoundGetSr(csound));
	hw.StartAdc();
	hw.StartAudio(AudioCallback);

	while(1){
	  midi_usb.Listen();
	  midi_uart.Listen();
	  
	  while(midi_usb.HasEvents()) {
	    auto msg = midi_usb.PopEvent();
	    auto rawBytes = ConvertMidiEventToBytes(msg);
	    midi_buffer.write(rawBytes);
	  }

	 while(midi_uart.HasEvents()) {
	    auto msg = midi_uart.PopEvent();
	    auto rawBytes = ConvertMidiEventToBytes(msg);
	    midi_buffer.write(rawBytes);
	  }

	  hw.ProcessDigitalControls();
	  if(hw.button1.RisingEdge())
	    toggle1 = !toggle1;
	  if(hw.button2.RisingEdge())
	    toggle2 = !toggle2;
	  if(hw.encoder.RisingEdge())
	    encoder_toggle = !encoder_toggle;
	  encoder_value += hw.encoder.Increment();
	  
	  csoundSetControlChannel(csound, "toggle1", toggle1 ? 1.f : 0.f);
	  csoundSetControlChannel(csound, "toggle2", toggle2 ? 1.f : 0.f);
	  csoundSetControlChannel(csound, "etoggle", encoder_toggle ?
				  1.f : 0.f);
	  csoundSetControlChannel(csound, "pressed1",
				  hw.button1.Pressed() ? 1.f : 0.f);
	  csoundSetControlChannel(csound, "pressed2",
				  hw.button2.Pressed() ? 1.f : 0.f);
	  csoundSetControlChannel(csound, "epressed",
				  hw.encoder.Pressed() ? 1.f : 0.f);
	  csoundSetControlChannel(csound, "encoder", (MYFLT)
				  DSY_CLAMP(encoder_value, 0, max_encoder)
				  /max_encoder);
	  csoundSetControlChannel(csound, "pot1", hw.knob1.GetRawFloat());
	  csoundSetControlChannel(csound, "pot2", hw.knob2.GetRawFloat());

	  hw.led1.Set(toggle1 ? 1 : 0, 0, 0);
          hw.led2.Set(0, 0, toggle2 ? 1 : 0);
	  hw.seed.SetLed(encoder_toggle);
          hw.UpdateLeds(); 
	}
      }
    }
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
  auto midi_buffer = static_cast<MidiBuffer *>(userData);
  if(midi_buffer->isAvailable){
    return midi_buffer->read(mbuf, nbytes);
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
