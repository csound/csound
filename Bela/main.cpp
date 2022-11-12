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

#include <getopt.h>
#include <Bela.h>
#include <libraries/Midi/Midi.h>
#include <libraries/Scope/Scope.h>
#include <libraries/Trill/Trill.h>
#include <csound/csound.hpp>
#include <csound/plugin.h>
#include <vector>
#include <sstream>
#include <iostream>
#include <atomic>


static int OpenMidiInDevice(CSOUND* csound, void** userData, const char* dev);
static int CloseMidiInDevice(CSOUND* csound, void* userData);
static int ReadMidiData(CSOUND* csound, void* userData, unsigned char* mbuf, int nbytes);
static int OpenMidiOutDevice(CSOUND* csound, void** userData, const char* dev);
static int CloseMidiOutDevice(CSOUND* csound, void* userData);
static int WriteMidiData(CSOUND* csound, void* userData, const unsigned char* mbuf, int nbytes);

std::vector<Trill*> gTouchSensors;

/** DigiIn opcode
    ksig digiInBela ipin
    asig digiInBela ipin
*/
struct DigiIn : csnd::Plugin<1, 1> {
    int pin;
    int fcount;
    int frms;
    int init_done;
    BelaContext* context;

    int init() {
        pin = (int)inargs[0];
        if (pin < 0)
            pin = 0;
        if (pin > 15)
            pin = 15;
        context = (BelaContext*)csound->host_data();
        fcount = 0;
        init_done = 0;
        frms = context->digitalFrames;
        return OK;
    }

    int kperf() {
        if (!init_done) {
            pinMode(context, 0, pin, 0);
            init_done = 1;
        }
        outargs[0] = (MYFLT)digitalRead(context, fcount, pin);
        fcount += nsmps;
        fcount %= frms;
        return OK;
    }

    int aperf() {
        csnd::AudioSig out(this, outargs(0));
        int cnt = fcount;
        if (!init_done) {
            pinMode(context, 0, pin, 0);
            init_done = 1;
        }
        for (auto& s : out) {
            s = (MYFLT)digitalRead(context, cnt, pin);
            if (cnt == frms - 1)
                cnt = 0;
            else
                cnt++;
        }
        fcount = cnt;
        return OK;
    }
};

/** DigiOut opcode
    digiOutBela ksig,ipin
    digiOutBela asig,ipin
*/
struct DigiOut : csnd::InPlug<2> {
    int pin;
    int fcount;
    int frms;
    int init_done;
    BelaContext* context;

    int init() {
        pin = (int)args[1];
        if (pin < 0)
            pin = 0;
        if (pin > 15)
            pin = 15;
        context = (BelaContext*)csound->host_data();
        init_done = 0;
        fcount = 0;
        frms = context->digitalFrames;
        return OK;
    }

    int kperf() {
        if (!init_done) {
            pinMode(context, 0, pin, 1);
            init_done = 1;
        }
        digitalWrite(context, fcount, pin, (args[0] > 0.0 ? 1 : 0));
        fcount += nsmps;
        fcount %= frms;
        return OK;
    }

    int aperf() {
        csnd::AudioSig in(this, args(0));
        int cnt = fcount;
        if (!init_done) {
            pinMode(context, 0, pin, 1);
            init_done = 1;
        }
        for (auto s : in) {
            digitalWriteOnce(context, cnt, pin, (s > 0.0 ? 1 : 0));
            if (cnt == frms - 1)
                cnt = 0;
            else
                cnt++;
        }
        fcount = cnt;
        return OK;
    }
};

/** DigiIO opcode
    allows change of direction & pin
    ksig/asig are used for input or output
    digiIOBela ksig,kpin,kdir
    digiIOBela asig,apin,adir
*/
struct DigiIO : csnd::InPlug<3> {
    int fcount;
    int frms;
    BelaContext* context;

    int init() {
        context = (BelaContext*)csound->host_data();
        fcount = 0;
        frms = context->digitalFrames;
        return OK;
    }

    int kperf() {
        int pin = (int)args[1];
        if (pin < 0)
            pin = 0;
        if (pin > 15)
            pin = 15;
        int mode = args[2] > 0.0 ? 1 : 0;
        pinMode(context, fcount, pin, mode);
        if (mode)
            digitalWrite(context, fcount, pin, (args[0] > 0.0 ? 1 : 0));
        else
            args[0] = (MYFLT)digitalRead(context, fcount, pin);
        fcount += nsmps;
        fcount %= frms;
        return OK;
    }

