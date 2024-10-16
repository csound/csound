#include "daisy_seed.h"
#include "daisysp.h"
#include <array>
#include <stdio.h>
#include "daisyCsound.h"
#include "midiBuffer.h"
#include "csound.h"
#include "plugin.h"

using namespace daisy;
using namespace daisy::seed;


static int32_t OpenMidiInDevice(CSOUND *csound, void **userData, const char *dev);
static int32_t CloseMidiInDevice(CSOUND *csound, void *userData);
static int32_t ReadMidiData(CSOUND *csound, void *userData, unsigned char *mbuf, int32_t nbytes);
static int32_t  OpenMidiOutDevice(CSOUND *csound, void **userData, const char *dev);
static int32_t  CloseMidiOutDevice(CSOUND *csound, void *userData);
static int32_t  WriteMidiData(CSOUND              *csound,
                          void                *userData,
                          const unsigned char *mbuf,
                          int                  nbytes);
static void DaisyCsoundMessageCallback(CSOUND     *csound,
                                       int         attr,
                                       const char *format,
                                       va_list     args);


DaisySeed      hw;
MidiUsbHandler midi;
MidiBuffer     midiBuffer;
int            cnt = 0;
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
        for(int i = 0; i < numDigiChannels; i++)
        {
            digiPullModes[i] = GPIO::Pull::NOPULL;
        }
    }

    void initDigiPins()
    {
        for(int i = 0; i < numDigiChannels; i++)
        {
            gpios[i].Init(digiPins[i], GPIO::Mode::INPUT, GPIO::Pull::PULLUP);
        }
    }

    void readDigiPins()
    {
        for(int i = 0; i < numDigiChannels; i++)
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


void AudioCallback(AudioHandle::InputBuffer  in,
                   AudioHandle::OutputBuffer out,
                   size_t                    size)
{
    const MYFLT *spout = csoundGetSpout(csound);
    int    end   = csoundGetKsmps(csound);
    for(size_t i = 0; i < size; i++)
    {
        if(cnt == 0)
        {
            csoundPerformKsmps(csound);
        }
        out[0][i] = spout[cnt] * 0.5f;
        out[1][i] = spout[cnt + 1] * 0.5f;
        //cnt = cnt != end - 1 ? cnt + 1 : 0; // for mono out
        cnt = (cnt + 2) % (end * 2); // for stereo out
    }
}


int main(void)
{
    hw.Configure();
    hw.Init();
    // hw.StartLog();



    System::Delay(5000);
    CSOUND *cs = csoundCreate(NULL, NULL);
    //csoundSetMessageCallback(cs, DaisyCsoundMessageCallback);
    csoundSetHostData(cs, (void *)&hw);
    csoundSetHostAudioIO(cs);
    csoundSetHostMIDIIO(cs);
    csoundSetExternalMidiInOpenCallback(cs, OpenMidiInDevice);
    csoundSetExternalMidiReadCallback(cs, ReadMidiData);
    csoundSetExternalMidiInCloseCallback(cs, CloseMidiInDevice);
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
        csoundSetOption(cs, "--ksmps=128");
        csoundSetOption(cs, "-M0");
        csoundSetOption(cs, "-dm0");

        int ret = csoundCompileCSD(cs, csdText.c_str(), 1);

        if(ret == 0)
        {
            csoundStart(cs);
            hw.adc.Init(adcConfig, numAdcChannels);
            hw.adc.Start();

            MidiUsbHandler::Config midi_cfg;
            midi_cfg.transport_config.periph
                = MidiUsbTransport::Config::INTERNAL;
            midi.Init(midi_cfg);

            hw.SetAudioBlockSize(32);
            hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);
            hw.StartAudio(AudioCallback);
            digiHandler.initDigiPins();
            while(1)
            {
                midi.Listen();
                while(midi.HasEvents())
                {
                    auto msg      = midi.PopEvent();
                    auto rawBytes = ConvertMidiEventToBytes(msg);
                    midiBuffer.write(rawBytes);
                }

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


int32_t CloseMidiInDevice(CSOUND *csound, void *userData)
{
    return 0;
}


int32_t OpenMidiInDevice(CSOUND *csound, void **userData, const char *dev)
{
    *userData = (void *)&midiBuffer;
    return 0;
}


int32_t ReadMidiData(CSOUND        *csound,
                 void          *userData,
                 unsigned char *mbuf,
                 int32_t            nbytes)
{
    auto buffer = static_cast<MidiBuffer *>(userData);
    if(buffer->isAvailable)
    {
        return buffer->read(mbuf, nbytes);
    }
    return 0;
}


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
