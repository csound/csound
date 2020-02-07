/*
        rthaiku.cpp:

        Real-time Audio and MIDI for Haiku.
        [Uses HaikuAuidio.cpp and HaikuMidi.cpp]
        (Haiku is the open-source successor to BeOS.
        The API is the same, but current Csound itself is
        unlikely ever to run under the older BeOS.)

        Copyright (C) 2012-2019 Pete Goodeve

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

#include <OS.h>

// Prevent int32 etc conflicts
#define __HAIKU_CONFLICT

#include "haiku_audio.h"
#include "haiku_midi.h"

#include "csound.h"
#include "csdl.h"
#include "soundio.h"


#define XDPRINTF(x)
#define DPRINTF(x)
//#define DPRINTF(x) printf x

/////////////////////////////////////////////////
// Ctrl-C handling to close audio node

static struct sigaction sa, old_sa;

static void gotSigIntAudio(int sig, char* data, vregs*)
{
        DPRINTF(("signal received to Audio\n");)
        sigaction(sig, &old_sa, NULL);
        CSOUND *csound = (CSOUND *)data;
        csoundStop(csound);
        DPRINTF(("signal to Audio called csoundStop\n");)
}


static void setSigAudio(CSOUND *csound)
{
        sa.sa_handler = (__sighandler_t)gotSigIntAudio;
        sigemptyset(&sa.sa_mask);
        sa.sa_userdata = csound;
        sigaction(SIGINT, &sa, &old_sa);
        sigaction(SIGTERM, &sa, NULL);
}

/////////////////////////////////////////////////

/* open for audio input (not yet implemented) */

static int recopen_(CSOUND *csound, const csRtAudioParams *parm)
{
        return -1;
}


/* open for audio output */

static int playopen_(CSOUND *csound, const csRtAudioParams *parm)
{
        void** playdata = csound->GetRtPlayUserData(csound);
         DPRINTF(("devName=%s devNum=%d frag size (smpls)=%d (=%d bytes) buf "
                  "size=%d\nchans=%d fmt=%d rate=%.2f\n",
                  parm->devName, parm->devNum, parm->bufSamp_SW,
                  parm->bufSamp_SW*sizeof(MYFLT), parm->bufSamp_HW,
                  parm->nChannels, parm->sampleFormat, parm->sampleRate);)
        // Note that buffer sample size is float, source is MYFLT (double!)
        Generator *gen =
           new Generator(parm->sampleRate, parm->nChannels,
                         parm->bufSamp_SW*sizeof(float)*parm->nChannels,
                         sizeof(MYFLT));
        *playdata = gen;
        setSigAudio(csound);
        return gen->RunAudio();
}


/* get samples from ADC (not yet implemented) */

static int rtrecord_(CSOUND *csound, MYFLT *inbuf, int nbytes)
{
        return -1;
}


/* put samples to DAC */

static void rtplay_(CSOUND *csound, const MYFLT *outbuf, int nbytes)
{
        Generator * gen = (Generator *)*csound->GetRtPlayUserData(csound);
        if (!gen) return;
        if (gen->mBufSize*(sizeof(MYFLT)/sizeof(float)) < (size_t)nbytes) {
          // we assume MYFLT === double for now...
          csound->ErrorMsg(csound,
                           Str("buffer mismatch! source %d <>  dest %ld\n"),
                           nbytes, gen->mBufSize);
                return;
        }
        gen->mXferSize = nbytes;
        gen->mDataBuf = (double *)outbuf;
        status_t res = acquire_sem(gen->cs_sem);
        if (res != B_OK) fprintf(stderr, "cs_sem failed\n");
}


/* close the I/O device entirely  */
/* called only when both complete */

static void rtclose_(CSOUND *csound)
{
        void** playdata = csound->GetRtPlayUserData(csound);
//      Generator * gen = (Generator *)*csound->GetRtPlayUserData(csound);
        Generator * gen = (Generator *)*playdata;
        delete gen;
        *playdata = NULL;
}


/////////////////////////////////////////////////
// Ctrl-C handling to remove midi node

static struct sigaction sa_midi, old_sa_midi;

static void gotSigIntMidi(int sig, char* data, vregs*)
{
        DPRINTF(("signal received to Midi\n");)
        CSOUND *csound = (CSOUND *)data;
        csoundStop(csound);
        DPRINTF(("signal to MIDI called csoundStop\n");)
}