    int aperf() {
        csnd::AudioSig sig(this, args(0));
        csnd::AudioSig pins(this, args(1));
        csnd::AudioSig modes(this, args(2));
        int cnt = fcount;
        int n = 0, pin, mode;
        for (auto& s : sig) {
            pin = pins[n];
            if (pin < 0)
                pin = 0;
            if (pin > 15)
                pin = 15;
            mode = modes[n] > 0.0 ? 1 : 0;
            pinModeOnce(context, cnt, pin, mode);
            if (mode)
                digitalWriteOnce(context, cnt, pin, (s > 0.0 ? 1 : 0));
            else
                s = (MYFLT)digitalRead(context, cnt, pin);
            if (cnt == frms - 1)
                cnt = 0;
            else
                cnt++;
            n++;
        }
        fcount = cnt;
        return OK;
    }
};

/*  TrillIn opcode
  read values from Trill sensors
  array sizes must be same as inumTouches.
  - 1D sensors (e.g.: Bar, Ring) will report all the touches as reported by the
  hardware
  - 2D sensors (e.g.: Square, Hex) will report at most one touch, combining the
  horizontal and vertical readings. See "compound touch"
  [here](http://docs.bela.io/classTrill.html)
  - RAW sensors (e.g.: Craft) will report raw readings from all pads. inumTouches
  in this case indicates how many pads to use/report, the reported location
  will be the pad index, and the reported size will be the pad reading.

  kactiveTouches, ksizes[], kvertLocations[], khoriLocations[] trill inumTouches, itrillID
*/
struct TrillIn : csnd::Plugin<4, 2> {
    unsigned int activeTouches;
    unsigned int numTouches;
    unsigned int trillID;
    float* touchSize;
    float* touchVertLocation;
    float* touchHoriLocation;

    int init() {
        numTouches = inargs[0];
        trillID = inargs[1];
        activeTouches = 0;
        if (gTouchSensors.size() < trillID + 1)
            return csound->init_error("No Trill sensor connected");

        touchSize = new float[numTouches];
        touchVertLocation = new float[numTouches];
        touchHoriLocation = new float[numTouches];
        return OK;
    }

    int kperf() {
        Trill& t = *gTouchSensors[trillID];
        if (t.is1D())
            activeTouches = t.getNumTouches();
        else if (t.is2D()) // either 0 or 1 touches
            activeTouches = (t.getNumTouches() || t.getNumHorizontalTouches());
        else // we are in raw/diff/baseline mode: the number of "touches" is actually the number of pads
            activeTouches = std::min(numTouches, t.rawData.size());
        outargs[0] = MYFLT(activeTouches);
        // Read locations from Trill sensor
        for (unsigned int i = 0; i < activeTouches; i++) {
            if (t.is1D()) {
                touchSize[i] = t.touchSize(i);
                touchVertLocation[i] = t.touchLocation(i);
                touchHoriLocation[i] = t.touchHorizontalLocation(i);
            } else if (t.is2D()) {
                touchSize[i] = t.compoundTouchSize();
                touchVertLocation[i] = t.compoundTouchLocation();
                touchHoriLocation[i] = t.compoundTouchHorizontalLocation();
            } else {
                touchSize[i] = t.rawData[i];
                touchVertLocation[i] = i;
            }
        }
        // For all inactive touches, set location and size to 0
        for (unsigned int i = activeTouches; i < numTouches; i++) {
            touchSize[i] = 0.0;
            touchVertLocation[i] = 0.0;
            touchHoriLocation[i] = 0.0;
        }

        csnd::Vector<MYFLT>& out_size = outargs.vector_data<MYFLT>(1);
        csnd::Vector<MYFLT>& out_v_location = outargs.vector_data<MYFLT>(2);
        csnd::Vector<MYFLT>& out_h_location = outargs.vector_data<MYFLT>(3);

        std::copy(touchSize, touchSize + numTouches, out_size.begin());
        std::copy(touchVertLocation, touchVertLocation + numTouches, out_v_location.begin());
        std::copy(touchHoriLocation, touchHoriLocation + numTouches, out_h_location.begin());
        return OK;
    }
};

