/*
    csound_rtio.c: engine rtio

    Copyright (C) 2025 The Csound Developers

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
#include "csoundCore.h"
#include "csmodule.h"

/* dummy functions for the case when no real-time audio module is available */
static double *get_dummy_rtaudio_globals(CSOUND *csound) {
  double *p;

  p = (double *)csound->QueryGlobalVariable(csound, "__rtaudio_null_state");
  if (p == NULL) {
    if (UNLIKELY(csound->CreateGlobalVariable(csound, "__rtaudio_null_state",
                                              sizeof(double) * 4) != 0))
      csound->Die(csound, Str("rtdummy: failed to allocate globals"));
    csound->Message(csound, Str("rtaudio: dummy module enabled\n"));
    p = (double *)csound->QueryGlobalVariable(csound, "__rtaudio_null_state");
  }
  return p;
}

static void dummy_rtaudio_timer(CSOUND *csound, double *p) {
  double timeWait;
  int32_t i;

  timeWait = p[0] - csoundGetRealTime(csound->csRtClock);
  i = (int32_t)(timeWait * 1000.0 + 0.5);
  if (i > 0)
    csoundSleep((size_t)i);
}

int32_t playopen_dummy(CSOUND *csound, const csRtAudioParams *parm) {
  double *p;
  char *s;

  /* find out if the use of dummy real-time audio functions was requested, */
  /* or an unknown plugin name was specified; the latter case is an error  */
  s = (char *)csoundQueryGlobalVariable(csound, "_RTAUDIO");
  if (s != NULL && !(strcmp(s, "null") == 0 || strcmp(s, "Null") == 0 ||
                     strcmp(s, "NULL") == 0)) {
    if (s[0] == '\0')
      csoundErrorMsg(csound,
                     Str(" *** error: rtaudio module set to empty string"));
    else {
      // print_opcodedir_warning(csound);
      csoundErrorMsg(
          csound, Str(" unknown rtaudio module: '%s', using dummy module"), s);
    }
    // return CSOUND_ERROR;
  }
  p = get_dummy_rtaudio_globals(csound);
  csound->rtPlay_userdata = (void *)p;
  p[0] = csound->GetRealTime(csound->csRtClock);
  p[1] = 1.0 / ((double)((int32_t)sizeof(MYFLT) * parm->nChannels) *
                (double)parm->sampleRate);
  return CSOUND_SUCCESS;
}

void rtplay_dummy(CSOUND *csound, const MYFLT *outBuf, int32_t nbytes) {
  double *p = (double *)csound->rtPlay_userdata;
  (void)outBuf;
  p[0] += ((double)nbytes * p[1]);
  dummy_rtaudio_timer(csound, p);
}

int32_t recopen_dummy(CSOUND *csound, const csRtAudioParams *parm) {
  double *p;
  char *s;

  /* find out if the use of dummy real-time audio functions was requested, */
  /* or an unknown plugin name was specified; the latter case is an error  */
  s = (char *)csoundQueryGlobalVariable(csound, "_RTAUDIO");
  if (s != NULL && !(strcmp(s, "null") == 0 || strcmp(s, "Null") == 0 ||
                     strcmp(s, "NULL") == 0)) {
    if (s[0] == '\0')
      csoundErrorMsg(csound,
                     Str(" *** error: rtaudio module set to empty string"));
    else {
      // print_opcodedir_warning(csound);
      csoundErrorMsg(
          csound, Str(" unknown rtaudio module: '%s', using dummy module"), s);
    }
    // return CSOUND_ERROR;
  }
  p = (double *)get_dummy_rtaudio_globals(csound) + 2;
  csound->rtRecord_userdata = (void *)p;
  p[0] = csound->GetRealTime(csound->csRtClock);
  p[1] = 1.0 / ((double)((int32_t)sizeof(MYFLT) * parm->nChannels) *
                (double)parm->sampleRate);
  return CSOUND_SUCCESS;
}

int32_t rtrecord_dummy(CSOUND *csound, MYFLT *inBuf, int32_t nbytes) {
  double *p = (double *)csound->rtRecord_userdata;

  /* for (i = 0; i < (nbytes / (int32_t) sizeof(MYFLT)); i++) */
  /*   ((MYFLT*) inBuf)[i] = FL(0.0); */
  memset(inBuf, 0, nbytes);

  p[0] += ((double)nbytes * p[1]);
  dummy_rtaudio_timer(csound, p);

  return nbytes;
}

void rtclose_dummy(CSOUND *csound) {
  csound->rtPlay_userdata = NULL;
  csound->rtRecord_userdata = NULL;
}

int32_t audio_dev_list_dummy(CSOUND *csound, CS_AUDIODEVICE *list,
                             int32_t isOutput) {
  IGN(csound);
  IGN(list);
  IGN(isOutput);
  return 0;
}

int32_t midi_dev_list_dummy(CSOUND *csound, CS_MIDIDEVICE *list,
                            int32_t isOutput) {
  IGN(csound);
  IGN(list);
  IGN(isOutput);
  return 0;
}