static void setSigMidi(CSOUND *csound)
{
        sa_midi.sa_handler = (__sighandler_t)gotSigIntMidi;
        sigemptyset(&sa_midi.sa_mask);
        sa.sa_userdata = csound;
        sigaction(SIGINT, &sa_midi, &old_sa_midi);
        sigaction(SIGTERM, &sa_midi, NULL);
}

/////////////////////////////////////////////////

static int midi_in_open(CSOUND *csound, void **userData, const char *devName)
{
        MidiIn *hmidi = new MidiIn(devName);
        if (*devName == '?') {  // was a query -- don't proceed
                delete hmidi;
                exit(1);
        }
        *userData = hmidi;
        setSigMidi(csound);
        return 0;
}


static int midi_in_read(CSOUND *csound,
                        void *userData, unsigned char *buf, int nbytes)
{
        MidiIn *hmidi = (MidiIn *) userData;
        int bufpos = 0;
        uint32 evword = 0;
        while (bufpos < nbytes-3 && (evword = hmidi->GetEvent()) != 0) {
                MidiEvent ev = evword;
                uint8 *evp = ev.Bytes();
                for (int i=0; i<ev.Size(); i++) buf[bufpos++] = *evp++;
        }
        return bufpos;
}


static int midi_in_close(CSOUND *csound, void *userData)
{
        MidiIn *hmidi = (MidiIn *) userData;
        delete hmidi;
    return 0;
}


/* Output to MIDI Device (not yet implemented) */

static int midi_out_open(CSOUND *csound, void **userData, const char *devName)
{
        return -1;
}


static int midi_out_write(CSOUND *csound,
                          void *userData, const unsigned char *buf, int nbytes)
{
        return 0;
}


static int midi_out_close(CSOUND *csound, void *userData)
{
        return 0;
}


/* module interface functions */

#ifdef __cplusplus
extern "C" {
PUBLIC int csoundModuleCreate(CSOUND *csound);
PUBLIC int csoundModuleInit(CSOUND *csound);
PUBLIC int csoundModuleInfo(void);
};
#endif

PUBLIC int csoundModuleCreate(CSOUND *csound)
{
        OPARMS oparms;
        csound->GetOParms(csound, &oparms);
//      printf("haiku module entered -- msg level = %x\n", oparms.msglevel);
        /* report success */
        if (oparms.msglevel & 0x400)
          csound->Message(csound, Str("Haiku real-time audio and MIDI module "
                                      "for Csound by Pete Goodeve\n"));
        return 0;
}


PUBLIC int csoundModuleInit(CSOUND *csound)
{
        char    *s;
        int      i;
        char    buf[9];

    csound->module_list_add(csound, (char *)"haiku", (char *)"audio");
    csound->module_list_add(csound, (char *)"haiku", (char *)"midi");
        s = (char*) csound->QueryGlobalVariable(csound, "_RTAUDIO");
        i = 0;
        if (s != NULL) {
          while (*s != (char)0 && i < 8)
                buf[i++] = *(s++) | (char) 0x20;
        }
        buf[i] = (char) 0;
        if (strcmp(&(buf[0]), "haiku") == 0) {
          csound->Message(csound, Str("rtaudio: Haiku module enabled\n"));
          csound->SetPlayopenCallback(csound, playopen_);
          csound->SetRecopenCallback(csound, recopen_);
          csound->SetRtplayCallback(csound, rtplay_);
          csound->SetRtrecordCallback(csound, rtrecord_);
          csound->SetRtcloseCallback(csound, rtclose_);
        }
        s = (char*) csound->QueryGlobalVariable(csound, "_RTMIDI");
        i = 0;
        if (s != NULL) {
          while (*s != (char) 0 && i < 8)
                buf[i++] = *(s++) | (char) 0x20;
        }
        buf[i] = (char) 0;
        if (strcmp(&(buf[0]), "haiku") == 0) {
          csound->Message(csound, Str("rtmidi: Haiku module enabled\n"));
          csound->SetExternalMidiInOpenCallback(csound, midi_in_open);
          csound->SetExternalMidiReadCallback(csound, midi_in_read);
          csound->SetExternalMidiInCloseCallback(csound, midi_in_close);
          csound->SetExternalMidiOutOpenCallback(csound, midi_out_open);
          csound->SetExternalMidiWriteCallback(csound, midi_out_write);
          csound->SetExternalMidiOutCloseCallback(csound, midi_out_close);
        }

        return 0;
}


PUBLIC int csoundModuleInfo(void)
{
        return ((CS_APIVERSION << 16) + (CS_APISUBVER << 8) + (int) sizeof(MYFLT));
}