void trillReadLoop(void*) {
    while (!Bela_stopRequested()) {
        for (unsigned int n = 0; n < gTouchSensors.size(); ++n) {
            Trill* t = gTouchSensors[n];
            t->readI2C();
        }
        usleep(10000);
    }
}

struct CsChan {
    std::vector<MYFLT> samples;
    std::stringstream name;
};

struct CsData {
    Csound* csound;
    std::string csdfile;
    int blocksize;
    std::atomic_int res;
    int count;
    int counti;
    int blockframes;
    std::vector<CsChan> channel;
    std::vector<CsChan> ochannel;
    CsChan schannel;
    Scope scope;
};

bool csound_setup(BelaContext* context, void* p) {
    CsData* csData = (CsData*)p;
    Csound* csound;
    std::string ksmps_string = "--default-ksmps=";
    ksmps_string += std::to_string(context->audioFrames);
    const char* args[] = { "csound",   csData->csdfile.c_str(), "-iadc", "-odac", "-+rtaudio=null", "--realtime",
                           "--daemon", ksmps_string.c_str() };
    int numArgs = (int)(sizeof(args) / sizeof(char*));

    /* allocate analog channel memory */
    csData->channel.resize(context->analogInChannels);
    csData->ochannel.resize(context->analogOutChannels);

    /* set up Csound */
    csound = new Csound();
    csData->csound = csound;
    csound->SetHostData((void*)context);
    csound->SetHostImplementedAudioIO(1, 0);
    csound->SetHostImplementedMIDIIO(1);
    csound->SetExternalMidiInOpenCallback(OpenMidiInDevice);
    csound->SetExternalMidiReadCallback(ReadMidiData);
    csound->SetExternalMidiInCloseCallback(CloseMidiInDevice);
    csound->SetExternalMidiOutOpenCallback(OpenMidiOutDevice);
    csound->SetExternalMidiWriteCallback(WriteMidiData);
    csound->SetExternalMidiOutCloseCallback(CloseMidiOutDevice);
    /* set up digi opcodes */
    if (csnd::plugin<DigiIn>((csnd::Csound*)csound->GetCsound(), "digiInBela", "k", "i", csnd::thread::ik) != 0)
        printf("Warning: could not add digiInBela k-rate opcode\n");
    if (csnd::plugin<DigiIn>((csnd::Csound*)csound->GetCsound(), "digiInBela", "a", "i", csnd::thread::ia) != 0)
        printf("Warning: could not add digiInBela a-rate opcode\n");
    if (csnd::plugin<DigiOut>((csnd::Csound*)csound->GetCsound(), "digiOutBela", "", "ki", csnd::thread::ik) != 0)
        printf("Warning: could not add digiOutBela k-rate opcode\n");
    if (csnd::plugin<DigiOut>((csnd::Csound*)csound->GetCsound(), "digiOutBela", "", "ai", csnd::thread::ia) != 0)
        printf("Warning: could not add digiOutBela a-rate opcode\n");
    if (csnd::plugin<DigiIO>((csnd::Csound*)csound->GetCsound(), "digiIOBela", "", "kkk", csnd::thread::ik) != 0)
        printf("Warning: could not add digiIOBela k-rate opcode\n");
    if (csnd::plugin<DigiIO>((csnd::Csound*)csound->GetCsound(), "digiIOBela", "", "aaa", csnd::thread::ia) != 0)
        printf("Warning: could not add digiIOBela a-rate opcode\n");

    /* set up trill opcode */
    if (csnd::plugin<TrillIn>((csnd::Csound*)csound->GetCsound(), "trill", "kk[]k[]k[]", "ii", csnd::thread::ik) != 0)
        printf("Warning: could not add trillBar a-rate opcode\n");

    /* set up trill sensors */
    unsigned int i2cBus = 1;
    for (uint8_t addr = 0x20; addr <= 0x50; ++addr) {
        Trill::Device device = Trill::probe(i2cBus, addr);
        if (Trill::NONE != device && Trill::CRAFT != device) {
            gTouchSensors.push_back(new Trill(i2cBus, device, addr));
            gTouchSensors.back()->printDetails();
        }
    }

    /* compile CSD */

    if ((csData->res = csound->Compile(numArgs, args)) != 0) {
        printf("Error: Csound could not compile CSD file.\n");
        return false;
    }
    csData->blocksize = csound->GetKsmps();
    if (context->audioFrames % csData->blocksize) {
        fprintf(stderr,
                "Warning: Csound's ksmps (%d) and Bela's periodSize (%u) differ and the latter is not a multiple of "
                "the former. This would lead to uneven CPU usage, and may result in dropouts while some CPU resources "
                "remain unused.\n",
                csData->blocksize, context->audioFrames);
    }
    csData->count = 0;
    csData->counti = 0;
    csData->blockframes = 0;

    /* set up the channels */
    for (unsigned int i = 0; i < csData->channel.size(); i++) {
        csData->channel[i].samples.resize(csound->GetKsmps());
        csData->channel[i].name << "analogIn" << i;
    }

    for (unsigned int i = 0; i < csData->ochannel.size(); i++) {
        csData->ochannel[i].samples.resize(csound->GetKsmps());
        csData->ochannel[i].name << "analogOut" << i;
    }

    csData->schannel.samples.resize(csound->GetKsmps());
    csData->schannel.name << "scope";
    csData->scope.setup(1, context->audioSampleRate);

    // Enable the Trill sensing thread. Do it as the last thing
    // so it doesn't slow down loading.
    if (gTouchSensors.size())
        Bela_runAuxiliaryTask(trillReadLoop);
    return true;
}