void csoundSetPlayopenCallback(
    CSOUND *csound,
    int32_t (*playopen__)(CSOUND *, const csRtAudioParams *parm)) {
  csound->playopen_callback = playopen__;
}

void csoundSetRtplayCallback(CSOUND *csound,
                             void (*rtplay__)(CSOUND *, const MYFLT *outBuf,
                                              int32_t nbytes)) {
  csound->rtplay_callback = rtplay__;
}

void csoundSetRecopenCallback(
    CSOUND *csound,
    int32_t (*recopen__)(CSOUND *, const csRtAudioParams *parm)) {
  csound->recopen_callback = recopen__;
}

void csoundSetRtrecordCallback(CSOUND *csound,
                               int32_t (*rtrecord__)(CSOUND *, MYFLT *inBuf,
                                                     int32_t nbytes)) {
  csound->rtrecord_callback = rtrecord__;
}

void csoundSetRtcloseCallback(CSOUND *csound, void (*rtclose__)(CSOUND *)) {
  csound->rtclose_callback = rtclose__;
}

void csoundSetAudioDeviceListCallback(
    CSOUND *csound, int32_t (*audiodevlist__)(CSOUND *, CS_AUDIODEVICE *list,
                                              int32_t isOutput)) {
  csound->audio_dev_list_callback = audiodevlist__;
}

PUBLIC void csoundSetMIDIDeviceListCallback(
    CSOUND *csound,
    int32_t (*mididevlist__)(CSOUND *, CS_MIDIDEVICE *list, int32_t isOutput)) {
  csound->midi_dev_list_callback = mididevlist__;
}

PUBLIC int32_t csoundGetAudioDevList(CSOUND *csound, CS_AUDIODEVICE *list,
                                     int32_t isOutput) {
  return csound->audio_dev_list_callback(csound, list, isOutput);
}

PUBLIC int32_t csoundGetMIDIDevList(CSOUND *csound, CS_MIDIDEVICE *list,
                                    int32_t isOutput) {
  return csound->midi_dev_list_callback(csound, list, isOutput);
}

/* dummy real time MIDI functions */
int32_t DummyMidiInOpen(CSOUND *csound, void **userData, const char *devName) {
  char *s;

  (void)devName;
  *userData = NULL;
  s = (char *)csoundQueryGlobalVariable(csound, "_RTMIDI");
  if (UNLIKELY(s == NULL || (strcmp(s, "null") == 0 || strcmp(s, "Null") == 0 ||
                             strcmp(s, "NULL") == 0))) {
    csoundMessage(csound, Str("!!WARNING: real time midi input disabled, "
                              "using dummy functions\n"));
    return 0;
  }
  if (s[0] == '\0')
    csoundErrorMsg(csound, Str("error: -+rtmidi set to empty string"));
  else {
    print_opcodedir_warning(csound);
    csoundErrorMsg(csound, Str("error: -+rtmidi='%s': unknown module"), s);
  }
  return -1;
}

int32_t DummyMidiRead(CSOUND *csound, void *userData, unsigned char *buf,
                      int32_t nbytes) {
  (void)csound;
  (void)userData;
  (void)buf;
  (void)nbytes;
  return 0;
}

int32_t DummyMidiOutOpen(CSOUND *csound, void **userData, const char *devName) {
  char *s;

  (void)devName;
  *userData = NULL;
  s = (char *)csoundQueryGlobalVariable(csound, "_RTMIDI");
  if (s == NULL || (strcmp(s, "null") == 0 || strcmp(s, "Null") == 0 ||
                    strcmp(s, "NULL") == 0)) {
    csoundMessage(csound, Str("WARNING: real time midi output disabled, "
                              "using dummy functions\n"));
    return 0;
  }
  if (s[0] == '\0')
    csoundErrorMsg(csound, Str("error: -+rtmidi set to empty string"));
  else {
    print_opcodedir_warning(csound);
    csoundErrorMsg(csound, Str("error: -+rtmidi='%s': unknown module"), s);
  }
  return -1;
}

int32_t DummyMidiWrite(CSOUND *csound, void *userData, const unsigned char *buf,
                       int32_t nbytes) {
  (void)csound;
  (void)userData;
  (void)buf;
  return nbytes;
}

static const char *midi_err_msg = Str_noop("Unknown MIDI error");

/**
 * Returns pointer to a string constant storing an error massage
 * for error code 'errcode'.
 */
const char *csoundExternalMidiErrorString(CSOUND *csound, int32_t errcode) {
  if (csound->midiGlobals->MidiErrorStringCallback == NULL)
    return midi_err_msg;
  return (csound->midiGlobals->MidiErrorStringCallback(errcode));
}

/* Set real time MIDI function pointers. */
PUBLIC void csoundSetExternalMidiInOpenCallback(
    CSOUND *csound, int32_t (*func)(CSOUND *, void **, const char *)) {
  csound->midiGlobals->MidiInOpenCallback = func;
}

PUBLIC void csoundSetExternalMidiReadCallback(CSOUND *csound,
                                              int32_t (*func)(CSOUND *, void *,
                                                              unsigned char *,
                                                              int32_t)) {
  csound->midiGlobals->MidiReadCallback = func;
}

