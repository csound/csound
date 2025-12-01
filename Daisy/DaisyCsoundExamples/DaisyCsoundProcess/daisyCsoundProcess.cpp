/*
  daisyCsoundProcess.cpp:

  Copyright (C) 2025 Aman Jagawni

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
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA, 02110-1335, USA
*/

#include "daisy_seed.h"
#include "daisysp.h"
#include <array>
#include <stdio.h>
#include "daisyCsoundProcess.h"
#include "csound.h"
#include "plugin.h"


using namespace daisy;
using namespace daisy::seed;

// this function can be used to provide a message callback
// for csound
/* static void DaisyCsoundMessageCallback(CSOUND     *csound,
                                       int         attr,
                                       const char *format,
                                       va_list     args);
*/


DaisySeed hw;
int       cnt = 0;
#define SR 48000
const int numAdcChannels = 12;
CSOUND   *csound;
Pin       adcPins[numAdcChannels]
    = {A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11};
float       adcVals[12]                         = {0};
const char *controlChannelNames[numAdcChannels] = {"AnalogIn0",
                                                   "AnalogIn1",
                                                   "AnalogIn2",
                                                   "AnalogIn3",
                                                   "AnalogIn4",
                                                   "AnalogIn5",
                                                   "AnalogIn6",
                                                   "AnalogIn7",
                                                   "AnalogIn8",
                                                   "AnalogIn9",
                                                   "AnalogIn10",
                                                   "AnalogIn11"};

struct DigiInHandler
{
    static const int numDigiChannels = 15;
    Pin              digiPins[numDigiChannels]
        = {D27, D1, D2, D3, D4, D5, D6, D7, D8, D9, D10, D11, D12, D13, D14};
    bool       digiPinActive[numDigiChannels] = {false};
    int        digiVals[numDigiChannels]      = {0};
    GPIO       gpios[numDigiChannels];
    GPIO::Pull digiPullModes[numDigiChannels];


    DigiInHandler()
    {
        for(int i = 0; i < numDigiChannels; ++i)
        {
            digiPullModes[i] = GPIO::Pull::NOPULL;
        }
    }

    void initDigiPins()
    {
        for(int i = 0; i < numDigiChannels; ++i)
        {
            gpios[i].Init(digiPins[i], GPIO::Mode::INPUT, GPIO::Pull::PULLUP);
        }
    }

    void readDigiPins()
    {
        for(int i = 0; i < numDigiChannels; ++i)
        {
            if(digiPinActive[i])
            {
                digiVals[i] = gpios[i].Read();
            }
        }
    }
};

static DigiInHandler digiHandler;

std::vector<uint8_t> ConvertMidiEventToBytes(const MidiEvent &event)
{
    std::vector<uint8_t> rawBytes;
    uint8_t              statusByte = 0;

    switch(event.type)
    {
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

    if(event.type != ProgramChange && event.type != ChannelPressure)
    {
        rawBytes.push_back(event.data[1]);
    }

    return rawBytes;
}


struct DigiIn : csnd::Plugin<1, 2>
{
    DigiInHandler *handler;
    int            pinNumber;
    bool           initDone;

    int init()
    {
        handler                           = (DigiInHandler *)&digiHandler;
        pinNumber                         = (int)inargs[0];
        pinNumber                         = (pinNumber < 0) ? 0
                                                            : (pinNumber > handler->numDigiChannels - 1
                                                                   ? handler->numDigiChannels - 1
                                                                   : pinNumber);
        handler->digiPinActive[pinNumber] = true;
        GPIO::Pull pullMode               = (GPIO::Pull)inargs[1];
        handler->digiPullModes[pinNumber] = pullMode;
        initDone                          = true;
        return OK;
    }

    int kperf()
    {
        if(!initDone)
        {
            return NOTOK;
        }
        outargs[0] = (MYFLT)handler->digiVals[pinNumber];
        return OK;
    }
};

void AudioCallback(AudioHandle::InterleavingInputBuffer  in,
                   AudioHandle::InterleavingOutputBuffer out,
                   size_t                                size)
{
    const MYFLT *spout = csoundGetSpout(csound);
    MYFLT       *spin  = csoundGetSpin(csound);

    memcpy(spin, in, size * sizeof(MYFLT));
    csoundPerformKsmps(csound);
    memcpy(out, spout, size * sizeof(MYFLT));
}


int main(void)
{
    hw.Configure();
    hw.Init();
    // hw.StartLog();

    CSOUND *cs = csoundCreate(NULL, NULL);
    // csoundSetMessageCallback(cs, DaisyCsoundMessageCallback);
    csoundSetHostData(cs, (void *)&hw);
    csoundSetHostAudioIO(cs);

    if(csnd::plugin<DigiIn>(
           (csnd::Csound *)cs, "digiInDaisy", "k", "ii", csnd::thread::ik)
       != 0)
        hw.PrintLine("Warning: could not add digiInDaisy k-rate opcode\n");

    AdcChannelConfig adcConfig[numAdcChannels];
    for(int i = 0; i < numAdcChannels; i++)
    {
        adcConfig[i].InitSingle(adcPins[i]);
    }

    if(cs)
    {
        csound = cs;
        csoundSetOption(cs, "-n");
        csoundSetOption(cs, "--ksmps=256");
        csoundSetOption(cs, "-dm0");

        int ret = csoundCompileCSD(cs, csdText.c_str(), 1, 0);

        if(ret == 0)
        {
            csoundStart(cs);
            hw.adc.Init(adcConfig, numAdcChannels);
            hw.adc.Start();

            hw.SetAudioBlockSize(256);
            hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);
            hw.StartAudio(AudioCallback);
            digiHandler.initDigiPins();
            while(1)
            {
                for(int i = 0; i < numAdcChannels; i++)
                {
                    adcVals[i] = hw.adc.GetFloat(i);
                }
                digiHandler.readDigiPins();

                for(int i = 0; i < numAdcChannels; i++)
                {
                    csoundSetControlChannel(
                        csound, controlChannelNames[i], adcVals[i]);
                }
            }
            csoundReset(cs);
        }
        else
        {
            // hw.PrintLine("Error: could not compile csd. \n");
        }
    }
    else
    {
        //  hw.PrintLine("Error: csoundCreate failed.\n");
        return 1;
    }
    return 0;
}


/*
// message callback

constexpr size_t kMessageBufferSize = 64;
static void DaisyCsoundMessageCallback(CSOUND     *csound,
                                       int         attr,
                                       const char *format,
                                       va_list     args)
{
    char messageBuffer[kMessageBufferSize];
    vsnprintf(messageBuffer, kMessageBufferSize, format, args);


    hw.PrintLine("%s", messageBuffer);
}
*/