void csound_render(BelaContext* context, void* p) {
    CsData* csData = (CsData*)p;
    if (csData->res == 0) {
        unsigned int i, k, count, counti, frmcount, blocksize, blockframes;
        int res = csData->res;
        unsigned int n;
        Csound* csound = csData->csound;
        MYFLT scal = csound->Get0dBFS();
        MYFLT* audioIn = csound->GetSpin();
        MYFLT* audioOut = csound->GetSpout();
        int nchnls = csound->GetNchnls();
        int nchnls_i = csound->GetNchnlsInput();
        unsigned int chns = (unsigned int)nchnls < context->audioOutChannels ? nchnls : context->audioOutChannels;
        unsigned int ichns = (unsigned int)nchnls_i < context->audioInChannels ? nchnls_i : context->audioInChannels;

        std::vector<CsChan>& channel = csData->channel;
        std::vector<CsChan>& ochannel = csData->ochannel;
        CsChan& schannel = csData->schannel;
        Scope& scope = csData->scope;
        float frm = 0.f, incr = ((float)context->analogFrames) / context->audioFrames;
        count = csData->count;
        counti = csData->counti;
        blocksize = csData->blocksize;
        blockframes = csData->blockframes;

        /* processing loop */
        for (n = 0; n < context->audioFrames; n++, blockframes++, frm += incr, count += nchnls, counti += nchnls_i) {
            if (blockframes == blocksize) {
                /* set the channels */
                for (i = 0; i < channel.size(); i++)
                    csound->SetChannel(channel[i].name.str().c_str(), channel[i].samples.data());

                /* run csound */
                if ((res = csound->PerformKsmps()) == 0) {
                    count = 0;
                    counti = 0;
                    blockframes = 0;
                } else
                    break;

                /* get the channels */
                for (i = 0; i < ochannel.size(); i++)
                    csound->GetAudioChannel(ochannel[i].name.str().c_str(), ochannel[i].samples.data());

                /* get the scope data */
                csound->GetAudioChannel(schannel.name.str().c_str(), schannel.samples.data());
            }
            /* read/write audio data */
            for (i = 0; i < ichns; i++)
                audioIn[counti + i] = audioRead(context, n, i) * scal;
            for (i = 0; i < chns; i++)
                audioWrite(context, n, i, audioOut[count + i] / scal);

            /* read analogue data
               analogue frame pos frm gets incremented according to the
               ratio analogFrames/audioFrames.
            */
            frmcount = count / nchnls;
            k = (int)frm;
            for (i = 0; i < channel.size(); i++)
                channel[i].samples[frmcount] = analogRead(context, k, i);

            /* write analogue data */
            for (i = 0; i < ochannel.size(); i++)
                analogWriteOnce(context, k, i, ochannel[i].samples[frmcount]);

            scope.log(schannel.samples[frmcount]);
        }
        csData->res = res;
        csData->count = count;
        csData->counti = counti;
        csData->blockframes = blockframes;
    } else {
        // processing has ended, so let's silence the outputs until main()
        // realises that and tells us to stop
        memset(context->audioOut, 0, sizeof(context->audioOut[0]) * context->audioOutChannels * context->audioFrames);
        memset(context->analogOut, 0,
               sizeof(context->analogOut[0]) * context->analogOutChannels * context->analogFrames);
        memset(context->digital, 0, sizeof(context->digital[0]) * context->digitalFrames);
    }
}