PUBLIC void csoundSetExternalMidiInCloseCallback(CSOUND *csound,
                                                 int32_t (*func)(CSOUND *,
                                                                 void *)) {
  csound->midiGlobals->MidiInCloseCallback = func;
}

PUBLIC void csoundSetExternalMidiOutOpenCallback(
    CSOUND *csound, int32_t (*func)(CSOUND *, void **, const char *)) {
  csound->midiGlobals->MidiOutOpenCallback = func;
}

PUBLIC void csoundSetExternalMidiWriteCallback(
    CSOUND *csound,
    int32_t (*func)(CSOUND *, void *, const unsigned char *, int32_t)) {
  csound->midiGlobals->MidiWriteCallback = func;
}

PUBLIC void csoundSetExternalMidiOutCloseCallback(CSOUND *csound,
                                                  int32_t (*func)(CSOUND *,
                                                                  void *)) {
  csound->midiGlobals->MidiOutCloseCallback = func;
}

PUBLIC void
csoundSetExternalMidiErrorStringCallback(CSOUND *csound,
                                         const char *(*func)(int32_t)) {
  csound->midiGlobals->MidiErrorStringCallback = func;
}

/**
 * Calling this function with a non-zero will disable all default
 * handling of sound I/O by the Csound library, allowing the host
 * application to use the spin/<spout/input/output buffers directly.
 * If 'bufSize' is greater than zero, the buffer size (-b) will be
 * set to the integer multiple of ksmps that is nearest to the value
 * specified.
 */

PUBLIC void csoundSetHostImplementedAudioIO(CSOUND *csound, int32_t state,
                                            int32_t bufSize) {
  csound->enableHostImplementedAudioIO = state;
  csound->hostRequestedBufferSize = (bufSize > 0 ? bufSize : 0);
}

PUBLIC void csoundSetHostImplementedMIDIIO(CSOUND *csound, int32_t state) {
  csound->enableHostImplementedMIDIIO = state;
}

PUBLIC double csoundGetScoreTime(CSOUND *csound) {
  double curtime = csound->icurTimeSamples;
  double esr = csound->esr;
  return curtime / esr;
}

PUBLIC void csoundSetHostAudioIO(CSOUND *csound) {
  csound->enableHostImplementedAudioIO = 1;
}

PUBLIC void csoundSetHostMIDIIO(CSOUND *csound) {
  csound->enableHostImplementedMIDIIO = 1;
}

/**
 * Return pointer to user data pointer for real time audio input.
 */
void **csoundGetRtRecordUserData(CSOUND *csound) {
  return &(csound->rtRecord_userdata);
}

/**
 * Return pointer to user data pointer for real time audio output.
 */
void **csoundGetRtPlayUserData(CSOUND *csound) {
  return &(csound->rtPlay_userdata);
}


PUBLIC void csoundSetRTAudioModule(CSOUND *csound, const char *module) {
  char *s;
  if ((s = csoundQueryGlobalVariable(csound, "_RTAUDIO")) != NULL)
    strNcpy(s, module, 20);
  if (UNLIKELY(s == NULL))
    return; /* Should not happen */
  if (strcmp(s, "null") == 0 || strcmp(s, "Null") == 0 ||
      strcmp(s, "NULL") == 0) {
    csound->Message(csound, Str("setting dummy interface\n"));
    csound->SetPlayopenCallback(csound, playopen_dummy);
    csound->SetRecopenCallback(csound, recopen_dummy);
    csound->SetRtplayCallback(csound, rtplay_dummy);
    csound->SetRtrecordCallback(csound, rtrecord_dummy);
    csound->SetRtcloseCallback(csound, rtclose_dummy);
    csound->SetAudioDeviceListCallback(csound, audio_dev_list_dummy);
    return;
  }
  if (csoundInitModules(csound) != 0)
    csound->LongJmp(csound, 1);
}

PUBLIC void csoundSetMIDIModule(CSOUND *csound, const char *module) {
  char *s;

  if ((s = csoundQueryGlobalVariable(csound, "_RTMIDI")) != NULL)
    strNcpy(s, module, 20);
  if (UNLIKELY(s == NULL))
    return; /* Should not happen */
  if (strcmp(s, "null") == 0 || strcmp(s, "Null") == 0 ||
      strcmp(s, "NULL") == 0) {
    csound->SetMIDIDeviceListCallback(csound, midi_dev_list_dummy);
    csound->SetExternalMidiInOpenCallback(csound, DummyMidiInOpen);
    csound->SetExternalMidiReadCallback(csound, DummyMidiRead);
    csound->SetExternalMidiInCloseCallback(csound, NULL);
    csound->SetExternalMidiOutOpenCallback(csound, DummyMidiOutOpen);
    csound->SetExternalMidiWriteCallback(csound, DummyMidiWrite);
    csound->SetExternalMidiOutCloseCallback(csound, NULL);

    return;
  }
  if (csoundInitModules(csound) != 0)
    csound->LongJmp(csound, 1);
}