void csound_cleanup(BelaContext* context, void* p) {
    CsData* csData = (CsData*)p;
    delete csData->csound;
    for (auto t : gTouchSensors)
        delete t;
}

static Midi gMidi;
/** MIDI functions */
int OpenMidiInDevice(CSOUND* csound, void** userData, const char* dev) {
    Midi* midi = &gMidi;
    if (midi->readFrom(dev) == 1) {
        midi->enableParser(false);
        *userData = (void*)midi;
        return 0;
    }
    csoundMessage(csound, "Could not open Midi in device %s", dev);
    return -1;
}

int CloseMidiInDevice(CSOUND* csound, void* userData) { return 0; }

int ReadMidiData(CSOUND* csound, void* userData, unsigned char* mbuf, int nbytes) {
    int n = 0, byte;
    if (userData) {
        Midi* midi = (Midi*)userData;
        while ((byte = midi->getInput()) >= 0) {
            *mbuf++ = (unsigned char)byte;
            if (++n == nbytes)
                break;
        }
        return n;
    }
    return 0;
}

int OpenMidiOutDevice(CSOUND* csound, void** userData, const char* dev) {
    Midi* midi = &gMidi;
    if (midi->writeTo(dev) == 1) {
        midi->enableParser(false);
        *userData = (void*)midi;
        return 0;
    }
    csoundMessage(csound, "Could not open Midi out device %s", dev);
    return -1;
}

int CloseMidiOutDevice(CSOUND* csound, void* userData) { return 0; }

int WriteMidiData(CSOUND* csound, void* userData, const unsigned char* mbuf, int nbytes) {
    if (userData) {
        Midi* midi = (Midi*)userData;
        if (midi->writeOutput((midi_byte_t*)mbuf, nbytes) > 0)
            return nbytes;
        return 0;
    }
    return 0;
}

void usage(const char* prg) {
    std::cerr << prg << " [options]\n";
    Bela_usage();
    std::cerr << "  --csd=name [-f name]: CSD file name\n";
    std::cerr << "  --help [-h]: help message\n";
}

/**
   Main program: takes Bela options and a --csd=<csdfile>
   option for Csound
*/
int main(int argc, const char* argv[]) {
    CsData csData;
    int c;
    bool res = false;
    BelaInitSettings* settings;
    settings = Bela_InitSettings_alloc();
    const option opt[] = { { "csd", required_argument, NULL, 'f' }, { "help", 0, NULL, 'h' }, { NULL, 0, NULL, 0 } };

    Bela_defaultSettings(settings);
    settings->setup = csound_setup;
    settings->render = csound_render;
    settings->cleanup = csound_cleanup;
    settings->highPerformanceMode = 1;
    settings->interleave = 1;
    settings->analogOutputsPersist = 0;

    while ((c = Bela_getopt_long(argc, (char**)argv, "hf", opt, settings)) >= 0) {
        if (c == 'h') {
            usage(argv[0]);
            Bela_InitSettings_free(settings);
            return 1;
        } else if (c == 'f') {
            csData.csdfile = optarg;
            res = true;
        } else {
            usage(argv[0]);
            Bela_InitSettings_free(settings);
            return 1;
        }
    }

    if (!res) {
        std::cerr << "no csd provided, use --csd=name \n";
        Bela_InitSettings_free(settings);
        return 1;
    }
    res = Bela_initAudio(settings, &csData);
    Bela_InitSettings_free(settings);
    if (res) {
        std::cerr << "error initialising Bela \n";
        Bela_cleanupAudio();
        return 1;
    }
    if (Bela_startAudio() == 0) {
        while (csData.res == 0)
            usleep(100000);
    } else {
        std::cerr << "error starting audio \n";
    }
    Bela_stopAudio();
    Bela_cleanupAudio();
    return 0;
}
